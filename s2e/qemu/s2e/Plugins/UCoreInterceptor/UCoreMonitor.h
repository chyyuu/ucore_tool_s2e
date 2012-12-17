#ifndef _UCORE_MONITOR_H
#define _UCORE_MONITOR_H


#include "UCoreStab.h"
#include "UCorePCB.h"
#include "UCoreFunc.h"
#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/OSMonitor.h>

#include <inttypes.h>
#include <vector>
#include <map>
#include <set>
#include <string>

namespace s2e{
  namespace plugins{

    class UCoreMonitor : public OSMonitor{
      S2E_PLUGIN
      public:
      UCoreMonitor(S2E *s2e) :OSMonitor(s2e){}
      virtual ~UCoreMonitor();
      void initialize();
      /*-------------signals------------------*/
      /* For funtion monitor */
      typedef sigc::signal<void, ExecutionSignal *, S2EExecutionState*, std::string, uint64_t> TransitionSignal;
      TransitionSignal onFunctionTransition;
      TransitionSignal onFunctionCalling;
      TransitionSignal onFunctionReturning;

      /* for thread monitor */
      typedef sigc::signal<void, S2EExecutionState*, UCorePCB*, UCorePCB*, uint64_t> ThreadSwitchSignal;
      typedef sigc::signal<void, S2EExecutionState*, UCorePCB*, uint64_t> ThreadSignal;
      ThreadSwitchSignal onThreadSwitching;
      ThreadSignal onThreadCreating;
      ThreadSignal onThreadExiting;

      void slotCall(S2EExecutionState* state, uint64_t pc);
      void slotRet(S2EExecutionState* state, uint64_t pc);
      void disconnect(S2EExecutionState *state){
        return;
      }
      void printHelloWorld(void);
      void printPanicInfo(S2EExecutionState* state);

    private:
      //mointor controllers
      bool m_MonitorThreads;
      bool m_MonitorFunction;
      bool m_MonitorPanic;

      //Symbol table
      typedef struct __symbol_struct{
        uint64_t addr;
        char type;
        std::string name;
      } symbol_struct;
      typedef std::map<uint64_t, symbol_struct> SymbolTable;
      typedef std::map<std::string, uint64_t> SymbolMap;

      std::string system_map_file;
      std::string kernel_ld_file;

      //UCore System Map
      SymbolMap sMap;
      SymbolTable sTable;
      //Stab Array
      UCoreStab* stab_array;
      UCoreStab* stab_array_end;
      char* stabstr_array;
      char* stabstr_array_end;

      //Kernel Addresses
      uint64_t m_KernelBase;
      uint64_t m_KeCurrentThread;
      uint64_t m_KeNrProcess;
      uint64_t m_KePCBLinkedList;
      //STAB Related Address
      uint64_t m_StabStart;
      uint64_t m_StabEnd;
      uint64_t m_StabStrStart;
      uint64_t m_StabStrEnd;
      bool stabParsed;

      //Indicating the first instructions
      //To get the chance of parsing STAB file
      bool first;
      bool paniced;
      int range;

      //Signal connectors
      void onPageDirectoryChange(S2EExecutionState *state,
                                 uint64_t previous,
                                 uint64_t current);
      void onTranslateBlockEnd(ExecutionSignal* signal, S2EExecutionState *state,
                               TranslationBlock *tb, uint64_t pc,
                               bool, uint64_t);
      void onTBJumpStart (ExecutionSignal *signal, S2EExecutionState *state,
                          TranslationBlock *tb, uint64_t, int jump_type);

      // signal slot functions
      void slotFunctionCalling(ExecutionSignal *signal,
                               S2EExecutionState *state,
                               std::string fname, uint64_t pc);
      void slotKmThreadInit(S2EExecutionState *state, uint64_t pc);
      void slotKmThreadExit(S2EExecutionState *state, uint64_t pc);
      void slotKmThreadSwitch(S2EExecutionState *state, uint64_t pc);
      void PanicMonitor(S2EExecutionState *state,
                        uint64_t pc);

      // Meta functions starts here

      // parse origin files
      void parseSystemMapFile();
      void parseKernelLd();

      //print functions
      void printUCorePCB(UCorePCB* ucorePCB);
      void printUCoreFunc(UCoreFunc func);
      void notifyLoadForAllThreads(S2EExecutionState* state);
      uint64_t getKernelStart() const;
      uint64_t getKeInitThread() const;
      uint64_t getKeTerminateThread() const;
      uint64_t getKeSwitchThread() const;

      // parse functions
      void parseUCoreStab(S2EExecutionState *state);
      int parseUCoreFunc(uint64_t addr, UCoreFunc* func);
      void stab_binsearch(UCoreStab* stabs, int* region_left,
                          int* region_right, int type, uint64_t addr);
      UCorePCB* parseUCorePCB(S2EExecutionState *state,
                              uint64_t addr);
      std::string* parseUCorePName(S2EExecutionState *state,
                                  uint64_t addr);
      std::string* parseUCorePNamePrint(S2EExecutionState *state,
                                  uint64_t addr);

      uint64_t getCurrentThread(S2EExecutionState *state);
      bool getImports(S2EExecutionState *s,
                      const ModuleDescriptor &desc,
                      Imports &I);
      bool getExports(S2EExecutionState *s,
                      const ModuleDescriptor &desc,
                      Exports &E);
      uint64_t getPid(S2EExecutionState *s,
                      uint64_t pc);
      bool getCurrentStack(S2EExecutionState *s,
                           uint64_t *base,
                           uint64_t *size);
      bool isKernelAddress(uint64_t pc) const;
    public:
      void printAllThreads(S2EExecutionState* state);
      uint32_t parseNrProcess(S2EExecutionState* state);
      UCorePCB** parseUCorePCBLinkedList(S2EExecutionState* state,
                                         uint32_t n);
    };// class UCoreMonitor

    class UCoreMonitorState: public PluginState{
    private:
      uint64_t m_CurrentPid;

    public:
      UCoreMonitorState();
      virtual ~UCoreMonitorState();
      virtual UCoreMonitorState* clone() const;
      static PluginState *factory(Plugin *p, S2EExecutionState *state);

      friend class UCoreMonitor;
    };// class UCoreMonitorState

  }// plugins
}// s2e

#endif
