# IRIS Kernel  
> <b>I</b>rgendein <b>RI</b>SC <b>S</b>ystem

IRIS is my attempt at building a modular microkernel for RISC-V.
Right now, this repository mainly serves as a backup for my work - but if you're interested in contributing, I'd be happy to see improvements or ideas!

## Goals
My main goal is to keep as much logic as possible out of kernel mode. That means:
- No drivers in the kernel
- No APIs or ABIs implemented in the kernel
- No filesystem logic in the kernel

with some exceptions like the root FS which will be needed to load the real filesystem.
The kernel itself should only provide enough to load these services.

## Targets
The kernel is currently being built for 64-bit RISC-V (RV64) with the goal of later expanding to support 128-bit RISC-V when it solidifies.
It also currently relies on OpenSBI for initialization.

## State
Currently theres not much to see except memory detection and mapping.