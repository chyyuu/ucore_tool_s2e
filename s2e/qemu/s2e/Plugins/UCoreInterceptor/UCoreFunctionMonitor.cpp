extern "C" {
#include "config.h"
#include "qemu-common.h"
}

#include "UCoreFunctionMonitor.h"

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

using namespace std;
using namespace s2e;
using namespace s2e::plugins;

S2E_DEFINE_PLUGIN(UCoreFunctionMonitor,
                  "Monitoring Function Calling of UCore",
                  "", "FunctionMonitor");

UCoreFunctionMonitor::~UCoreFunctionMonitor(){
}

void UCoreFunctionMonitor::initialize(){

  //for default mode
  s2e()->getCorePlugin()->onTranslateBlockEnd
    .connect(sigc::mem_fun(*this, &UCoreFunctionMonitor::slotTBEnd));

  //for slow mode
  m_SlowMode = false;
  if(m_SlowMode){
    m_registered = false;
    m_monitor = static_cast<FunctionMonitor*>
      (s2e()->getPlugin("FunctionMonitor"));
    s2e()->getCorePlugin()
      ->onTranslateBlockStart.connect
      (sigc::mem_fun(*this, &UCoreFunctionMonitor::slotTBStart));
  }

}

/*--------------------- Default Mode Code --------------------*/

void UCoreFunctionMonitor::slotTBEnd(ExecutionSignal *signal,
                                       S2EExecutionState *state,
                                       TranslationBlock *tb,
                                       uint64_t pc, bool static_target
                                     , uint64_t target_pc){
  if(tb->s2e_tb_type == TB_CALL || tb->s2e_tb_type == TB_CALL_IND){
    signal->connect(sigc::mem_fun(*this,
                                  &UCoreFunctionMonitor::slotCallInst));
  }
}

void UCoreFunctionMonitor::slotCallInst(S2EExecutionState *state,
                                        uint64_t pc){
  onFunCall.emit(state, pc);
}


/*--------------------- Slow Mode Code --------------------*/

void UCoreFunctionMonitor::slotTBStart(ExecutionSignal *sig,
                                       S2EExecutionState *state,
                                       TranslationBlock *tb,
                                       uint64_t pc){
  if(!m_registered){
    m_registered = true;
    return;
  }

  FunctionMonitor::CallSignal *callSignal;
  callSignal = m_monitor->getCallSignal(state, -1, -1);
  callSignal
    ->connect(sigc::mem_fun(*this,
                            &UCoreFunctionMonitor::FunCallMonitor));
}

void UCoreFunctionMonitor::FunCallMonitor(S2EExecutionState *state,
                                          FunctionMonitorState *fns){
  //s2e()->getWarningsStream() << "Enterning\n";
  onFunCallSlowMode.emit(state, state->getPc());
  FUNCMON_REGISTER_RETURN(state, fns,
                          UCoreFunctionMonitor::FunRetMonitor);
}

void UCoreFunctionMonitor::FunRetMonitor(S2EExecutionState *state){
  //s2e()->getWarningsStream() << "Exiting\n";
  onFunRetSlowMode.emit(state, state->getPc());
}

