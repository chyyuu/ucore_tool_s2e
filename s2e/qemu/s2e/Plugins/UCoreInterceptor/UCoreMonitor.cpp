/**
   UCore Monitor
   By Nuk, Tsinghua University.
*/

extern "C" {
#include "config.h"
#include "qemu-common.h"
#include "QEMUMonitorInterface.h"
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

S2E_DEFINE_PLUGIN(UCoreMonitor,
                  "Monitor of UCore OS",
                  "UCoreMonitor", );

UCoreMonitor::~UCoreMonitor(){
}

void UCoreMonitor::initialize(){

  m_MonitorFunction = s2e()->getConfig()->getBool(getConfigKey() +
                                                  ".MonitorFunction");
  m_MonitorThreads = s2e()->getConfig()->getBool(getConfigKey() +
                                                 ".MonitorThreads");
  m_MonitorPanic = s2e()->getConfig()->getBool(getConfigKey() +
                                               ".MonitorPanic");

  //parse kernel.sym file
  bool ok;
  system_map_file = s2e()->getConfig()->getString(getConfigKey() + ".system_map_file", "", &ok);
  if(!ok){
    s2e()->getWarningsStream() << "No kernel.sym file provided. System.map is needed for UCoreMonitor to work properly. Quit.\n";
    exit(-1);
  }
  parseSystemMapFile();

  //parse kernel.ld file
  kernel_ld_file = s2e()->getConfig()->getString(getConfigKey() + ".kernel_ld_file", "", &ok);
  if(!ok){
    s2e()->getWarningsStream() << "No kernel.ld file provided. System.map is needed for UCoreMonitor to work properly. Quit.\n";
    exit(-1);
  }
  parseKernelLd();
  //Ugly hard code
  m_KernelBase = 0xc0100000;

  //Get STAB section address
  m_StabStart = sMap[STAB_BEGIN_ADDR_SYMBOL];
  m_StabEnd = sMap[STAB_END_ADDR_SYMBOL];
  m_StabStrStart = sMap[STABSTR_BEGIN_ADDR_SYMBOL];
  m_StabStrEnd = sMap[STABSTR_END_ADDR_SYMBOL];
  stabParsed = false;
  stab_array = NULL;
  stab_array_end = NULL;
  stabstr_array = NULL;
  stabstr_array_end = NULL;

  //Define the first running varible as true.
  first = true;
  //connect Signals
  if(m_MonitorFunction){
    s2e()->getCorePlugin()->onTranslateBlockEnd
      .connect(sigc::mem_fun(*this, &UCoreMonitor::onTranslateBlockEnd));
    s2e()->getCorePlugin()->onTranslateJumpStart
      .connect(sigc::mem_fun(*this, &UCoreMonitor::onTBJumpStart));
    if(m_MonitorThreads){
      m_KeCurrentThread = sMap[CURRENT_THREAD_SYMBOL];
      m_KeNrProcess = sMap[NR_PROCESS_SYMBOL];
      m_KePCBLinkedList = sMap[PCB_LINKED_LIST_SYMBOL];
      this->onFunctionCalling.connect(sigc::mem_fun(*this, &UCoreMonitor::slotFunctionCalling));
    }
    if(m_MonitorPanic){
      paniced = false;
      range = 3;
      pcbCounter = 0;
      pcb_array = new UCorePCB[range];
    }
  }
} //Initialize

void UCoreMonitor::slotFunctionCalling(ExecutionSignal *signal,
                                          S2EExecutionState *state,
                                          string fname, uint64_t pc){
  if(fname == "proc_run"){
    //Proc switch
    signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotKmThreadSwitch));
  }else if(fname == "set_proc_name"){
    //Thread create
    signal->connect(sigc::mem_fun( *this, &UCoreMonitor::slotKmThreadInit));
  }else if(fname == "do_exit"){
    //Thread exit
    signal->connect(sigc::mem_fun(*this,
                                  &UCoreMonitor::slotKmThreadExit));
  }else if(m_MonitorPanic){
    if(fname == "__panic"){
      signal->connect(sigc::mem_fun(*this,
                                    &UCoreMonitor::PanicMonitor));
    }
  }
}//slotFunctionCalling

