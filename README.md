# DTS Studio

### What is this?

This project is mostly a playground to explore better ways to reverse engineer proprietary ICs, SoC hardware accelerators or IPs.

Oftentimes manufacturers don't, or just barely, document how their parts work and simply provide, oftentimes very convoluted, Linux drivers. This approach makes expanding upon their work extremely difficult.

This project's goal is to provide a simple way to iteratively implement hardware peripherals using a scripting language and then testing them by running a emulated RISC-V Linux Kernel on them; reducing potentially multiple interconnected Kernel drivers that span multiple execution environments down to their bare minimum register accesses.

### Structure

- `frontend` => The frontend of the project, implemented using React + Shadcn
- `main` => The Tauri application that creates the window and acts as an IPC interface to the frontend
- `lib`
  - `emulator` => Implementation of the RISC-V emulator
  - `interface` => Main application logic