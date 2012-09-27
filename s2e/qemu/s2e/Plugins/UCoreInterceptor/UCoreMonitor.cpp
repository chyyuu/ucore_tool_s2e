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
#include <cstring>
#include <iostream>
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

using namespace std;
using namespace s2e;
using namespace s2e::plugins;

S2E_DEFINE_PLUGIN(UCoreMonitor, "Monitor of UCore OS", "UCoreMonitor", );

uint64_t UCoreMonitor::s_KeInitThread = 0xc010bfcb;
uint64_t UCoreMonitor::s_KeTerminateThread = 0xc010c0f9;
uint64_t UCoreMonitor::s_KeSwitchThread = 0xc010bc7e;
uint64_t UCoreMonitor::s_KeCurrentThread = 0xc012ed48;

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
  m_MonitorFunction = s2e()->getConfig()->getBool(getConfigKey() + ".MonitorFunction");
  m_MonitorThreads = s2e()->getConfig()->getBool(getConfigKey() + ".MonitorThreads");
  //User Mode Event and Kernel Mode Event
  // m_UserMode = s2e()->getConfig()->getBool(getConfigKey() + ".userMode");
  // m_KernelMode = s2e()->getConfig()->getBool(getConfigKey() + ".kernelMode");
  // if(m_UserMode){
  //   m_UCoreUserModeEvent = new UCoreUserModeEvent(this);
  // }
  // if(m_KernelMode){
  //   m_KernelModeEvent = new UCoreKernelModeEvent(this);
  // }

  //parse system map file
  bool ok;
  system_map_file = s2e()->getConfig()->getString(getConfigKey() + ".system_map_file", "", &ok);
  if(!ok){
    s2e()->getWarningsStream() << "No System.map file provided. System.map is needed for UCoreMonitor to work properly. Quit.\n";
    exit(-1);
  }
  parseSystemMapFile();
  //connect Signals
  if(m_MonitorFunction){
    s2e()->getCorePlugin()->onTranslateBlockEnd
      .connect(sigc::mem_fun(*this, &UCoreMonitor::onTranslateBlockEnd));
    s2e()->getCorePlugin()->onTranslateJumpStart
      .connect(sigc::mem_fun(*this, &UCoreMonitor::onTBJumpStart));
  }
  s2e()->getCorePlugin()->onTranslateInstructionStart
    .connect(sigc::mem_fun(*this, &UCoreMonitor::onTranslateInstruction));
  s2e()->getCorePlugin()->onCustomInstruction
    .connect(sigc::mem_fun(*this, &UCoreMonitor::onCustomInstruction));
}

void UCoreMonitor::onCustomInstruction(S2EExecutionState *state, uint64_t opcode){
  uint64_t arg = (opcode >> 8) & 0xFF;
  if(arg == 0x1f){
    if(m_MonitorThreads){
      s2e()->getMessagesStream() << "[UCoreMonitor]Forking Stream :)\n";
      uint64_t pThread = 0;
      bool ok = state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]),
                                               &pThread, 4);
      if(!ok){
        s2e()->getWarningsStream(state) << "[ERROR] Get pThread failed.\n";
        return;
      }
      uint64_t pSize = 0;
      ok  = state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ECX]),
                                           &pSize, 4);
      if(!ok){
        s2e()->getWarningsStream(state) << "[ERROR] Get pSize failed.\n";
        return;
      }

      char* ucorePCB = new char[pSize];
      if(!state->readMemoryConcrete(pThread, ucorePCB, pSize)){
        s2e()->getWarningsStream(state) << "[ERROR] Get PCB content failed.";
        return;
      }
      uint32_t state;
      uint32_t pid;
      uint32_t runs;
      uint32_t parent;
      char name[16];
      memcpy(&state, ucorePCB, 4);
      memcpy(&pid, ucorePCB + 4, 4);
      memcpy(&runs, ucorePCB + 8, 4);
      memcpy(&parent, ucorePCB + 20, 4);
      memcpy(name, ucorePCB + 72, 16);
      s2e() ->getMessagesStream() << "[UCoreMonitor]State:" << state << "\n";
      s2e() ->getMessagesStream() << "[UCoreMonitor]Pid:" << pid << "\n";
      s2e() ->getMessagesStream() << "[UCoreMonitor]Runs:" << runs << "\n";
      s2e() ->getMessagesStream() << "[UCoreMonitor]Parent:0x";
      s2e() ->getMessagesStream().write_hex(parent);
      s2e() ->getMessagesStream() << "\n";
      s2e() ->getMessagesStream() << "[UCoreMonitor]Name:" << name << "\n";
      UCorePCB* pcb = new UCorePCB();
      pcb->state = (enum proc_state)state;
      pcb->pid = pid;
      pcb->runs = runs;
      pcb->parent = (UCorePCB*)parent;
      memcpy(pcb->name, ucorePCB + 72, 16);
      threadMap[pThread] = pcb;
      delete[] ucorePCB;
      ucorePCB = NULL;
    }
  }
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
    }else if(m_MonitorThreads && pc == getKeSwitchThread()){
      signal->connect(sigc::mem_fun(*this,
                                    &UCoreMonitor::slotKmThreadSwitch));
    }
  }
}

