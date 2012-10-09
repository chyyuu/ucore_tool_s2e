/**
   UCore Monitor
   By Nuk, Tsinghua University.
*/

extern "C" {
#include "config.h"
#include "qemu-common.h"
}

#include "UCoreMonitor.h"
#include <cstring>
#include <iostream>
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

using namespace std;
using namespace s2e;
using namespace s2e::plugins;

S2E_DEFINE_PLUGIN(UCoreMonitor, "Monitor of UCore OS", "UCoreMonitor", );
UCoreMonitor::~UCoreMonitor(){
}

void UCoreMonitor::initialize(){
  //Read kernel Version Address
  m_KernelBase = s2e()->getConfig()->getInt(getConfigKey() + ".kernelBase");
  m_KernelEnd = s2e()->getConfig()->getInt(getConfigKey() + ".kernelEnd");
  m_MonitorFunction = s2e()->getConfig()->getBool(getConfigKey() + ".MonitorFunction");
  m_MonitorThreads = s2e()->getConfig()->getBool(getConfigKey() + ".MonitorThreads");

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
  if(m_MonitorThreads){
    m_KeCurrentThread = sMap["current"];
    this->onFunctionCalling.connect(sigc::mem_fun(*this, &UCoreMonitor::slotFunctionCalling));
  }
}

void UCoreMonitor::slotFunctionCalling(ExecutionSignal *signal,
                                          S2EExecutionState *state,
                                          std::string fname, uint64_t pc){
  if(fname == "proc_run"){
    //Proc switch
    signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotKmThreadSwitch));
  }else if(fname == "set_proc_name"){
    //Thread create
    signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotKmThreadInit));
  }else if(fname == "do_exit"){
    //Thread exit
    signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotKmThreadExit));
  }
}

// void UCoreMonitor::onCustomInstruction(S2EExecutionState *state, uint64_t opcode){
//   uint64_t arg = (opcode >> 8) & 0xFF;
//   if(arg == 0x1f){
//     if(m_MonitorThreads){
//       s2e()->getMessagesStream() << "[UCoreMonitor]Forking Stream :)\n";
//       uint64_t pThread = 0;
//       bool ok = state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]),
//                                                &pThread, 4);
//       if(!ok){
//         s2e()->getWarningsStream(state) << "[ERROR] Get pThread failed.\n";
//         return;
//       }
//       uint64_t pSize = 0;
//       ok  = state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ECX]),
//                                            &pSize, 4);
//       if(!ok){
//         s2e()->getWarningsStream(state) << "[ERROR] Get pSize failed.\n";
//         return;
//       }

//       char* ucorePCB = new char[pSize];
//       if(!state->readMemoryConcrete(pThread, ucorePCB, pSize)){
//         s2e()->getWarningsStream(state) << "[ERROR] Get PCB content failed.";
//         return;
//       }
//       uint32_t state;
//       uint32_t pid;
//       uint32_t runs;
//       uint32_t parent;
//       char name[16];
//       memcpy(&state, ucorePCB, 4);
//       memcpy(&pid, ucorePCB + 4, 4);
//       memcpy(&runs, ucorePCB + 8, 4);
//       memcpy(&parent, ucorePCB + 20, 4);
//       memcpy(name, ucorePCB + 72, 16);
//       s2e() ->getMessagesStream() << "[UCoreMonitor]State:" << state << "\n";
//       s2e() ->getMessagesStream() << "[UCoreMonitor]Pid:" << pid << "\n";
//       s2e() ->getMessagesStream() << "[UCoreMonitor]Runs:" << runs << "\n";
//       s2e() ->getMessagesStream() << "[UCoreMonitor]Parent:0x";
//       s2e() ->getMessagesStream().write_hex(parent);
//       s2e() ->getMessagesStream() << "\n";
//       s2e() ->getMessagesStream() << "[UCoreMonitor]Name:" << name << "\n";
//       UCorePCB* pcb = new UCorePCB();
//       pcb->state = (enum proc_state)state;
//       pcb->pid = pid;
//       pcb->runs = runs;
//       pcb->parent = (UCorePCB*)parent;
//       memcpy(pcb->name, ucorePCB + 72, 16);
//       threadMap[pThread] = pcb;
//       delete[] ucorePCB;
//       ucorePCB = NULL;
//     }
//   }
// }

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

  uint64_t vpc = state->getPc();
  if (vpc >= 0x00100000 && vpc <= 0x3fffffff)
    vpc += 0xc0000000;

//  s2e()->getDebugStream() << "Entering function:" << sTable[vpc].name 
//  << " @ 0x" << func_addr << "\n";
  //added by fwl
  ExecutionSignal onFunctionSignal;
  onFunctionTransition.emit(&onFunctionSignal, state, sTable[vpc].name, pc);
  onFunctionSignal.emit(state, pc);
  //added by Nuk
  ExecutionSignal onFunctionCallingSignal;
  onFunctionCalling.emit(&onFunctionCallingSignal, state,
                         sTable[vpc].name, pc);
  onFunctionCallingSignal.emit(state, pc);
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
  //s2e()->getDebugStream() << "Exiting function:" << sTable[func].name << "\n";
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

//Monitoring function proc_run
void UCoreMonitor::slotKmThreadSwitch(S2EExecutionState *state, uint64_t pc){
  s2e()->getDebugStream() << "UCoreMonitor: switching kernel-mode thread @ ";
  s2e()->getDebugStream().write_hex(pc) << "\n";
  UCorePCB* current = parseUCorePCB(m_KeCurrentThread);
  uint64_t KeNextThread = state->getBp() + 8;
  UCorePCB* next = parseUCorePCB(KeNextThread);
  onThreadSwitching.emit(state, current, next, pc);
}

//Monitoring func set_proc_name
void UCoreMonitor::slotKmThreadInit(S2EExecutionState *state, uint64_t pc) {
  s2e()->getDebugStream() << "UCoreMonitor: creating kernel-mode thread! \n";
  s2e()->getDebugStream().write_hex(pc) << "\n";
  uint64_t KeProc = state->getBp() + 8;
  uint64_t KeProcName = state0>getBp() + 4;
  UCorePCB* proc = parseUCorePCB(KeProc);
  proc->name = parseUCorePName(KeProcName);
  onThreadCreating.emit(state, proc, pc);
}

//Monitoring func do_exit
void UCoreMonitor::slotKmThreadExit(S2EExecutionState *state, uint64_t pc) {
  s2e()->getDebugStream() << "UCoreMonitor: killing kernel-mode thread! \n";
  s2e()->getDebugStream().write_hex(pc) << "\n";
  UCorePCB current = parseUCorePCB(m_KeCurrentThread);
  onThreadKilling.emit(state, current, pc);
}

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
    sMap[temp] = addr;
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
