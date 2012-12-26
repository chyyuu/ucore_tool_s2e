====================
UCoreFunctionMonitor
====================

UCoreFunctionMonitor is the plugin monitoring the behaviour of UCore Function Calls.

Introduction
============

The UCoreFunctionMonitor has two modes: Fast-mode(default) and Slow-mode.

In Fast-mode, I only catch the X86 *Call* instructions and get the function name from the System Map provided by the UCoreUtils.

In Slow-mode, I use the S2E provided FunctionMonitor Plugin to monitor a function whose name is specified by the *config.lua*. In this mode we can track both the call and the return behaviour of the function. However, this method could be quite slow. So I cannot use it to track all functions like Fast-mode.

The slow-mode can only be used when you want to track a specific function.

How It Works?
=============

Fast-mode
---------

In the Fast-mode I catch the onTranslateBlockEnd signal and check if there is a *Call* instruction at the end of the TranslateBlock. If so, emit a FunCall signal which provides the *S2EExecutionState* pointer and *pc* value.

In this mode, we only track the calling of all functions in UCore.

Slow-mode
---------

In this mode I use the FunctionMonitor plugin. For its usage, please refer to: `Function Monitor Reference <https://s2e.epfl.ch/embedded/s2e/Plugins/FunctionMonitor.html>`_

In this mode, we only track the calling and returning of a specific function in UCore.
