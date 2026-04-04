# My OS Project

Built during Lock-In week 1 of FlavorTown. No proper name yet so the URL might change at some point.

## What it does
 
- Boots via the **Limine bootloader** and splashes a big **OS** in the middle of your screen
- Shows the current time and timezone (UTC by default, change the `timer_set_timezone()` call in `kernel.c` if you want)
- Has a working **shell** with basic commands
- Dynamically loads and runs **userspace programs** — the kernel literally scans an ELF section for registered programs and just runs them, which honestly took way too long to figure out but is extremely cool
- Comes with a few programs out of the box:
  - `drawing` — move a cursor around and draw pixels on screen
  - `snake` — fully playable snake with a rainbow-colored snake because why not
  - `music` — plays songs through the PC speaker with a lot of songs included + a playable piano/keyboard (Songs thanks to the people who helped write songs for the API)
  - `info` — system info screen (memory, screen size, etc.)
- Detects your GPU via a PCI bus scan (Intel, AMD, NVIDIA, VMware, VirtIO) and knows the vendor, device ID, and VRAM size — but doesn't actually *use* it. All rendering goes straight through the Limine framebuffer. The GPU driver stubs (`gpu_memcpy_to_vram` etc.) are there and ready, just not hooked up yet. It's more of a "I know you exist,but fuck you" situation. This will be most likely added in the next update.
 
## Shell commands (non-programs)

```
cls          clear the screen
clock        show current time
help         list commands
reboot       reboot the machine
shutdown     shut it down
exit         halt the CPU (rip)
<program>    run any userspace program by name
```

## Architecture (roughly)

```
kernel/          kernel entry point, framebuffer init, RTC clock, boots into init
libraries/       draw, font, keyboard, timer, speaker, power — no libc, all handwritten
userspace/       shell + programs, each registered via a linker section so init can find them
limine/          bootloader blobs (don't touch)
```

The userspace loader works by putting every program struct into a custom `.userspace_programs` ELF section. At runtime `init` scans from `__start_userspace_programs` to `__stop_userspace_programs`, matches the name, runs the test function, and launches it. Biggest breakthrough of the whole project tbh.

## How to build and run

Not really recommending you do this right now, but if you insist:

Clone the repo and `cd` into it. You need to be on Linux, the dependencies (`x86_64-linux-gnu-gcc`, `xorriso`, `qemu-system-x86_64`) should already be installed on most setups.

```bash
make clean && make && make run
```

This compiles everything, creates `OS.iso`, and launches it in QEMU with VirtIO GPU and PC speaker audio. That's it! It has real VRAM detection!

---

Locking back in now. More stuff coming soon probably.
Cya

- Me in week 1 devlog 3
