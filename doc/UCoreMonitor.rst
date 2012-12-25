============
UCoreMonitor
============

Here is the document of UCoreMonitor.

How the UCoreMonitor monitor function
-------------------------------------

The principle behind UCoreMonitor is very simple. Firstly you should know the signal and event programming model in C++. Then you can catch a S2E CorePlugin signal like this:

.. code-block:: c++

    s2e()->getCorePlugin()->onTranslateBlockEnd
      .connect(sigc::mem_fun(*this, &UCoreMonitor::onTranslateBlockEnd));
    //catch call inst
    void UCoreMonitor::onTranslateBlockEnd(ExecutionSignal *signal,
                                         S2EExecutionState *state,
                                         TranslationBlock *tb,
                                         uint64_t pc, bool static_target
                                         , uint64_t target_pc){
       //blah blah
     }

When the signal onTranslateBlockEnd occurs, the UCoreMonitor::onTranslateBlockEnd function will be executed.

By slotting the onTranslateBlockEnd signal, I can execute the UCoreMonitor::onTranslateBlockEnd function every time the program meets x86 *call* intructions, thus we can get the callee's name by reading the symbol map provided by the compiler.

How the UCoreMonitor monitor process
------------------------------------

After we can slot all *call* instructions, we can track the function calling graph. When it comes to monitoring process, we are especially interested in three functions.

::

     #Create the process and assign its name
     set_proc_name()
     #Switch process
     proc_run()
     #Process exits
     proc_exit()

So if we encountered these three functions, we try to parse the UCorePCB data structure from the memory. It is a hard job because we cannot get the accurate size of UCorePCB object and the offset of each field. So I have to define the struct manually in UCorePCB.h.

.. code-block:: c++

    #define PCB_SIZE                    228
    #define PCB_STATE_OFFSET            0
    #define PCB_PID_OFFSET              4
    #define PCB_RUNS_OFFSET             16
    #define PCB_PARENT_OFFSET           28
    #define PCB_NAME_OFFSET             80
    #define PCB_NAME_LEN                16
    #define PCB_LIST_LINK_OFFSET        96

It's an ugly fix but works. Accordingly I have to detect the offset manually when I use a new version of UCore.

So every time I met the *key functions* mentioned above, I will try to parse the UCorePCB object from the VM memory, then print them.

As a demo result, you can see the scheduling and switching process in QEMU, real time.

Signals
-------

This plugin provides the following signals.

.. code-block:: c++

      /* For funtion monitor */
      typedef sigc::signal<void, ExecutionSignal *,
                           S2EExecutionState*,
                           std::string, uint64_t> TransitionSignal;
      TransitionSignal onFunctionCalling;
      /* for thread monitor */
      typedef sigc::signal<void, S2EExecutionState*,
                           UCorePCB*, UCorePCB*,
                           uint64_t> ThreadSwitchSignal;
      typedef sigc::signal<void, S2EExecutionState*,
                           UCorePCB*, uint64_t> ThreadSignal;
      ThreadSwitchSignal onThreadSwitching;
      ThreadSignal onThreadCreating;
      ThreadSignal onThreadExiting;


