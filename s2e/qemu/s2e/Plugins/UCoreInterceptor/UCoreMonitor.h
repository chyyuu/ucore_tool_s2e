#ifndef S2E_PLUGINS_UCOREMONITOR_H
#define S2E_PLUGINS_UCOREMONITOR_H

#include "UCorePCB.h"
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

    class UCoreUserModeEvent;
    class UCoreKernelModeEvent;

    class UCoreMonitor : public OSMonitor{

      S2E_PLUGIN

      public:

      UCoreMonitor(S2E *s2e) :OSMonitor(s2e){}
      virtual ~UCoreMonitor();
      void initialize();

      typedef sigc::signal<void, ExecutionSignal *, S2EExecutionState*, std::string, uint64_t> TransitionSignal;
      TransitionSignal onFunctionTransition;

      void slotCall(S2EExecutionState* state, uint64_t pc);
      void slotRet(S2EExecutionState* state, uint64_t pc);

      void disconnect(S2EExecutionState *state){
        return;
      }

    private:

      bool m_UserMode, m_KernelMode;
      bool m_MonitorThreads;
      bool m_MonitorFunction;
      uint64_t m_KernelBase;
      uint64_t m_KernelEnd;
      std::vector<uint64_t> callStack;

      //Symbol table
      typedef struct __symbol_struct{
        uint64_t addr;
        char type;
        std::string name;
      } symbol_struct;
      typedef std::map<uint64_t, symbol_struct> SymbolTable;
      std::string system_map_file;
      SymbolTable sTable;

      //Kernel Addresses
      static uint64_t s_KeInitThread;
      static uint64_t s_KeTerminateThread;

      //Signal connectors
      void onTranslateInstruction(ExecutionSignal *signal,
                                  S2EExecutionState *state,
                                  TranslationBlock *tb,
                                  uint64_t pc);
      void onPageDirectoryChange(S2EExecutionState *state,
                                 uint64_t previous,
                                 uint64_t current);
      void onTranslateBlockEnd(ExecutionSignal* signal, S2EExecutionState *state,
                               TranslationBlock *tb, uint64_t pc,
                               bool, uint64_t);
      void onTBJumpStart (ExecutionSignal *signal, S2EExecutionState *state,
                          TranslationBlock *tb, uint64_t, int jump_type);
      void onCustomInstruction(S2EExecutionState *state, uint64_t arg);
      //User Mode Events
      // void slotMonitorProcessSwitch(S2EExecutionState *state,
      //                               uint64_t pc);
      // void slotUmCatchProcessTermination(S2EExecutionState *state,
      //                                    uint64_t pc);

      //Kernel Mode Events
      void slotKmThreadInit(S2EExecutionState *state, uint64_t pc);
      void slotKmThreadExit(S2EExecutionState *state, uint64_t pc);

      // Meta functions starts here
      void parseSystemMapFile();
      void notifyLoadForAllThreads(S2EExecutionState* state);
      uint64_t getKernelStart() const;
      uint64_t getKeInitThread() const;
      uint64_t getKeTerminateThread() const;
      // bool getThreadDescriptor(S2EExecutionState* state,
      //                          uint64_t pThread,
      //                          UCoreThreadDescriptor threadDescriptor);
      uint64_t getCurrentThread(S2EExecutionState *state);
      bool getImports(S2EExecutionState *s, const ModuleDescriptor &desc, Imports &I);
      bool getExports(S2EExecutionState *s, const ModuleDescriptor &desc, Exports &E);
      bool isKernelAddress(uint64_t pc) const;
      uint64_t getPid(S2EExecutionState *s, uint64_t pc);
      bool getCurrentStack(S2EExecutionState *s, uint64_t *base, uint64_t *size);
      void uint2hexstring(uint64_t number, char* string, int size);

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