void UCoreMonitor::PanicMonitor(S2EExecutionState *state,
                                uint64_t pc){
  s2e()->getWarningsStream() << "\n[UCoreMonitor]Kernel Panic!!\n";
  paniced = true;
}//PanicMonitor

//Monitoring function proc_run
void UCoreMonitor::slotKmThreadSwitch(S2EExecutionState *state,
                                      uint64_t pc){
  s2e()->getDebugStream() << "[UCoreMonitor]Thread switching\n";
  uint64_t esp = state->getSp();
  //Note: 4 for return address
  uint64_t ppPCB = esp + 4;
  //get current as prev
  uint64 pPCB = 0;
  if(!state->readMemoryConcrete(m_KeCurrentThread, (void*)(&pPCB), 4)){
    s2e()->getWarningsStream(state) << "[ERROR]Get pPCB error1!\n";
    return;
  }
  UCorePCB* prev = parseUCorePCB(state, pPCB);
  //get next...
  if(!state->readMemoryConcrete(ppPCB, (void*)(&pPCB), 4)){
    s2e()->getWarningsStream(state) << "[ERROR]Get pPCB error2!\n";
    return;
  }
  UCorePCB* next = parseUCorePCB(state, pPCB);
  //printUCorePCB(prev);
  printUCorePCB(next);
  pcb_array[pcbCounter] = *next;
  pcbCounter = (pcbCounter + 1) % range;
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
  uint64_t pPCB = 0;
  if(!state->readMemoryConcrete(ppPCB, (void*)(&pPCB), 4)){
    s2e()->getWarningsStream(state) << "[ERROR]Get pPCB error!\n";
    return;
  }
  UCorePCB* proc = parseUCorePCB(state, pPCB);
  proc->name = parseUCorePName(state, ppName);
  //for debug
  //printUCorePCB(proc);
  onThreadCreating.emit(state, proc, pc);
}

//Monitoring func do_exit
void UCoreMonitor::slotKmThreadExit(S2EExecutionState *state, uint64_t pc) {
  s2e()->getDebugStream() << "[UCoreMonitor]Thread exiting\n";
  uint64_t pPCB = 0;
  if(!state->readMemoryConcrete(m_KeCurrentThread, (void*)(&pPCB), 4)){
    s2e()->getWarningsStream(state) << "[ERROR]Get pPCB error!\n";
    return;
  }
  UCorePCB* current = parseUCorePCB(state, pPCB);
  //printUCorePCB(current);
  onThreadExiting.emit(state, current, pc);
}

//catch call inst
void UCoreMonitor::onTranslateBlockEnd(ExecutionSignal *signal,
                                       S2EExecutionState *state,
                                       TranslationBlock *tb,
                                       uint64_t pc, bool static_target
                                       , uint64_t target_pc){
  if(pc >= getKernelStart()){
    if(tb->s2e_tb_type == TB_CALL || tb->s2e_tb_type == TB_CALL_IND){
      signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotCall));
    }
  }
}

//slot call inst
void UCoreMonitor::slotCall(S2EExecutionState *state, uint64_t pc){
  if(pc > getKernelStart() && first){
    parseUCoreStab(state);
    first = false;
    stabParsed = true;
  }
  uint64_t vpc = state->getPc();

  //added by fwl
  ExecutionSignal onFunctionSignal;
  onFunctionTransition.emit(&onFunctionSignal, state, sTable[vpc].name, pc);
  onFunctionSignal.emit(state, pc);

  //added by Nuk
  ExecutionSignal callingSignal;
  onFunctionCalling.emit(&callingSignal, state,
                         sTable[vpc].name, pc);
  callingSignal.emit(state, pc);
}

