#ifndef S2E_PLUGINS_UCOREMEMORYMANAGEMENT_H
#define S2E_PLUGINS_UCOREMEMORYMANAGEMENT_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

#include "UCoreMonitor.h"

namespace s2e{
namespace plugins{
	class UCoreMemoryManagement : public Plugin{
		S2E_PLUGIN
	public:
		UCoreMemoryManagement(S2E *s2e) :Plugin(s2e){}
		void initialize();
		typedef sigc::signal<void, S2EExecutionState*, uint64_t> PmmSignal;
		PmmSignal onPmmDone;
		
		// emit signal when pmm_init done
		void onPmminitTransition(S2EExecutionState *state, std::string fname, uint64_t pc);

		// get allocPage pointer
		Page *getAllocPage(S2EExecutionState *state, std::string fname, uint64_t pc);

		// get freePage Info
		void getFreepageInfo(S2EExecutionState *state, std::string fname, uint64_t pc);

		// print Pagedir
		void print_pgdir(){
		}

		//for test
		void onTranslateInstruction(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb, uint64_t pc);
	private:
		int count;
		struct list_entry{
			struct list_entry *prev, *next;
		};
		typedef list_entry list_entry_t;
		struct Page{
			int ref;
			uint32_t flags;
			unsigned int property;
			int zone_num;
			list_entry_t page_link;
			list_entry_t pra_page_link;
			uint32_t pra_vaddr;
		};
	};
}
}

#endif
