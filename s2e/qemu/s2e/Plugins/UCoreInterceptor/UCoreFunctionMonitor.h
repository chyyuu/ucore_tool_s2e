#ifndef _UCORE_FUNCMONITOR_H
#define _UCORE_FUNCMONITOR_H

#include "UCoreMonitor.h"
#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

#include <string>

namespace s2e{
  namespace plugins{
    class UCoreFunctionMonitor : public Plugin{

      S2E_PLUGIN

      public:
      UCoreFunctionMonitor(S2E *s2e) : Plugin(s2e){}
      virtual ~UCoreFunctionMonitor();

    public:
      void initialize();
      void slotFuncCalling(ExecutionSignal* executionSignal,
                           S2EExecutionState* state,
                           std::string funcName,
                           uint64_t pc);
      void slotFunctionExecution(S2EExecutionState *state,
                                 uint64_t pc);
      void printCallDetail();
    private:
      std::string m_MonitorFuncName;
      bool first;
    };
  }
}

#endif