//catch ret inst
void UCoreMonitor::onTBJumpStart (ExecutionSignal *signal,
                                  S2EExecutionState *state,
                                  TranslationBlock *tb,
                                  uint64_t pc, int jump_type){
  /* We do not catch function return now, so these are commentted. */
  /*Added by fwl*/
  // uint64_t vpc = state->getPc();

  // if(vpc >= getKernelStart() && ((vpc & 0xf0000000) == 0xc0000000)){
  //   if(jump_type == JT_RET || jump_type == JT_LRET){
  //     signal->connect(sigc::mem_fun(*this, &UCoreMonitor::slotRet));
  //   }
  // }
}

//slot ret inst
/* We do not catch function return now, so these are not used. */
void UCoreMonitor::slotRet(S2EExecutionState *state, uint64_t pc){
  if(!stabParsed){
    return;
  }
  uint64_t vpc = state->getPc();
  UCoreFunc currentFunc;
  if(parseUCoreFunc(vpc, &currentFunc) == 0){ //0 means success
    //printUCoreFunc(currentFunc);
  }else{
    s2e()->getWarningsStream() << "Func @ " << vpc << "can't parse!";
  }

  //added by Nuk
  ExecutionSignal returningSignal;
  onFunctionReturning.emit(&returningSignal, state,
                           currentFunc.fn_name, pc);
  returningSignal.emit(state, pc);
  //added by fwl
  ExecutionSignal onFunctionSignal;
  onFunctionTransition.emit(&onFunctionSignal, state,
  sTable[currentFunc.fn_entry].name, pc);
  onFunctionSignal.emit(state, pc);
}

/**************Signal Unrelated funcs********************/
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

//-------------------------------------
//         Print   Functions
//-------------------------------------

void UCoreMonitor::printPanicInfo(S2EExecutionState* state){
  if(!paniced){
    cout << "I haven't encounted panic in OS.\n";
    return;
  }
  uint64_t pc = state->getPc();
  UCoreFunc* func = new UCoreFunc();
  parseUCoreFunc(pc, func);
  s2e()->getWarningsStream() << "Panic @:" << func->fn_name << "\n" ;
  printUCorePCB(&pcb_array[0]);
}

void UCoreMonitor::printUCorePCB(UCorePCB* proc){
  if(proc == NULL){
    return;
  }
  cout << "proc->state: " << proc->state <<"\n";
  cout << "proc->pid: " << proc->pid << "\n";
  cout << "proc->runs: " << proc->runs << "\n";
  cout << "proc->parent: ";
  cout << std::hex << (proc->parentAddr) << "\n";
  if(proc->name != NULL){
    cout << "proc->name: " << *proc->name << "\n";
  }
}

void UCoreMonitor::printUCoreFunc(UCoreFunc func){
  s2e()->getWarningsStream() << "\nSource Name:";
  s2e()->getWarningsStream() << func.src_name;
  s2e()->getWarningsStream() << "\nFunc Entry:";
  s2e()->getWarningsStream().write_hex(func.fn_entry);
  s2e()->getWarningsStream() << "\nFunc Name:";
  s2e()->getWarningsStream() << func.fn_name;
  s2e()->getWarningsStream() << "\n";
}

void UCoreMonitor::printAllThreads(S2EExecutionState* state){
  int nr_process = parseNrProcess(state);
  cout << "We have " << nr_process << " processes now.\n";
  UCorePCB** processes = parseUCorePCBLinkedList(state, nr_process);
  for(int i = 0; i < nr_process; i ++){
    printUCorePCB(processes[i]);
  }
  for(int i = 0; i < nr_process; i ++){
    delete processes[i]->name;
  }
  delete[] processes;
  return;
}

void UCoreMonitor::printHelloWorld(){
  cout << "Hello world!\n";
}

//-------------------------------------
//         Parse   Functions
//-------------------------------------

