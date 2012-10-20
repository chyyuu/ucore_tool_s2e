# Tutorial for ucore_tool_s2e

## Required Packages

    $ sudo apt-get install build-essential
    $ sudo apt-get install subversion
    $ sudo apt-get install git
    $ sudo apt-get install qemu
    $ sudo apt-get install liblua5.1-dev
    $ sudo apt-get install libsigc++-2.0-dev
    $ sudo apt-get install binutils-dev
    $ sudo apt-get install python-docutils
    $ sudo apt-get install python-pygments
    $ sudo apt-get install nasm
    $ sudo apt-get build-dep llvm-2.7
    $ sudo apt-get build-dep qemu

## How to compile

Run the following commands under Linux.

    $ git clone https://github.com/chyyuu/ucore_tool_s2e.git
    $ cd ucore_tool_s2e
    $ mkdir build
    $ cd build
    $ ln -s ../s2e/Makefile .
    $ make

Have some coffee and wait for the finish of the compilation.


## Directory Tree

Here is the code directory tree of my s2e project.

    ucore_tool_s2e/
    | ./s2e		// source code
    | ./build		// binary
    | ./test		// test dir
    | ----./test/lab4		// ucore OS being tested
    | ./doc		// documentation
    | ./wiki		// wiki
    | ./config		// config files

## How to solve compile errors 

 * MiniSat/Solve error

Just run "make" command again.

    $ make

 * SDL error

Modify the files' contents like:

    include <SDL.h>

to

    include <SDL/SDL.h>

 * unable to open UCoreMonitor.d/.o

Run the following commands under ucore_tool_s2e directory,

    $ cd build/qemu-release/i386-s2e-softmmu/s2e/Plugins  
    $ mkdir UCoreInterceptor

then run "make" command under ucore_tool_s2e/build directory.

    $ make

## modify ucore's Makefile

 * Find the line start with "QEMUOPTS",

    QEMUOPTS = -hda $(UCOREIMG) -drive file=$(SWAPIMG),media=disk,cache=writeback -drive file=$(SFSIMG),media=disk,cache=writeback 

    .PHONY: qemu qemu-nox debug debug-nox monitor
    qemu: $(UCOREIMG) $(SWAPIMG) $(SFSIMG)
	$(V)$(QEMU) -parallel stdio $(QEMUOPTS) -serial null

    qemu-nox: $(UCOREIMG) $(SWAPIMG) $(SFSIMG)
	$(V)$(QEMU) -serial mon:stdio $(QEMUOPTS) -nographic

    monitor: $(UCOREIMG) $(SWAPING) $(SFSIMG)
	$(V)$(QEMU) -monitor stdio $(QEMUOPTS) -serial null

 * Copy these lines like the follow ones and use your_own_path in "S2E" and "-s2e-config-file".

    S2E :=	/home/fwl/ucore/ucore_tool_s2e/build/qemu-release/i386-s2e-softmmu/qemu-system-i386

    S2EOPTS = -hda $(UCOREIMG) -drive file=$(SWAPIMG),media=disk,cache=writeback -drive file=$(SFSIMG),media=disk,cache=writeback \
		-s2e-config-file /home/fwl/ucore/ucore_tool_s2e/config/ucoreconfig.lua -s2e-verbose

    s2e: $(UCOREIMG) $(SWAPIMG) $(SFSIMG)
	$(V)$(S2E) -parallel stdio $(S2EOPTS) -serial null

## modify s2e config file

Ucore_tool_s2e use the file "kernel.sym" which is compiled from ucore.

 * Make ucore lab(suppose the directory tree as default),

    $ cd test/lab4  
    $ make

then you have "kernel.sym" file in the "obj" directory.

 * Modify the path with your_own_path in config file,

## Use ucore_tool_s2e

Run the following commands under ucore_tool_s2e directory.

    $ cd test/lab4  
    $ make s2e

The output files will be in "s2e-last" directory.
