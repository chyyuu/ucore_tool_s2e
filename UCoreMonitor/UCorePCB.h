#ifndef _UCORE_THREAD_DESCRIPTOR_H_

#define _UCORE_THREAD_DESCRIPTOR_H_

#include <inttypes.h>
#define PROC_NAME_LEN               15

namespace s2e {

  enum proc_state{
    PROC_UNINIT = 0,
    PROC_SLEEPING,
    PROC_RUNNABLE,
    PROC_ZOMBIE
  };

  typedef struct _UCorePCB{
    enum proc_state state;
    uint32_t pid;
    uint32_t runs;
    struct _UCorePCB* parent;
    char name[PROC_NAME_LEN + 1];
  }UCorePCB;
}

#endif
