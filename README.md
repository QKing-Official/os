# My OS Project

I’m working on a personal OS project for Lock-In week 1 of FlavorTown! 
I don’t have a proper name for it yet so url can change.

## What it does

- Boots up and shows a simple screen with **OS**  
- Displays the current time and timezone  
- Can run small userspace programs  
- Includes a test program called `drawing`  
- Uses the real-time clock for proper delays and time

## How to use

I do not recommend you do at this moment, but sure thing.
Clone the repo and cd into it. Make sure you are on linux, the dependecies should be preinstalled.
You can build and run it in QEMU:

```bash
make clean && make && make run
```
It will create an ISO and launch it in a virtual machine!

Enjoy! I will continue locking in now!
