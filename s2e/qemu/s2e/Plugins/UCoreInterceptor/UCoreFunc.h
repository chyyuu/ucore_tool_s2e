#ifndef __UCORE_FUNC_H_
#define __UCORE_FUNC_H_

#include <string>
#include <inttypes.h>

namespace s2e{
  typedef struct _UCoreFunc{
    std::string src_name;
    uint64_t fn_entry;
    std::string fn_name;
    uint16_t line_num;
  } UCoreFunc;
}

#endif
