#ifndef _UCORE_UTILS_H
#define _UCORE_UTILS_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/Plugins/FunctionMonitor.h>
#include <s2e/S2EExecutionState.h>

#include <map>

using namespace std;

namespace s2e{
  namespace plugins{
    class UCoreUtils : public Plugin{
      S2E_PLUGIN
      public:
      UCoreUtils(S2E *s2e) : Plugin(s2e){}
      virtual ~UCoreUtils();
      void initialize();
    public:
      /* -----------------public utils------------------- */
      string* parseFunNameFromEntry(uint64_t pc);
      UCoreInst* parseInfoFromPc(uint64_t pc);
      UCorePCB* parsePCBFromAddr(uint64_t addr);
      map<uint64_t, string> Addr2Sym;
      map<string, uint64_t> Sym2Addr;
    private:
      /* -----------------Parse Funcs------------------- */
      bool first;
      //System Map Related
      string system_map_file;
      void parseSymbolMap();
      //Stab Segment Related
      uint64_t m_StabStart;
      uint64_t m_StabEnd;
      uint64_t m_StabStrStart;
      uint64_t m_StabStrEnd;
      void parseUCoreStab(S2EExecutionState *state);
      void stab_binsearch(UCoreStab* stabs, int* region_left,
                          int* region_right, int type, uint64_t addr);

    }; //class UCU
  } // plugins
} // s2e

#endif
