#ifndef _UCORE_THREAD_DESCRIPTOR_H_
#define _UCORE_THREAD_DESCRIPTOR_H_

#define CURRENT_THREAD_SYMBOL "pls_current"
#define NR_PROCESS_SYMBOL "nr_process"
#define PCB_LINKED_LIST_SYMBOL "proc_list"
// #define PCB_SIZE                    120
// #define PCB_STATE_OFFSET            0
// #define PCB_PID_OFFSET              4
// #define PCB_RUNS_OFFSET             8
// #define PCB_PARENT_OFFSET           20
// #define PCB_NAME_OFFSET             72
// #define PCB_NAME_LEN                16
// #define PCB_LIST_LINK_OFFSET        88
#define PCB_SIZE                    228
#define PCB_STATE_OFFSET            0
#define PCB_PID_OFFSET              4
#define PCB_RUNS_OFFSET             16
#define PCB_PARENT_OFFSET           28
#define PCB_NAME_OFFSET             80
#define PCB_NAME_LEN                16
#define PCB_LIST_LINK_OFFSET        96

#define LIST_ENTRY_SIZE  8
#define LIST_NEXT_OFFSET 4

#include <inttypes.h>
#include <string>

namespace s2e {

  enum proc_state{
    PROC_UNINIT = 0,
    PROC_SLEEPING = 1,
    PROC_RUNNABLE = 2,
    PROC_ZOMBIE = 3
  };

  typedef struct _UCorePCB{
    enum proc_state state;
    uint64_t runs;
    uint64_t pid;
    uint64_t parentAddr;
    uint64_t pcb_addr;
    std::string* name;
    bool isCurrent;
  } UCorePCB;
}

#endif
