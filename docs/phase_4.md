# Phase 4: Timing Attack

We're going to do something quite different now.. we're gonna look at an example attack to emphasize why we should think deeply about not only what we're doing on the board but also *how* we're doing it.

In the file [src/utilities.cpp](../src/utilities.cpp), there are two definitions for a basic primitive we do a lot: just checking if two arrays are equal! One of them is insecure (see "Caveats" in the `memcmp` [man page](https://man7.org/linux/man-pages/man3/memcmp.3.html)).

This is a pretty fundamental thing of course, to check passwords, permissions, etc. We're going to run a timing attack on this function (`auth_check` in [src/uart.cpp](../src/uart.cpp)) to see why we would prefer one implementation over the other.

## Instructions
1. Read through `time_attack.py`. Make sure you know what it's doing.
2. Run the script to see the time_attack succeed.
3. Try changing the `memcmp` to the constant time alternative in [src/utilities.cpp](../src/utilities.cpp) and determine if the attack still works.