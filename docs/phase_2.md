# Integrity Checks!

Like seen before in the [Python cryptography lab](https://github.com/pbrucla/2027-ectf-crypto-lab), we want to implement hashes to verify integrity! For this week, we're only focused on hashes since I figured C/C++ development is a bit of a learning curve. If you're more familiar with C/C++, feel free to go ahead and implement signatures, and replay protection as we have done in the Python lab.

Here we're focusing on `verify_firmware_hash();` in [security.cpp](../src/security.cpp). Implement this function and also implement `encode()` and `decode()` in [commands.cpp](../src/commands.cpp).

You have already implemented `verify_firmware_hash()` before in the cryptography lab, but implementing serialization and deserialization in `encode()` and `decode()` will certainly be more challenging. We wrote some unit tests so you may verify your solution. 

Some thoughts about `encode()` and `decode()`:
- You are converting a logical, structured format (`struct`) into an array of bytes. Maybe consider straight `memcpy`?
- Honestly the way you serialize is not super important rn, moreso, your decode has to understand your encode.
- Your `encode()` should remember to compute the SHA256 hash and include it in the message!
- The `decode()` should remember to compute the SHA256 hash of the message and compare it to *the hash provided in the message*.
