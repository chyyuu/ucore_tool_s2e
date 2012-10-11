#ifndef _UCORE_PROFILER_H
#define _UCORE_PROFILER_H

#include "UCorePCB.h"
#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>
#include <map>

namespace s2e{
  namespace plugins{
    class UCoreProfiler : public Plugin{
      S2E_PLUGIN
      public:
      UCoreProfiler(S2E *s2e): Plugin(s2e){}
      void initialize();
      //signal slots
      void slotThreadSwitch(ExecutionSignal* signal,
                          S2EExecutionState* state,
                          UCorePCB* prev,
                          UCorePCB* next,
                          uint64_t pc);
      void slotThreadCreation(ExecutionSignal* signal,
                            S2EExecutionState* state,
                            UCorePCB* newThread,
                            uint64_t pc);
      void slotThreadExit(ExecutionSignal* signal,
                        S2EExecutionState* state,
                        UCorePCB* newThread,
                        uint64_t pc);
      private:
      UCorePCB* current;
      std::map<uint32_t, UCorePCB*> threadMap;
      void disconnect(S2EExecutionState *state){
        return;
      }
    };
    class UCoreProfilerState: public PluginState{
    private:
      uint64_t m_CurrentPid;
    public:
      UCoreProfilerState();
      virtual ~UCoreProfilerState();
      virtual UCoreProfilerState* clone() const;
      static PluginState *factory(Plugin *p,
                                  S2EExecutionState *state);
      friend class UCoreProfiler;
    };
  }
}

#endif