UCorePCB* UCoreMonitor::parseUCorePCB(S2EExecutionState *state,
                                 uint64_t addr){
  if(addr == 0)
    return NULL;
  UCorePCB* pcb = new UCorePCB();
  char block[PCB_SIZE];
  if(!state->readMemoryConcrete(addr, block, PCB_SIZE)){
    s2e()->getWarningsStream(state) << "[ERROR]Parse PCB error!\n";
    return NULL;
  }
  memcpy(&(pcb->state), block + PCB_STATE_OFFSET, 4);
  memcpy(&(pcb->pid), block + PCB_PID_OFFSET, 4);
  memcpy(&(pcb->runs), block + PCB_RUNS_OFFSET, 4);
  memcpy(&(pcb->parentAddr), block + PCB_PARENT_OFFSET, 4);
  pcb->pcb_addr = addr;
  return pcb;
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
    s2e()->getWarningsStream(state) << "[ERROR]Get block error1!\n";
    return NULL;
  }
  std::string* result = new std::string(block);
  return result;
}

std::string* UCoreMonitor::parseUCorePNamePrint(S2EExecutionState *state,
                                   uint64_t addr){
  if(addr == 0)
    return NULL;
  char block[PCB_NAME_LEN];
  memset(block, 0, PCB_NAME_LEN);
  if(!state->readMemoryConcrete(addr, block, PCB_NAME_LEN)){
    s2e()->getWarningsStream(state) << "[ERROR]Get block error2!\n";
    return NULL;
  }
  std::string* result = new std::string(block);
  return result;
}

int UCoreMonitor::parseUCoreFunc(uint64_t addr,
                                  UCoreFunc* func){
  func->src_name = "<unknown>";
  func->fn_entry = addr;
  func->line_num = 0;

  UCoreStab *stabs, *stab_end;
  stabs = stab_array;
  stab_end = stab_array_end;
  char* stabstr, *stabstr_end;
  stabstr = stabstr_array;
  stabstr_end = stabstr_array_end;

  // String table validity checks
  if (stabstr_end <= stabstr || stabstr_end[-1] != 0) {
    return -1;
  }

  int lfile = 0, rfile = (stab_end - stabs) - 1;
  stab_binsearch(stabs, &lfile, &rfile, N_SO, addr);
  if(lfile == 0){
    return -1;
  }

  int lfun = lfile, rfun = rfile;
  int lline, rline;
  stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);
  char* fn_name;
  size_t fn_namelen;
  if(lfun <= rfun){
    if(stabs[lfun].n_strx < stabstr_end - stabstr){
      fn_name = stabstr + stabs[lfun].n_strx;
    }
    func->fn_entry = stabs[lfun].n_value;
    addr -= func->fn_entry;
    lline = lfun;
    rline = rfun;
  }else{
    func->fn_entry = addr;
    lline = lfile;
    rline = rfile;
  }
  char* temp = fn_name;
  while(*temp != '\0'){
    if(*temp == ':'){
      break;
    }
    temp ++;
  }
  fn_namelen = temp - fn_name;
  func->fn_name = *(new string(fn_name, fn_namelen));
  stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
  if (lline <= rline) {
    func->line_num = stabs[rline].n_desc;
  } else {
    return -1;
  }
  //search line
  while (lline >= lfile
         && stabs[lline].n_type != N_SOL
         && (stabs[lline].n_type != N_SO || !stabs[lline].n_value)) {
    lline --;
  }
  //get filename
  if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr) {
    func->src_name = stabstr + stabs[lline].n_strx;
  }
  //success
  return 0;
}

void UCoreMonitor::stab_binsearch(UCoreStab* stabs, int* region_left,
                                  int* region_right, int type, uint64_t addr){
  int l = *region_left, r = *region_right, any_matches = 0;

  while (l <= r) {
    int true_m = (l + r) / 2, m = true_m;

    // search for earliest stab with right type
    while (m >= l && stabs[m].n_type != type) {
      m --;
    }
    if (m < l) {    // no match in [l, m]
      l = true_m + 1;
      continue;
    }

    // actual binary search
    any_matches = 1;
    if (stabs[m].n_value < addr) {
      *region_left = m;
      l = true_m + 1;
    } else if (stabs[m].n_value > addr) {
      *region_right = m - 1;
      r = m - 1;
    } else {
      // exact match for 'addr', but continue loop to find
      // *region_right
      *region_left = m;
      l = m;
      addr ++;
    }
  }

  if (!any_matches) {
    *region_right = *region_left - 1;
  }
  else {
    // find rightmost region containing 'addr'
    l = *region_right;
    for (; l > *region_left && stabs[l].n_type != type; l --)
      /* do nothing */;
    *region_left = l;
  }
}

