// Ed25519 signatures as described in RFC 8032,
// https://www.rfc-editor.org/rfc/rfc8032
//
// This is given code: you call ed25519_sign() and ed25519_verify(), you don't
// need to change anything here. It follows the structure of TweetNaCl
// (https://tweetnacl.cr.yp.to), a tiny public-domain crypto library, and
// favors being short and readable over being fast.
//
// Background, if you're curious:
// - Numbers are elements of the field of integers mod p = 2^255 - 19. Each one
//   is stored as 16 limbs of 16 bits (a `gf`), with room in each int64_t for
//   the carries that build up during addition and multiplication.
// - Points are on the twisted Edwards curve -x^2 + y^2 = 1 + d x^2 y^2, stored
//   as extended coordinates (X, Y, Z, T) with x = X/Z, y = Y/Z, xy = T/Z.
// - The curve has a base point B whose multiples form a group of prime order
//   L = 2^252 + 27742317777372353535851937790883648493. Scalars are mod L.
//
// Signing with private key k (RFC 8032 section 5.1.6):
//   h = SHA-512(k), a = the first half of h (clamped), prefix = the second half
//   A = a*B (the public key)
//   r = SHA-512(prefix || msg) mod L
//   R = r*B
//   S = (r + SHA-512(R || A || msg) * a) mod L
//   signature = R || S
//
// Verifying (section 5.1.7): check that S*B == R + SHA-512(R || A || msg)*A.
#include <string.h>

#include "crypto.hpp"

typedef int64_t gf[16];

static const gf gf0 = {0};
static const gf gf1 = {1};
// d = -121665/121666, the curve constant.
static const gf D = {0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141,
                     0x0a4d, 0x0070, 0xe898, 0x7779, 0x4079, 0x8cc7,
                     0xfe73, 0x2b6f, 0x6cee, 0x5203};
// 2*d
static const gf D2 = {0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283,
                      0x149a, 0x00e0, 0xd130, 0xeef3, 0x80f2, 0x198e,
                      0xfce7, 0x56df, 0xd9dc, 0x2406};
// The base point B = (X, Y).
static const gf X = {0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525,
                     0xc760, 0x692c, 0xdc5c, 0xfdd6, 0xe231, 0xc0a4,
                     0x53fe, 0xcd6e, 0x36d3, 0x2169};
static const gf Y = {0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
                     0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
                     0x6666, 0x6666, 0x6666, 0x6666};
// sqrt(-1) mod p
static const gf I = {0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f,
                     0x1806, 0x2f43, 0xd7a7, 0x3dfb, 0x0099, 0x2b4d,
                     0xdf0b, 0x4fc1, 0x2480, 0x2b83};
// The group order L, little-endian bytes.
static const int64_t L[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
                              0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
                              0,    0,    0,    0,    0,    0,    0,    0,
                              0,    0,    0,    0,    0,    0,    0,    0x10};

// ---------- Field arithmetic mod p ----------

static void set25519(gf r, const gf a) {
    for (int i = 0; i < 16; i++) {
        r[i] = a[i];
    }
}

// Push each limb's overflow into the next limb. The top limb's overflow is
// worth 2^256 = 38 (mod p), so it wraps around to limb 0 times 38.
static void car25519(gf o) {
    for (int i = 0; i < 16; i++) {
        o[i] += (int64_t)1 << 16;
        int64_t c = o[i] >> 16;
        if (i < 15) {
            o[i + 1] += c - 1;
        } else {
            o[0] += 38 * (c - 1);
        }
        o[i] -= c * ((int64_t)1 << 16);
    }
}

// Swap p and q if b is 1, without branching on b (constant time).
static void sel25519(gf p, gf q, int b) {
    int64_t mask = ~((int64_t)b - 1);
    for (int i = 0; i < 16; i++) {
        int64_t t = mask & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}

// Write n, fully reduced mod p, as 32 little-endian bytes.
static void pack25519(uint8_t o[32], const gf n) {
    gf m, t;
    set25519(t, n);
    car25519(t);
    car25519(t);
    car25519(t);
    for (int j = 0; j < 2; j++) {
        // m = t - p; keep m instead of t if that didn't go negative.
        m[0] = t[0] - 0xffed;
        for (int i = 1; i < 15; i++) {
            m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
            m[i - 1] &= 0xffff;
        }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        int b = (int)((m[15] >> 16) & 1);
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);
    }
    for (int i = 0; i < 16; i++) {
        o[2 * i] = (uint8_t)(t[i] & 0xff);
        o[2 * i + 1] = (uint8_t)(t[i] >> 8);
    }
}

static void unpack25519(gf o, const uint8_t n[32]) {
    for (int i = 0; i < 16; i++) {
        o[i] = n[2 * i] + ((int64_t)n[2 * i + 1] << 8);
    }
    o[15] &= 0x7fff;
}

// Compare n bytes without stopping at the first difference, so the time
// taken doesn't reveal where they differ. Returns 0 if equal, -1 if not.
static int verify_bytes(const uint8_t* x, const uint8_t* y, size_t n) {
    uint32_t diff = 0;
    for (size_t i = 0; i < n; i++) {
        diff |= x[i] ^ y[i];
    }
    return diff == 0 ? 0 : -1;
}

