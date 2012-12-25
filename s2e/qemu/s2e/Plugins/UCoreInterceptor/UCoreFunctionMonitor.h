#ifndef _UCORE_MONITOR_H
#define _UCORE_MONITOR_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/Plugins/FunctionMonitor.h>
#include <s2e/S2EExecutionState.h>

namespace s2e{
  namespace plugins{
    class UCoreFunctionMonitor : public Plugin{
      S2E_PLUGIN
      public:
      UCoreFunctionMonitor(S2E *s2e) : Plugin(s2e){}
      virtual ~UCoreFunctionMonitor();
      void initialize();
    public:
      /*--------------- public signals -------------*/
      typedef sigc::signal<void, S2EExecutionState*,
                           uint64_t> FunMonSignal;
      FunMonSignal onFunCall;
      FunMonSignal onFunCallSlowMode;
      FunMonSignal onFunRetSlowMode;
    private:
      /*-------------- Default Mode -------------*/
      void slotTBEnd(ExecutionSignal* signal, S2EExecutionState *state,
                   TranslationBlock *tb, uint64_t pc,
                   bool, uint64_t);
      void slotCallInst(S2EExecutionState *state,
                        uint64_t pc);
      /*-------------- Slow Mode ----------------*/
      void slotTBStart(ExecutionSignal *sig,
                       S2EExecutionState *state,
                       TranslationBlock *tb,
                       uint64_t pc);
      void FunCallMonitor(S2EExecutionState *state,
                          FunctionMonitorState *fns);
      void FunRetMonitor(S2EExecutionState *state);
      bool m_SlowMode;
      bool m_registered;
      FunctionMonitor *m_monitor;

    }; //class UCFM
  } // plugins
} // s2e

#endif