void UCoreMonitor::parseUCoreStab(S2EExecutionState *state){
  //parse stab
  int n = (m_StabEnd - m_StabStart) / sizeof(UCoreStab);
  stab_array = new UCoreStab[n];
  for(int i = 0; i < n;i ++){
    int addr = m_StabStart + i * sizeof(UCoreStab);
    if(!state->readMemoryConcrete(addr,
                                  (void*)(stab_array + i),
                                  sizeof(UCoreStab))){
      s2e()->getWarningsStream() << "[ERROR]Parsing UCoreStab\n";
      exit(-1);
    }
  }
  stab_array_end = stab_array + n;
  //parse stabstr
  n = (m_StabStrEnd - m_StabStrStart) / sizeof(char);
  stabstr_array = new char[n];
  if(!state->readMemoryConcrete(m_StabStrStart,
                                (void*)(stabstr_array),
                                sizeof(char) * n)){
    s2e()->getWarningsStream() << "[ERROR]Parsing UCoreStab\n";
    exit(-1);
  }
  stabstr_array_end = stabstr_array + n;
  //print result
  //printUCoreStabs();
  return;
}

void UCoreMonitor::parseKernelLd(){
  ifstream kernel_ld_stream;
  kernel_ld_stream.open(kernel_ld_file.c_str());
  if(!kernel_ld_stream){
    s2e()->getWarningsStream() << "Unable to open file"
                               << kernel_ld_file << ".\n";
    exit(1);
  }
  char line[255];
  while(kernel_ld_stream){
    kernel_ld_stream.getline(line, 255);
  }
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

uint32_t UCoreMonitor::parseNrProcess(S2EExecutionState* state){
  uint32_t ret;
  if(!state->readMemoryConcrete(m_KeNrProcess, (void*)(&ret), 4)){
      s2e()->getWarningsStream(state) << "[ERROR]Get NrProcess error!\n";
      return NULL;
  }
  return ret;
}

UCorePCB** UCoreMonitor::parseUCorePCBLinkedList(S2EExecutionState* state,
                                   uint32_t n){
  UCorePCB** ret = new UCorePCB*[n];
  uint64_t offset = m_KePCBLinkedList;
  for(int i = 0; i < n; i ++){
    uint32_t list_pointer;
    if(!state->readMemoryConcrete(offset, (void*)(&list_pointer), sizeof(list_pointer))){
      s2e()->getWarningsStream(state) << "[ERROR]Get Linked List error!\n";
      exit(-1);
    }
    uint32_t pcb_pointer = list_pointer - PCB_LIST_LINK_OFFSET;
    ret[i] = parseUCorePCB(state, pcb_pointer);
    ret[i]->name = parseUCorePNamePrint(state, pcb_pointer + PCB_NAME_OFFSET);
    offset = offset + LIST_NEXT_OFFSET;
    uint32_t next_offset;
    if(!state->readMemoryConcrete(offset, (void*)(&next_offset), sizeof(next_offset))){
      s2e()->getWarningsStream(state) << "[ERROR]Get next offset error!\n";
      exit(-1);
    }
    offset = next_offset;
  }
  return ret;
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

///////////////////////////

void ucore_monitor_print_threads(void){
  ((UCoreMonitor*)g_s2e->getPlugin("UCoreMonitor"))->printAllThreads(g_s2e_state);
}

void ucore_monitor_print_panic_info(void){
  ((UCoreMonitor*)g_s2e->getPlugin("UCoreMonitor"))->printPanicInfo(g_s2e_state);
}