// Returns nonzero if a != b (mod p).
static int neq25519(const gf a, const gf b) {
    uint8_t c[32], d[32];
    pack25519(c, a);
    pack25519(d, b);
    return verify_bytes(c, d, 32);
}

// The lowest bit of a (mod p), which encodes the sign of x in a point.
static uint8_t par25519(const gf a) {
    uint8_t d[32];
    pack25519(d, a);
    return d[0] & 1;
}

static void A(gf o, const gf a, const gf b) {
    for (int i = 0; i < 16; i++) {
        o[i] = a[i] + b[i];
    }
}

static void Z(gf o, const gf a, const gf b) {
    for (int i = 0; i < 16; i++) {
        o[i] = a[i] - b[i];
    }
}

static void M(gf o, const gf a, const gf b) {
    int64_t t[31] = {0};
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            t[i + j] += a[i] * b[j];
        }
    }
    // Limb 16 + i is worth 2^256 * limb i = 38 * limb i (mod p).
    for (int i = 0; i < 15; i++) {
        t[i] += 38 * t[i + 16];
    }
    for (int i = 0; i < 16; i++) {
        o[i] = t[i];
    }
    car25519(o);
    car25519(o);
}

static void S(gf o, const gf a) { M(o, a, a); }

// o = i^(p-2) = 1/i (mod p), by Fermat's little theorem.
static void inv25519(gf o, const gf i) {
    gf c;
    set25519(c, i);
    for (int a = 253; a >= 0; a--) {
        S(c, c);
        if (a != 2 && a != 4) {
            M(c, c, i);
        }
    }
    set25519(o, c);
}

// o = i^((p-5)/8), used to take square roots.
static void pow2523(gf o, const gf i) {
    gf c;
    set25519(c, i);
    for (int a = 250; a >= 0; a--) {
        S(c, c);
        if (a != 1) {
            M(c, c, i);
        }
    }
    set25519(o, c);
}

// ---------- Curve points ----------

// p = p + q
static void add(gf p[4], gf q[4]) {
    gf a, b, c, d, t, e, f, g, h;

    Z(a, p[1], p[0]);
    Z(t, q[1], q[0]);
    M(a, a, t);
    A(b, p[0], p[1]);
    A(t, q[0], q[1]);
    M(b, b, t);
    M(c, p[3], q[3]);
    M(c, c, D2);
    M(d, p[2], q[2]);
    A(d, d, d);
    Z(e, b, a);
    Z(f, d, c);
    A(g, d, c);
    A(h, b, a);

    M(p[0], e, f);
    M(p[1], h, g);
    M(p[2], g, f);
    M(p[3], e, h);
}

static void cswap(gf p[4], gf q[4], int b) {
    for (int i = 0; i < 4; i++) {
        sel25519(p[i], q[i], b);
    }
}

// Encode a point as 32 bytes: y, with the top bit holding the sign of x.
static void pack(uint8_t r[32], gf p[4]) {
    gf tx, ty, zi;
    inv25519(zi, p[2]);
    M(tx, p[0], zi);
    M(ty, p[1], zi);
    pack25519(r, ty);
    r[31] ^= (uint8_t)(par25519(tx) << 7);
}

// p = s * q, for a 32-byte little-endian scalar s. Uses the same sequence of
// operations for every s (a "Montgomery ladder"), so timing doesn't leak s.
static void scalarmult(gf p[4], gf q[4], const uint8_t s[32]) {
    set25519(p[0], gf0);
    set25519(p[1], gf1);
    set25519(p[2], gf1);
    set25519(p[3], gf0);
    for (int i = 255; i >= 0; i--) {
        int b = (s[i / 8] >> (i & 7)) & 1;
        cswap(p, q, b);
        add(q, p);
        add(p, p);
        cswap(p, q, b);
    }
}

// p = s * B
static void scalarbase(gf p[4], const uint8_t s[32]) {
    gf q[4];
    set25519(q[0], X);
    set25519(q[1], Y);
    set25519(q[2], gf1);
    M(q[3], X, Y);
    scalarmult(p, q, s);
}

// Decode a 32-byte point into r, negated (so verify can compute S*B - h*A).
// Returns -1 if the bytes aren't a point on the curve.
static int unpackneg(gf r[4], const uint8_t p[32]) {
    gf t, chk, num, den, den2, den4, den6;

    set25519(r[2], gf1);
    unpack25519(r[1], p);
    // x^2 = (y^2 - 1) / (d y^2 + 1)
    S(num, r[1]);
    M(den, num, D);
    Z(num, num, r[2]);
    A(den, r[2], den);

    S(den2, den);
    S(den4, den2);
    M(den6, den4, den2);
    M(t, den6, num);
    M(t, t, den);

    pow2523(t, t);
    M(t, t, num);
    M(t, t, den);
    M(t, t, den);
    M(r[0], t, den);

    S(chk, r[0]);
    M(chk, chk, den);
    if (neq25519(chk, num)) {
        M(r[0], r[0], I);
    }

    S(chk, r[0]);
    M(chk, chk, den);
    if (neq25519(chk, num)) {
        return -1;
    }

    if (par25519(r[0]) == (p[31] >> 7)) {
        Z(r[0], gf0, r[0]);
    }

    M(r[3], r[0], r[1]);
    return 0;
}

