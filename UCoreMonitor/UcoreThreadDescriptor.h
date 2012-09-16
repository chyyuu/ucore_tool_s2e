#ifndef _UCORE_THREAD_DESCRIPTOR_H_

#define _UCORE_THREAD_DESCRIPTOR_H_


namespace s2e {

  struct UCoreThreadDescriptor {
    uint64_t KernelStackBottom;
    uint64_t KernelStackSize;

    bool KernelMode;
    //TODO: add other interesting information

    UCoreThreadDescriptor() {
      KernelStackBottom = 0;
      KernelStackSize = 0;
      KernelMode = false;
    }
  };

}

#endif
