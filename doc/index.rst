=======================
The S2E Tools For UCore
=======================

.. contents::

If you have any questions, feel free to email to: nuklly@gmail.com

Online Reference
================

* Web Page Reference
  1. `Official S2E Documentation<https://s2e.epfl.ch/embedded/s2e/>`_
  2. `Nuk's Wiki Page in OS Course, 2012<http://os.cs.tsinghua.edu.cn/oscourse/OS2012/projects/U03>`_

* Source Code
  1. `Project UCore Tool S2E<https://github.com/chyyuu/ucore_tool_s2e>`_
  2. `S2E Version1.2<https://s2e.epfl.ch/attachments/download/209/s2e-source-1.2-27.04.2012.tar.bz2>`_

Plugin Reference
================

UCoreMonitor
----------------

UCoreMonitor is the basic plugin monitoring the behaviour of UCore OS.
It has the following functionalities.

* Monitor the calling of the functions.
* Parse the Stab segment in the binary and get debug information.
* Monitor the initing, switching and exitting of processes.
* Parse UCore process PCB and list the running processes in the QEMU monitor.
* Monitor Kernel Panic and give out debug information when the panic occurs.

UCoreFunctionMonitor
--------------------

UCoreFunctionMonitor is another plugin works based on the UCoreMonitor.
It has the following functionalities.

* Monitor the target function(name specified in config.lua) behaviour.
* Print the debug info into standard output as well as log files.
