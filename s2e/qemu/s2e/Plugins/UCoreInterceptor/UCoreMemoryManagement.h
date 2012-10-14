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
		uint64_t page_link;		//TODO
		uint64_t pra_page_link;		//TODO
		uint32_t pra_vaddr;
	};

	typedef struct kmem_cache_s kmem_cache_t;
	struct kmem_cache_s {
	    uint64_t slabs_full;     //TODO list for fully allocated slabs
	    uint64_t slabs_notfull;  //TODO list for not-fully allocated slabs

	    uint32_t objsize;              // the fixed size of obj
	    uint32_t num;                  // number of objs per slab
	    uint32_t offset;               // this first obj's offset in slab
	    bool off_slab;               // the control part of slab in slab or not.

	    /* order of pages per slab (2^n) */
	    uint32_t page_order;

	    uint32_t slab_cachep;
	};

	typedef struct slab_s {
	    uint64_t slab_link; // the list entry linked to kmem_cache list
	    uint32_t s_mem;            // the kernel virtual address of the first obj in slab
	    uint32_t inuse;           // the number of allocated objs
	    uint32_t offset;          // the first obj's offset value in slab
	    uint32_t free;     // the first free obj's index in slab
	} slab_t;

	class UCoreMemoryManagement : public Plugin{
		S2E_PLUGIN
	private:
			int pmmInitDone;
			int allocPageRet;
			int freePageCall;
			int kmallocSize;
			int kmallocInfo;
			Page *allocpage;
			kmem_cache_t *kmallocCache;
			slab_t *kmallocSlab;
			bool enable_paging;

			uint64_t printPc;
			uint64_t vptaddr;
			uint64_t vpdaddr;
	public:
		UCoreMemoryManagement(S2E *s2e) :Plugin(s2e), pmmInitDone(0),
			allocPageRet(0), freePageCall(0), kmallocSize(0), kmallocInfo(0), allocpage(NULL), enable_paging(false){}
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

		// get kmalloc page size
		void getKmallocSize(S2EExecutionState *state, uint64_t pc);

		// get kmalloc page info
		void getKmallocInfo(S2EExecutionState *state, uint64_t pc);

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
