========================
UCoreUtils & UCoreStruct
========================

UCoreUtils provides miscellaneous functions used in other Plugins.

UCoreStruct defines the UCore OS-level data structures needed in the Plugins.

Introduction
============

We need many meta information to parse the UCore OS data structure.
Up to now, UCore Utils supports the following features.

* Parse the detail of an instruction based on its address.
* Parse the detail of a PCB based on its address.

To implement these two functions, we need to do a lot of work, including:

* Parse the Symbol Table and provide the map between Kernel Address and the Symbol String.
* Parse the Stab debug segment in the Kernel ELF file.
* Hard Code the UCorePCB structure size and offset.

How UCoreUtils Works?
=====================

Parse Symbol Table
------------------

Symbol Table is the mose fundamental thing in this series of plugin. The file actually looks like this.

::

         #Symbol Name   #Address
         kern_init      c1000000

This file is created by the *sed*, *objdump* tool and the kernel ELF image. You can refer the UCore Lab Makefile to see how it is created.

In *config.lua*, the location of the symbol table file is specified and UCoreUtils read the file when the S2E is initialized.

Finally the Symbol Table is stored into the memory as two *std::map* variables. One is the Addr2Sym map, and the other is Sym2Addr map.

Parse Stab Segment
------------------

*Stabs* is a kind of debug format from FreeBSD. Here is the `reference <http://docs.freebsd.org/info/stabs/stabs.pdf>`_ if you want to dig into it.

In UCore, we also provides the Stabs Segment to help debug by adding *-g* flag in the gcc compiler. These debug information are used when the *debug_monitor* function is called.

In S2E, we can also parse the Stabs Segment. Firstly we have to read this segment into memory and store them as UCoreStab Array. Then we can do the binary search and find the needed item when we want to parse the detailed information of a instruction. The only thing we need is the value in *EIP* register when we encountered the target instruction.

Parse UCorePCB
--------------

It is a little complex this time.

Parse UCoreInst
---------------

Read the loaded Stabs segment and do the binary search to find the instruction information.
Finally we will create an UCoreInst object and fill it with debug information.
You should be quite familiar to these things if you do the UCore Lab1.

How UCoreStruct Works?
======================

Define UCoreInst
----------------

Just provides struct UCoreInst, including the instruction information parsed from Stabs segment.

Define UCorePCB
---------------

Hard code the essential field offsets and the size of UCore Process Control Block.
Furthermore, provides the UCorePCB struct to provide more detailed Process information.

Define UCoreStab
----------------

We need to accuratly define the size of the Stabs item. The size is defined in UCoreStruct.h.

Now it just copies the definition of Stabs segment item in UCore.
Note that we still need to copy many *define* macros in UCore header file to use the Stabs debug information.
