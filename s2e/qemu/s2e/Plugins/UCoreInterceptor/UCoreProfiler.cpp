/**
   UCore Monitor
   By Nuk, Tsinghua University.
*/

extern "C" {
#include "config.h"
#include "qemu-common.h"
}

#include "UCoreProfiler.h"
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

using namespace std;
using namespace s2e;
using namespace s2e::plugins;

S2E_DEFINE_PLUGIN(UCoreProfiler, "Profiler of UCore",
                    "UCoreProfiler", "UCoreMonitor");

UCoreProfiler::~UCoreProfiler(){
  delete current;
}

void UCoreProfiler::initialize(){
  UCoreMonitor* monitor = static_cast<UCoreMonitor*>
    (s2e()->getPlugin("UCoreMonitor"));
  assert(monitor);
  //init variables
  current = NULL;
  //connect Thread signals
  monitor->onThreadSwitching.
    connect(sigc::mem_fun(*this, &UCoreProfiler::slotThreadSwitch));
  monitor->onThreadCreating.
    connect(sigc::mem_fun(*this, &UCoreProfiler::slotThreadSwitch));
  monitor->onThreadExiting.
    connect(sigc::mem_fun(*this, &UCoreProfiler::slotThreadSwitch));

}

void UCoreProfiler::slotThreadCreation(S2EExecutionState *state,
                                       UCorePCB* newThread,
                                       uint64_t pc){
  assert(newThread);
  //assert when proc inits
  if(newThread->pid == 0 && current == NULL){
    current = newThread;
  }
  pid2addr[newThread->pid] = newThread->pcb_addr;
  addr2PCB[newThread->pcb_addr] = newThread;
  return;
}

void UCoreProfiler::slotThreadSwitch(S2EExecutionState *state,
                                       uint64_t prevPid,
                                       uint64_t nextPid){
  UCorePCB* prev = addr2PCB[pid2addr[prevPid]];
  UCorePCB* next = addr2PCB[pid2addr[nextPid]];
  assert(prev);
  assert(next);
  current = next;
  return;
}

void UCoreProfiler::slotThreadExit(S2EExecutionState* state,
                                   uint64_t exitPid,
                                   uint64_t pc){
  UCorePCB* exit = addr2PCB[pid2addr[exitPid]];
  assert(exit);
  delete exit;
  addr2PCB[pid2addr[exitPid]] = NULL;
  pid2addr[exitPid] = -1;
  return;
}

void UCoreProfiler::printThreadGraph(){
  UCorePCB* iteraror = current;
  while(iteraror->parent){
    s2e()->getDebugStream() << iterator->name << "<----";
    iterator = addr2PCB[iterator->parentAddr];
  }
  s2e()->getDebugStream() << "\n";
}
