===================
QEMUMonitorEnhancer
===================

I provide the QEMU Monitor enhancement feature to help the debug of UCore.

Introduction
============

QEMU Emulator has an useful monitor mode, which can display the value of registers, the memory and so on. I enhanced this monitor and now it can print all processes the UCore has. I name the command print_all_threads.

You can redirect the monitor to stdio by adding the *-monitor stdio* flag into QEMU/S2E command line.

How To Do this?
===============

For S2E version 1.2, it is based on QEMU v1.0.

Follow the instructions below and you can customize your own QEMU monitor enhancer.

1. In *hmp.h*, declare your own enhance function.
2. In *hmp.c*, define the function and call the related function in S2E.
3. In *hmp-commands.hx*, write your own command instructions for the enhancer..
4. In *QEMUMonitorEnhancer.h*, declare the functions called in *hmp.c*.
5. In *UCoreProcessMonitor.h*, include the *QEMUMonitorEnhancer.h* as *extern "C"* and implement the function declared in the *QEMUMonitorEnhancer.h*.
6. Run *Make* and check if the command successeded in the monitor.
