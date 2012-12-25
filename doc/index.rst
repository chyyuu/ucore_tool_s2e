=======================
The S2E Tools For UCore
=======================

.. contents::

Welcome to visit the documentation for UCore Tool S2E

If you have any questions, feel free to email to: nuklly@gmail.com

Online Reference
================

* Web Page Reference

1. `Official S2E Documentation <https://s2e.epfl.ch/embedded/s2e/>`_
2. `Nuk's Wiki Page in OS Course, 2012 <http://os.cs.tsinghua.edu.cn/oscourse/OS2012/projects/U03>`_

* Source Code

1. `Project UCore Tool S2E <https://github.com/chyyuu/ucore_tool_s2e>`_
2. `S2E Version1.2 <https://s2e.epfl.ch/attachments/download/209/s2e-source-1.2-27.04.2012.tar.bz2>`_

Plugin Documentation
====================

UCoreFunctionMonitor
--------------------

`UCoreFunctionMonitor <UCoreMonitor.rst>`_ is the basic plugin monitoring the Function of UCore OS.

It has the following functionalities.

* Monitor the calling of the functions.(Speed: Quick)
* Monitor the calling and returning behavious of the specific function.(Speed: Slow)
* Print the specific function detail using Stab information.

UCoreProcessMonitor
--------------------

`UCoreProcessMonitor <UCoreProcessMonitor.rst>`_ is another plugin works based on the UCoreMonitor.
It monitors the process switching behaviour of UCore OS.

It has the following functionalities.

* Monitor the Process Switching behavious of UCore.
* Retrieve the UCorePCB struct to get detail of a specific process.
* List all processes exists in the running kernel.

UCoreStruct
-----------

`UCoreStruct <UCoreStruct.rst>`_ is the abstract layer of UCore data structures in S2E.

It defines the following data structures.

* UCoreInst: including the source code file location, function entry and line number info.
* UCorePCB: including the process's pid, name and state info.
* UCoreStab: including an item in the Stab debug information list.

UCoreUtils
----------

`UCoreUtils <UCoreUtils.rst>`_ is a series of meta functions used in all plugins.

It includes following functions.

* Parse UCoreStab Segment to get debug information.
* Parse UCorePCB object from the given address.
* Parse UCoreInst object based on the given address and the UCore Stab debug information.
* Parse the Symbol Table to add symbol-address map support.

QEMUMonitorEnhancer
-------------------

`QEMUMonitorEnhancer <QEMUMonitorEnhancer.rst>`_ is a plugin supporting the enhance of QEMU monitor.
It now supports the following commands.

* print_all_threads: Print detail of all running threads in UCore and indicate the current one.

UCoreMemoryManagement
---------------------

`UCoreMemoryManagement <UCoreMemoryManagement.rst>`_ is a plugin monitoring the UCore memory usage. It needs UCoreMonitor to work.

It has the following functionalities.

* Monitor the page allocate and free behaviours.
* Print the page table and memory map.

