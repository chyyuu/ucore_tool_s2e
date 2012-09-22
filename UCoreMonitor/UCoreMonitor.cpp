/**
   UCore Monitor
   By Nuk, Tsinghua University.
*/

extern "C" {
#include "config.h"
#include "qemu-common.h"
// #include "./UCoreHeaders/proc.h"
}

#include "UCoreMonitor.h"
// #include "UCoreUserModeEvent.h"
// #include "UCoreKernelModeEvent.h"

#include <iostream>
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

using namespace std;
using namespace s2e;
using namespace s2e::plugins;

S2E_DEFINE_PLUGIN(UCoreMonitor, "Monitor of UCore OS", "Interceptor", );

uint64_t UCoreMonitor::s_KeInitThread = 0xc010bfcb;
uint64_t UCoreMonitor::s_KeTerminateThread = 0xc010c0f9;

UCoreMonitor::~UCoreMonitor(){
  // if(m_UserModeEvent){
  //   delete m_UCoreUserModeEvent;
  // }
  // if(m_KernelModeEvent){
  //   delete m_UCoreKernelModeEvent;
  // }
}

void UCoreMonitor::initialize(){
  //Read kernel Version Address

  m_KernelBase = s2e()->getConfig()->getInt(getConfigKey() + ".kernelBase");
  m_KernelEnd = s2e()->getConfig()->getInt(getConfigKey() + ".kernelEnd");

  //parse system map file
  bool ok;
  system_map_file = s2e()->getConfig()->getString(getConfigKey() + ".system_map_file", "", &ok);
  if(!ok){
    s2e()->getWarningsStream() << "No System.map file provided. System.map is needed for UCoreMonitor to work properly. Quit.\n";
    exit(-1);
  }
  parseSystemMapFile();

  //User Mode Event and Kernel Mode Event
  m_UserMode = s2e()->getConfig()->getBool(getConfigKey() + ".userMode");
  m_KernelMode = s2e()->getConfig()->getBool(getConfigKey() + ".kernelMode");
  m_MonitorThreads = s2e()->getConfig()->getBool(getConfigKey() + ".monitorThreads");
  // if(m_UserMode){
  //   m_UCoreUserModeEvent = new UCoreUserModeEvent(this);
  // }
  // if(m_KernelMode){
  //   m_KernelModeEvent = new UCoreKernelModeEvent(this);
  // }

  //connect Signals
  s2e()->getCorePlugin()->onTranslateBlockEnd
    .connect(sigc::mem_fun(*this, &UCoreMonitor::onTranslateBlockEnd));
  s2e()->getCorePlugin()->onTranslateJumpStart
    .connect(sigc::mem_fun(*this, &UCoreMonitor::onTBJumpStart));
  s2e()->getCorePlugin()->onTranslateInstructionStart
    .connect(sigc::mem_fun(*this, &UCoreMonitor::onTranslateInstruction));
}

void UCoreMonitor::onTranslateInstruction(ExecutionSignal *signal,
                                          S2EExecutionState *state,
                                          TranslationBlock *tb,
                                          uint64_t pc){
  // if(m_UserMode){
  // }

  if(m_KernelMode){
    if(m_MonitorThreads && pc == getKeInitThread()){
      signal->connect(sigc::mem_fun(*this,
                                    &UCoreMonitor::slotKmThreadInit));
    }else if(m_MonitorThreads && pc == getKeTerminateThread()){
      signal->connect(sigc::mem_fun(*this,
                                    &UCoreMonitor::slotKmThreadExit));
    }
  }
}

//emit Signal
void UCoreMonitor::onTranslateBlockEnd(ExecutionSignal *signal,
                                       S2EExecutionState *state,
                                       TranslationBlock *tb,
                                       uint64_t pc, bool, uint64_t){
  if(pc >= getKernelStart()){
    if(tb->s2e_tb_type == TB_CALL || tb->s2e_tb_type == TB_CALL_IND){
      signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotCall));
    }
  }
}
void UCoreMonitor::slotCall(S2EExecutionState *state, uint64_t pc){
  uint64_t callAddr;
  callAddr = state->getEip();
  // for debug
  //s2e()->getDebugStream() << "slot call @ " << ret_addr << " Entering:" << sTable[final_addr].name << "\n";
  onFunctionTransition.emit(state, sTable[callAddr].name, pc);
  callStack.push_back(callAddr);
}

void UCoreMonitor::onTBJumpStart (ExecutionSignal *signal, S2EExecutionState *state,
                                  TranslationBlock *tb, uint64_t, int jump_type){
  if(state->getPc() >= getKernelStart()){
    if(jump_type == JT_RET || jump_type == JT_LRET){
      signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotRet));
    }
  }
}
void UCoreMonitor::slotRet(S2EExecutionState *state, uint64_t pc){
  if(callStack.size() == 0)
    return;
  uint64_t func = callStack[callStack.size() - 1];
  // for debug
  // s2e()->getDebugStream() << "slot ret @ " << std::hex << ret_addr << " Exiting: " << sTable[func].name << "\n";
  onFunctionTransition.emit(state, sTable[func].name, pc);
  callStack.pop_back();
}