// ---------- Scalars mod L ----------

// r = x mod L, for a 64-byte little-endian number x stored one byte per limb.
static void modL(uint8_t r[32], int64_t x[64]) {
    int64_t carry;
    int i, j;
    for (i = 63; i >= 32; --i) {
        carry = 0;
        for (j = i - 32; j < i - 12; ++j) {
            x[j] += carry - 16 * x[i] * L[j - (i - 32)];
            carry = (x[j] + 128) >> 8;
            x[j] -= carry * 256;
        }
        x[j] += carry;
        x[i] = 0;
    }
    carry = 0;
    for (j = 0; j < 32; j++) {
        x[j] += carry - (x[31] >> 4) * L[j];
        carry = x[j] >> 8;
        x[j] &= 255;
    }
    for (j = 0; j < 32; j++) {
        x[j] -= carry * L[j];
    }
    for (i = 0; i < 32; i++) {
        x[i + 1] += x[i] >> 8;
        r[i] = (uint8_t)(x[i] & 255);
    }
}

// Reduce a 64-byte hash mod L, in place (the result is in the first 32 bytes).
static void reduce(uint8_t r[64]) {
    int64_t x[64];
    for (int i = 0; i < 64; i++) {
        x[i] = r[i];
    }
    memset(r, 0, 64);
    modL(r, x);
}

// Returns 1 if the 32-byte little-endian scalar s is less than L.
static int scalar_is_canonical(const uint8_t s[32]) {
    for (int i = 31; i >= 0; i--) {
        if (s[i] < L[i]) {
            return 1;
        }
        if (s[i] > L[i]) {
            return 0;
        }
    }
    return 0;  // s == L
}

// ---------- Public API ----------

// h = SHA-512(private_key); the first half becomes the secret scalar a.
static void expand_private_key(
    const uint8_t private_key[ED25519_PRIVATE_KEY_LEN], uint8_t h[64]) {
    sha512(private_key, ED25519_PRIVATE_KEY_LEN, h);
    h[0] &= 248;
    h[31] &= 127;
    h[31] |= 64;
}

void ed25519_public_key(const uint8_t private_key[ED25519_PRIVATE_KEY_LEN],
                        uint8_t public_key[ED25519_PUBLIC_KEY_LEN]) {
    uint8_t h[64];
    gf p[4];
    expand_private_key(private_key, h);
    scalarbase(p, h);
    pack(public_key, p);
}

void ed25519_sign(const uint8_t* msg, size_t len,
                  const uint8_t private_key[ED25519_PRIVATE_KEY_LEN],
                  uint8_t sig[ED25519_SIGNATURE_LEN]) {
    uint8_t h[64], r[64], k[64], public_key[32];
    gf p[4];
    sha512_ctx_t ctx;

    expand_private_key(private_key, h);
    scalarbase(p, h);
    pack(public_key, p);

    // r = SHA-512(prefix || msg) mod L
    sha512_init(&ctx);
    sha512_update(&ctx, &h[32], 32);
    sha512_update(&ctx, msg, len);
    sha512_final(&ctx, r);
    reduce(r);

    // R = r*B
    scalarbase(p, r);
    pack(sig, p);

    // k = SHA-512(R || A || msg) mod L
    sha512_init(&ctx);
    sha512_update(&ctx, sig, 32);
    sha512_update(&ctx, public_key, 32);
    sha512_update(&ctx, msg, len);
    sha512_final(&ctx, k);
    reduce(k);

    // S = (r + k*a) mod L
    int64_t x[64] = {0};
    for (int i = 0; i < 32; i++) {
        x[i] = r[i];
    }
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            x[i + j] += (int64_t)k[i] * h[j];
        }
    }
    modL(&sig[32], x);
}

int ed25519_verify(const uint8_t* msg, size_t len,
                   const uint8_t public_key[ED25519_PUBLIC_KEY_LEN],
                   const uint8_t sig[ED25519_SIGNATURE_LEN]) {
    uint8_t k[64], check[32];
    gf p[4], q[4];
    sha512_ctx_t ctx;

    // RFC 8032 requires S < L. Otherwise S + L would be a second valid
    // signature for the same message.
    if (!scalar_is_canonical(&sig[32])) {
        return -1;
    }
    // q = -A
    if (unpackneg(q, public_key) != 0) {
        return -1;
    }

    // k = SHA-512(R || A || msg) mod L
    sha512_init(&ctx);
    sha512_update(&ctx, sig, 32);
    sha512_update(&ctx, public_key, 32);
    sha512_update(&ctx, msg, len);
    sha512_final(&ctx, k);
    reduce(k);

    // check = S*B - k*A, which should equal R.
    scalarmult(p, q, k);
    scalarbase(q, &sig[32]);
    add(p, q);
    pack(check, p);

    return verify_bytes(sig, check, 32);
}
