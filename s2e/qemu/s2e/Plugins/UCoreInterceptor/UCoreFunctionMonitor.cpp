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
  m_registered = false;
  m_monitor = static_cast<FunctionMonitor*>
    (s2e()->getPlugin("FunctionMonitor"));
  s2e()->getCorePlugin()
    ->onTranslateBlockStart.connect
    (sigc::mem_fun(*this, &UCoreFunctionMonitor::slotTBStart));
}

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
  onFunCall.emit(state, state->getPc());
  FUNCMON_REGISTER_RETURN(state, fns,
                          UCoreFunctionMonitor::FunRetMonitor);
}

void UCoreFunctionMonitor::FunRetMonitor(S2EExecutionState *state){
  onFunRet.emit(state, state->getPc());
}

