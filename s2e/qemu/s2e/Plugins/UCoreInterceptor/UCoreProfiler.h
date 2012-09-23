#ifndef _UCORE_PROFILER
#define _UCORE_PROFILER

#include "UCorePCB.h"
#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

namespace s2e{
  namespace plugins{
    class UCoreProfiler{
      S2E_PLUGIN
      public:

      private:
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
