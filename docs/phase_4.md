# Phase 4: Timing Attack

We're going to do something quite different now.. we're gonna look at an example attack to emphasize why we should think deeply about not only what we're doing on the board but also *how* we're doing it.

In the file [src/utilities.cpp](../src/utilities.cpp), there are two definitions for a basic primitive we do a lot: just checking if two arrays are equal!

This is a pretty fundamental thing of course, to check passwords, permissions, etc. We're going to run a timing attack on this function to see why we would prefer one implementation over the other.

// some python commands ig, some setup

// todo: write the python server code, write a new command on the uav that needs to accept and check a pin

// maybe part of the assignment is to just write this new command lol