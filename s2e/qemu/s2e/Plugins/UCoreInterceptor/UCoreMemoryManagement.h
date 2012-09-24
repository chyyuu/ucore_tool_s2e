#ifndef S2E_PLUGINS_UCOREMEMORYMANAGEMENT_H
#define S2E_PLUGINS_UCOREMEMORYMANAGEMENT_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

#include "UCoreMonitor.h"

namespace s2e{
namespace plugins{
//	struct list_entry{
//		struct list_entry *prev, *next;
//	};
//	typedef list_entry list_entry_t;
	struct Page{
		int ref;
		uint32_t flags;
		unsigned int property;
		int zone_num;
//		list_entry_t page_link;
//		list_entry_t pra_page_link;
		uint64_t page_link;
		uint64_t pra_page_link;
		uint32_t pra_vaddr;
	};

	class UCoreMemoryManagement : public Plugin{
		S2E_PLUGIN
	private:
			int pmmInitDone;
			int allocPageRet;
			uint64_t printPc;
			Page *allocpage;
	public:
		UCoreMemoryManagement(S2E *s2e) :Plugin(s2e), pmmInitDone(0),
			allocPageRet(0), allocpage(NULL){}
		void initialize();
		typedef sigc::signal<void, S2EExecutionState*, uint64_t> PmmSignal;
		PmmSignal onPmmDone;
		
		void sortByFname(ExecutionSignal* signal, S2EExecutionState *state,
				std::string fname, uint64_t pc);

		// emit signal when pmm_init done
		void onPmminitTransition(S2EExecutionState *state, uint64_t pc);

		// get allocPage pointer
		void getAllocPage(S2EExecutionState *state, uint64_t pc);

		// get freePage Info
		void getFreepageInfo(S2EExecutionState *state, uint64_t pc);

		// print Pagedir
		void print_pgdir(ExecutionSignal *signal,
				 S2EExecutionState *state, TranslationBlock *tb, uint64 pc);

		int get_pgtable_items(S2EExecutionState *state, uint32_t left, uint32_t right, uint32_t start,
				uint32_t *table, uint32_t *left_store, uint32_t *right_store);

		char* perm2str(int perm);

		void print_pginfo(Page *page);
		//for test
		void onTranslateInstruction(ExecutionSignal *signal, S2EExecutionState *state,
				TranslationBlock *tb, uint64_t pc);
	};
}
}

#endif
