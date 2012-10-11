# UCore Tool S2E

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
    | ./s2e                         // source code
    | ./build                       // binary
    | ./test                        // test dir
    | ----./test/lab4              // OS being tested
    | ./doc                         //documentation
    | ./wiki                        //wiki