/**
   UCore Function Monitor
   Monitor function calling behaviour in UCore
   By Nuk, Tsinghua University
 */
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
                  "Monitor of Functions in UCore OS",
                  "UCoreFunctionMonitor", "UCoreMonitor");

UCoreFunctionMonitor::~UCoreFunctionMonitor(){
}

void UCoreFunctionMonitor::initialize(){
  first = true;
  m_MonitorFuncName = s2e()->getConfig()
    ->getString(getConfigKey() +
                ".FunctionName");
  ((UCoreMonitor*)s2e()->getPlugin("UCoreMonitor"))
    ->onFunctionCalling.connect(sigc::mem_fun(*this,
                                              &UCoreFunctionMonitor::
                                              slotFuncCalling));
}

void UCoreFunctionMonitor::slotFuncCalling(ExecutionSignal* executionSignal,
                                           S2EExecutionState* state,
                                           string funcName, uint64_t pc)
{
  if(funcName == m_MonitorFuncName){
    executionSignal->connect(sigc::mem_fun(*this,
                                          &UCoreFunctionMonitor::
                                          slotFunctionExecution));
  }
}

void UCoreFunctionMonitor::slotFunctionExecution(S2EExecutionState *state,
                                                 uint64_t pc)
{
  if(first){
    UCoreFunc* func = new UCoreFunc;
    ((UCoreMonitor*)s2e()->getPlugin("UCoreMonitor"))
      ->parseUCoreFunc(pc, func);
    first = false;
  }
  s2e()->getWarningsStream() << "I'm here.\n";
}

void UCoreFunctionMonitor::printCallDetail()
{
}

