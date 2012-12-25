#ifndef _UCORE_PROC_MONITOR_H
#define _UCORE_PROC_MONITOR_H

#include "UCoreUtils.h"

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

namespace s2e{
  namespace plugins{
    class UCoreProcessMonitor : public Plugin{
      S2E_PLUGIN
      public:
      UCoreProcessMonitor(S2E *s2e) :Plugin(s2e){}
      virtual ~UCoreProcessMonitor();
      void initialize();
    public:
      /*--------------signals-------------*/
      typedef sigc::signal<void, S2EExecutionState*,
                           UCorePCB*, UCorePCB*,
                           uint64_t> ThreadSwitchSignal;
      typedef sigc::signal<void, S2EExecutionState*,
                           UCorePCB*, uint64_t> ThreadSignal;
      ThreadSwitchSignal onThreadSwitching;
      ThreadSignal onThreadCreating;
      ThreadSignal onThreadExiting;
      void slotFunCall(S2EExecutionState *state, uint64_t pc);

    private:
      void slotProcInit();
      void slotProcSwitch();
      void slotProcExit();
      UCoreUtils* utils;
    };
  }
}

#endif
