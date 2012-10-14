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
#include <cstdlib>
#include <string>
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
                                          string fname, uint64_t pc){
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
//Monitoring function proc_run
void UCoreMonitor::slotKmThreadSwitch(S2EExecutionState *state, uint64_t pc){
  s2e()->getDebugStream() << "[UCoreMonitor]Thread switching\n";
  uint64_t esp = state->getSp();
  //Note: 4 for return address
  uint64_t ppPCB = esp + 4;
  UCorePCB* prev = parseUCorePCB(state, m_KeCurrentThread);
  UCorePCB* next = parseUCorePCB(state, ppPCB);
  //printUCorePCB(prev);
  //printUCorePCB(next);
  onThreadSwitching.emit(state, prev, next, pc);
}
//Monitoring func set_proc_name
void UCoreMonitor::slotKmThreadInit(S2EExecutionState *state, uint64_t pc) {
  s2e()->getDebugStream() << "[UCoreMonitor]Thread initing\n";
  uint64_t esp = state->getSp();
  //Note: here 4 = return address
  uint64_t ppPCB = esp + 4;
  //Note: here 8 = 4(return address) + 4(proc struct pointer);
  uint64_t ppName = esp + 8;
  UCorePCB* proc = parseUCorePCB(state, ppPCB);
  proc->name = parseUCorePName(state, ppName);
  //for debug
  //printUCorePCB(proc);
  onThreadCreating.emit(state, proc, pc);
}
//Monitoring func do_exit
void UCoreMonitor::slotKmThreadExit(S2EExecutionState *state, uint64_t pc) {
  s2e()->getDebugStream() << "[UCoreMonitor]Thread exiting\n";
  UCorePCB* current = parseUCorePCB(state, m_KeCurrentThread);
  //printUCorePCB(current);
  onThreadExiting.emit(state, current, pc);
}

//Printing UCorePCB
void UCoreMonitor::printUCorePCB(UCorePCB* proc){
  if(proc == NULL){
    return;
  }
  s2e()->getDebugStream() << "proc->state: " << proc->state <<"\n";
  s2e()->getDebugStream() << "proc->pid: " << proc->pid << "\n";
  s2e()->getDebugStream() << "proc->runs: " << proc->runs << "\n";
  s2e()->getDebugStream() << "proc->parent: ";
  s2e()->getDebugStream().write_hex(proc->parentAddr) << "\n";
  if(proc->name != NULL){
    s2e()->getDebugStream() << "proc->name: " << *proc->name << "\n";
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

  uint64_t vpc = state->getPc();
  if (vpc >= 0x00100000 && vpc <= 0x3fffffff)
    vpc += 0xc0000000;

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

//Nuk's parsing area, dirty code's all here ;)

UCorePCB* UCoreMonitor::parseUCorePCB(S2EExecutionState *state,
                                 uint64_t addr){
  uint64_t pPCB = 0;
  if(!state->readMemoryConcrete(addr, (void*)(&pPCB), 4)){
    s2e()->getWarningsStream(state) << "[ERROR]Get pPCB error!\n";
    return NULL;
  }
  UCorePCB* pcb = new UCorePCB();
  char block[PCB_SIZE];
  if(!state->readMemoryConcrete(pPCB, block, PCB_SIZE)){
    s2e()->getWarningsStream(state) << "[ERROR]Get PCB error!\n";
    return NULL;
  }
  memcpy(&(pcb->state), block + PCB_STATE_OFFSET, 4);
  memcpy(&(pcb->pid), block + PCB_PID_OFFSET, 4);
  memcpy(&(pcb->runs), block + PCB_RUNS_OFFSET, 4);
  memcpy(&(pcb->parentAddr), block + PCB_PARENT_OFFSET, 4);
  pcb->pcb_addr = pPCB;
  return pcb;
}
uint64_t UCoreMonitor::parseUCorePPid(S2EExecutionState* state,
                                  uint64_t addr){
  uint64_t pPCB = 0;
  if(!state->readMemoryConcrete(addr, (void*)(&pPCB), 4)){
    s2e()->getWarningsStream(state) << "[ERROR]Get pPCB error!\n";
    return NULL;
  }
  char block[PCB_SIZE];
  if(!state->readMemoryConcrete(pPCB, block, PCB_SIZE)){
    s2e()->getWarningsStream(state) << "[ERROR]Get PCB error!\n";
    return NULL;
  }
  uint64_t ret = 0;
  memcpy(&ret, block + PCB_PID_OFFSET, 4);
  return ret;
}
std::string* UCoreMonitor::parseUCorePName(S2EExecutionState *state,
                                   uint64_t addr){
  char block[PCB_NAME_LEN];
  memset(block, 0, PCB_NAME_LEN);
  uint64_t pBlock = 0;
  //for debug
  //s2e()->getDebugStream() << "pPointer: ";
  //s2e()->getDebugStream().write_hex(pPointer) << "\n";
  if(!state->readMemoryConcrete(addr, (void*)(&pBlock), 4)){
    s2e()->getWarningsStream(state) << "[ERROR]Get pBlock error!\n";
    return NULL;
  }
  if(!state->readMemoryConcrete(pBlock, block, PCB_NAME_LEN)){
    s2e()->getWarningsStream(state) << "[ERROR]Get block error!\n";
    return NULL;
  }
  std::string* result = new std::string(block);
  return result;
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
