#ifndef _UCORE_THREAD_DESCRIPTOR_H_

#define _UCORE_THREAD_DESCRIPTOR_H_

#define PCB_SIZE                    120
#define PCB_STATE_OFFSET            0
#define PCB_PID_OFFSET              4
#define PCB_RUNS_OFFSET             8
#define PCB_PARENT_OFFSET           20
#define PCB_NAME_OFFSET             72
#define PCB_NAME_LEN                16

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
    uint64_t parentAddr;
    uint64_t pcb_addr;
    uint32_t runs;
    uint32_t parentAddr;
    std::string* name;
  } UCorePCB;
}

#endif