//emit Signal
void UCoreMonitor::onTranslateBlockEnd(ExecutionSignal *signal,
                                       S2EExecutionState *state,
                                       TranslationBlock *tb,
                                       uint64_t pc, bool static_target
                                       , uint64_t target_pc){
        uint64_t vpc = pc;
        if (vpc >= 0x00100000 && vpc <= 0x3fffffff)
                vpc += 0xc0000000;
  if(vpc >= getKernelStart()){
    if(tb->s2e_tb_type == TB_CALL || tb->s2e_tb_type == TB_CALL_IND){
      signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotCall));
    }
  }
}

void UCoreMonitor::slotCall(S2EExecutionState *state, uint64_t pc){
  char func_addr[1024];
  uint64_t vpc = state->getPc();
  if (vpc >= 0x00100000 && vpc <= 0x3fffffff)
          vpc += 0xc0000000;
  uint2hexstring(vpc, func_addr, 1024);

//  s2e()->getDebugStream() << "Entering function:" << sTable[vpc].name << " @ 0x" << func_addr
//                << "\n";

  //added by fwl

  ExecutionSignal onFunctionSignal;
  onFunctionTransition.emit(&onFunctionSignal, state, sTable[vpc].name, pc);
  onFunctionSignal.emit(state, pc);
  //callStack.push_back(pc);
  callStack.push_back(vpc);
}

void UCoreMonitor::onTBJumpStart (ExecutionSignal *signal,
                                  S2EExecutionState *state,
                                  TranslationBlock *tb,
                                  uint64_t, int jump_type){
        uint64_t vpc = state->getPc();
        if (vpc >= 0x00100000 && vpc <= 0x3fffffff)
                        vpc += 0xc0000000;
  if(vpc >= getKernelStart()){
    if(jump_type == JT_RET || jump_type == JT_LRET){
      signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotRet));
    }
  }
}
void UCoreMonitor::slotRet(S2EExecutionState *state, uint64_t pc){
  if(callStack.size() == 0)
    return;
  uint64_t func = callStack[callStack.size() - 1];
  char func_addr[1024];
  uint2hexstring(func, func_addr, 1024);
  char ret_addr[1024];
  uint2hexstring(pc, ret_addr, 1024);
//  s2e()->getDebugStream() << "@ 0x" << ret_addr << ": Exiting function:" << sTable[func].name
//                << " @ 0x" << func_addr << "\n";

  //added by fwl

  ExecutionSignal onFunctionSignal;
  onFunctionTransition.emit(&onFunctionSignal, state, sTable[func].name, pc);
  onFunctionSignal.emit(state, pc);
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

uint64_t UCoreMonitor::getKeSwitchThread() const {
  return s_KeCurrentThread;
}

void UCoreMonitor::slotKmThreadSwitch(S2EExecutionState *state, uint64_t pc){
  s2e()->getDebugStream() << "UCoreMonitor: switching kernel-mode thread!\n";

}

void UCoreMonitor::slotKmThreadInit(S2EExecutionState *state, uint64_t pc) {
  s2e()->getDebugStream() << "UCoreMonitor: creating kernel-mode thread! \n";
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
  s2e()->getDebugStream() << "UCoreMonitor: deleting kernel-mode thread! \n";
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
void UCoreMonitor::uint2hexstring(uint64_t number, char* string, int size){
  uint64_t len = 0;
  memset(string, 0, size * sizeof(char));

  uint64_t remainder;
  uint64_t quotient = number;

  while(quotient != 0){
    remainder = quotient % 16;
    quotient = quotient / 16;
    if(remainder < 10){
      string[len] = '0' + remainder;
    }else{
      string[len] = 'a' + remainder - 10;
    }
    len ++;
    //printf("%lu\n", quotient);
  }
  int i = 0;
  for(i = 0; i < len / 2; i ++){
    char temp;
    temp = string[i];
    string[i] = string[len - i - 1];
    string[len - i - 1] = temp;
  }
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