// Kernel Events

uint64_t UCoreMonitor::getKernelStart() const {
  return m_KernelBase;
}

uint64_t UCoreMonitor::getKeInitThread() const {
  return s_KeInitThread;
}

uint64_t UCoreMonitor::getKeTerminateThread() const {
  return s_KeTerminateThread;
}

void UCoreMonitor::slotKmThreadInit(S2EExecutionState *state, uint64_t pc) {
  s2e()->getDebugStream() << "UCoreMonitor: creating kernel-mode thread ";
  // uint32_t pThread;
  //TODO: Get pThread, the pointer of current UCOREPCB
  // UCoreThreadDescriptor threadDescriptor;
  // bool res = getThreadDescriptor(state, pThread, threadDescriptor);
  // if(!res){
    // return;
  // }
  // onThreadCreate.emit(state, threadDescriptor);
}
void UCoreMonitor::slotKmThreadExit(S2EExecutionState *state, uint64_t pc) {
  // uint64_t pThread = getCurrentThread(state);
  // UCoreThreadDescriptor threadDescriptor;
  // bool res = getThreadDescriptor(state, pThread, threadDescriptor);
  // if(!res){
  //   return;
  // }
  // s2e()->getDebugStream() << "UCoreMonitor: terminating kernel-mode thread";
  // onThreadExit.emit(state, threadDescriptor);
}
// uint64_t UCoreMonitor::getCurrentThread(S2EExecutionState *state){
// }
// bool UCoreMonitor::getThreadDescriptor(S2EExecutionState* state,
//                                        uint64_t pThread,
//                                        UCoreThreadDescriptor threadDescriptor){
//   uint64_t base = 0, size = 0;
//   if(!getThreadStack(state, pThread, &base, &size)){
//     return false;
//   }
//   threadDescriptor.KernelMode = true;
//   threadDescriptor.KernelStackBottom = base;
//   threadDescriptor.KernelStackSize = size;
//   return true;
// }
// bool UCoreMonitor::getThreadStack(S2EExecutionState* state,
//                                   uint64_t pThread,
//                                   uint64_t* base,
//                                   uint64_t* size){
//   if(!isKernelAddress(state->getPc())){
//     assert(false && "User-mode stack retrieval not implemented.");
//   }
//   //Get the PCB from given pointer pThread
//   s2e::ucore::UCOREKTHREAD kThread;
//   if(!state->readMemoryConcrete(pThread, &kThread, sizeof(kThread))){
//     return false;
//   }
//   if(base){
//     *base = kThread.StackLimit;
//   }
//   if(size){
//     *size = kThread.InitialStack - kThread.StackLimit;
//   }
// }
// bool UCoreMonitor::getCurrentStack(S2EExecutionState* state,
//                                    uint64_t pThread,
//                                    uint64_t* base,
//                                    uint64_t* size){
//   uint64_t pThread = getCurrentThread(state);
//   if(!pThread){
//     return false;
//   }
//   return getThreadStack(state, pThread, base, size);
// }

// bool UCoreMonitor::isKernelAddress(uint64_t pc) const {
//   return pc >= getKernelStart();
// }

bool UCoreMonitor::getImports(S2EExecutionState *s, const ModuleDescriptor &desc,
                Imports &I){
  return false;
}

bool UCoreMonitor::getExports(S2EExecutionState *s, const ModuleDescriptor &desc,
                Exports &E){
  return false;
}

bool UCoreMonitor::isKernelAddress(uint64_t pc) const {
  return (pc >= getKernelStart());
}

uint64_t UCoreMonitor::getPid(S2EExecutionState *s, uint64_t pc){
  return pc;
}

bool UCoreMonitor::getCurrentStack(S2EExecutionState *s, uint64_t *base,
                                   uint64_t *size){
  return false;
}

void UCoreMonitor::parseSystemMapFile(){
  ifstream system_map_stream;
  system_map_stream.open(system_map_file.c_str());
  if(!system_map_stream){
    s2e()->getWarningsStream() << "Unable to open System.map file"
                               << system_map_file << ".\n";
    exit(1);
  }

  char line[255];
  uint64_t addr;
  string kernel_symbol;
  while(system_map_stream){
    system_map_stream.getline(line, 255);
    char temp[200];
    sscanf(line, "%lx %s", &addr, temp);
    symbol_struct sym;
    sym.addr = addr;
    sym.type = 's';
    sym.name = temp;
    sTable[addr] = sym;
  }
  return;
}
///////////////////////////

UCoreMonitorState::UCoreMonitorState(){
  m_CurrentPid = -1;
}

UCoreMonitorState::~UCoreMonitorState(){
}

UCoreMonitorState* UCoreMonitorState::clone() const {
  return new UCoreMonitorState(*this);
}

PluginState *UCoreMonitorState::factory(Plugin *p, S2EExecutionState *state){
  return new UCoreMonitorState();
}
