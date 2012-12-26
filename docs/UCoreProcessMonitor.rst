===================
UCoreProcessMonitor
===================

Introduction
============

The UCoreProcessMonitor can track the Init, Switch and Exit behaviour of a UCore Process.

How it works?
=============

Catch Signal: CallFunc
----------------------

The CallFunc signal is emitted by the UCoreFunctionMonitor Plugin.
It will retrieve the pc of the call target from the S2EExecutionState and search it in the Addr2Sym map provided by the UCoreUtils.
Then run the different functions according to the retrieved function name.

Note: the *pc* in the argument is the address of the call instruction, not the target of call instruction.

Process Init
------------

Track the *proc_init* function. Retrieve the inited proc PCB from the argument list and call parseUCorePCB function in UCoreUtils.

Process Switch
--------------

Track the *proc_run* function.
Retrieve the previous and next PCB from the *current* global variable and argument list, respectively. Then parse UCorePCB and emit the signal.

Process Exit
------------

Track the *do_exit* function.
Retrieve the target PCB from the global variable *current* and call pareUCorePCB.

