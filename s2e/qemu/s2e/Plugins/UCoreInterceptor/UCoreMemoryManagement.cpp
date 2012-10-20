extern "C" {
#include "config.h"
#include "qemu-common.h"
}

#include "UCoreMemoryManagement.h"

#include <iostream>
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

#include <string>

using namespace std;

namespace s2e{
namespace plugins{

#define PTE_P 0x001
#define PTE_W 0x002
#define PTE_U 0x004
#define PTE_USER 0x007
#define NPDEENTRY 1024
#define PTSIZE 4096 * 1024
#define NPTEENTRY 1024
#define PGSIZE 4096
//#define VPTADDR 0xc0116548
//#define VPDADDR 0xc011654c
//#define S2EESPOFFSET 0x4

	S2E_DEFINE_PLUGIN(UCoreMemoryManagement, "MemoryManagement of UCore",
			"UCoreMemoryManagement", "UCoreMonitor");
	void UCoreMemoryManagement::initialize(){
		printPc = s2e()->getConfig()->getInt(getConfigKey() + ".print_pgdir_pc");
		vptaddr = s2e()->getConfig()->getInt(getConfigKey() + ".vptaddr");
		vpdaddr = s2e()->getConfig()->getInt(getConfigKey() + ".vpdaddr");

		((UCoreMonitor *)s2e()->getPlugin("UCoreMonitor"))->onFunctionTransition.connect(
				sigc::mem_fun(*this, &UCoreMemoryManagement::sortByFname));

		 s2e()->getCorePlugin()->onTranslateInstructionStart.connect(
				sigc::mem_fun(*this, &UCoreMemoryManagement::print_pgdir));
	}

	void UCoreMemoryManagement::sortByFname(ExecutionSignal *signal, S2EExecutionState *state,
			string fname, uint64_t pc){
		//s2e()->getDebugStream() << "#Function:" << fname << " received\n";

		if (fname == "page_init")
			signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::onPmminitTransition));
		else if (fname == "alloc_pages")
			signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::getAllocPage));
		else if (fname == "free_pages")
			signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::getFreepageInfo));
		else if (fname == "kmalloc")
			signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::getKmallocSize));
		else if (fname == "kmem_cache_alloc_one")
			signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::getKmallocInfo));
		else if (fname == "kfree")
			signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::getKfreeobjp));
		else if (fname == "kmem_cache_free_one")
			signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::getKfreeInfo));
//		else
//			signal->disconnect();
	}

	void UCoreMemoryManagement::onPmminitTransition(S2EExecutionState *state, uint64_t pc){

		++pmmInitDone;

//		char buf[9];
//		sprintf(buf, "%08lx", pc);
//		s2e()->getDebugStream() << buf << '\n';

		if (pmmInitDone == 2){
			onPmmDone.emit(state, pc);
		}
		else if (pmmInitDone > 2){
			s2e()->getDebugStream() << "#####error pmmInit call/ret "
					<< pmmInitDone << " times\n";
		}
	}

	void UCoreMemoryManagement::getAllocPage(S2EExecutionState *state, uint64_t pc){
		if (pmmInitDone == 2){
			++allocPageCall;
			if (allocPageCall == 1){
				s2e()->getDebugStream() << "----------alloc_pages info BEGIN----------\n";

				int allocPageSize = 0;
				uint64_t pAddr = 0;

				if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ESP]), &pAddr, 4))
					s2e()->getDebugStream() << "#####get esp fail\n";
				pAddr += 0x4;

//				char buf[9];
//				sprintf(buf, "%08lx", pAddr);
//				s2e()->getDebugStream() << "#####pAddr:" << buf << "\n";
//				sprintf(buf, "%08lx", pc);
//				s2e()->getDebugStream() << "#####p:" << buf << "\n";

				if (enable_paging == false)
					pAddr -= 0xc0000000;
				if (!state->readMemoryConcrete(pAddr, &allocPageSize, 4))
					s2e()->getDebugStream() << "#####get allocpage size fail\n";
				s2e()->getDebugStream() << "#####alloc page size:" << allocPageSize << "\n";
//				}
			}
			else if (allocPageCall == 2){

				uint64_t pageAddr = 0;
				Page apage;

				if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]), &pageAddr, 4))
					s2e()->getDebugStream() << "#####get allocpage address fail\n";
				if (enable_paging == false)
					pageAddr -= 0xc0000000;

//				char buf[9];
//				sprintf(buf, "%08lx", pageAddr);
//				s2e()->getDebugStream() << "#####pageAddr:" << buf << '\n';

				if (!state->readMemoryConcrete(pageAddr, &apage, sizeof(Page)))
					s2e()->getDebugStream() << "#####get Page struct fail\n";
				print_pginfo(&apage);

				s2e()->getDebugStream() << "----------alloc_pages info END----------\n";

//				allocPageCall = 0;		//enable print each alloc_page
			}
		}
	}

	void UCoreMemoryManagement::getFreepageInfo(S2EExecutionState *state, uint64_t pc){
		if (pmmInitDone == 2){
			++freePageCall;
			if (freePageCall == 1){
				s2e()->getDebugStream() << "----------free_pages info BEGIN----------\n";

				uint64_t pAddr = 0;
				uint64_t fAddr = 0;
				Page fpage;
				int freePageSize;

				if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ESP]), &pAddr, 4))
					s2e()->getDebugStream() << "#####get esp fail\n";
				pAddr += 0x4;
				pAddr += 0x4;
				if (enable_paging == false)
					pAddr -= 0xc0000000;
				if (!state->readMemoryConcrete(pAddr, &freePageSize, 4))
					s2e()->getDebugStream() << "#####get freepage size fail\n";
				s2e()->getDebugStream() << "#####free page size:" << freePageSize << "\n";

//				char buf[9];
//				sprintf(buf, "%08lx", pAddr);
//				s2e()->getDebugStream() << "#####pAddr:" << buf << "\n";
//				sprintf(buf, "%08lx", pc);
//				s2e()->getDebugStream() << "#####p:" << buf << "\n";

				pAddr -= 0x4;

				if (!state->readMemoryConcrete(pAddr, &fAddr, 4))
					s2e()->getDebugStream() << "#####get freepage addr fail\n";
				if (enable_paging == false)
					fAddr -= 0xc0000000;
				if (!state->readMemoryConcrete(fAddr, &fpage, sizeof(Page)))
					s2e()->getDebugStream() << "#####get Page struct fail\n";

//				char buf[9];
//				sprintf(buf, "%08lx", pAddr);
//				s2e()->getDebugStream() << "#####pAddr:" << buf << "\n";
//				sprintf(buf, "%08lx", pc);
//				s2e()->getDebugStream() << "#####p:" << buf << "\n";

				print_pginfo(&fpage);

				s2e()->getDebugStream() << "----------free_pages info END----------\n";
			}
			else{
//				freePageCall = 0;		//enable print each freepage
			}
		}
	}

	void UCoreMemoryManagement::getKmallocSize(S2EExecutionState *state, uint64_t pc){
		if (pmmInitDone == 2){
			++kmallocCall;
			if (kmallocCall == 1){
				s2e()->getDebugStream() << "----------kmalloc info BEGIN----------\n";
				int kmallocSize = 0;
				uint64_t kAddr = 0;

				if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ESP]), &kAddr, 4))
					s2e()->getDebugStream() << "#####get esp fail\n";
				kAddr += 0x4;
//				if (enable_paging == false)
//					kAddr -= 0xc0000000;
				if (!state->readMemoryConcrete(kAddr, &kmallocSize, 4))
					s2e()->getDebugStream() << "#####get kmalloc size fail\n";
				s2e()->getDebugStream() << "#####kmalloc page size:" << kmallocSize << "\n";
			}
			else if (kmallocCall == 2){
				uint64_t objAddr = 0;
				if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]), &objAddr, 4))
					s2e()->getDebugStream() << "#####get kmalloc objp address fail\n";
				char buf[9];
				sprintf(buf, "%08lx", objAddr);
				s2e()->getDebugStream() << "#####objAddr:" << buf << '\n';
				s2e()->getDebugStream() << "----------kmalloc info END----------\n";

//				kmallocCall = 0;	//enable print each kmalloc pagesize;
			}
		}
	}

	void UCoreMemoryManagement::getKmallocInfo(S2EExecutionState *state, uint64_t pc){
		if (pmmInitDone == 2){
			++kmallocInfoCall;
			if (kmallocInfoCall == 1){
				uint64_t kAddr = 0;
//				char buf[9];
//				sprintf(buf, "%08lx", pc);
//				s2e()->getDebugStream() << "#####pc:" << buf << '\n';

				if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ESP]), &kAddr, 4))
					s2e()->getDebugStream() << "#####get esp fail\n";
				kAddr += 0x4;
				uint64_t kmemCacheAddr = 0;
				if (!state->readMemoryConcrete(kAddr, &kmemCacheAddr, 4))
					s2e()->getDebugStream() << "#####get kmem_cache_t addr fail\n";
				kmem_cache_t kmemCache;
				if (!state->readMemoryConcrete(kmemCacheAddr, &kmemCache, sizeof(kmem_cache_t)))
					s2e()->getDebugStream() << "#####get kmem_cache_t struct fail\n";

				s2e()->getDebugStream() << "kmem_cache_t objsize:" << kmemCache.objsize << '\n';

				kAddr += 0x4;
				uint64_t kmemSlabAddr = 0;
				if (!state->readMemoryConcrete(kAddr, &kmemSlabAddr, 4))
					s2e()->getDebugStream() << "#####get slab_t addr fail\n";
				slab_t kmemSlab;
				if (!state->readMemoryConcrete(kmemSlabAddr, &kmemSlab, sizeof(slab_t)))
					s2e()->getDebugStream() << "#####get slab_t struct fail\n";

				char buf[9];
				sprintf(buf, "%x", kmemSlab.s_mem);
				s2e()->getDebugStream() << "slab_t mem:" << buf << '\n';
				s2e()->getDebugStream() << "slab_t free:" << kmemSlab.free << '\n';
			}
			else if (kmallocInfoCall == 2){
//				kmallocInfoCall = 0;		//enable print each kmalloc info;
			}
		}
	}

	void UCoreMemoryManagement::getKfreeobjp(S2EExecutionState *state, uint64_t pc){
		if (pmmInitDone == 2){
			++kfreeCall;
			if (kfreeCall == 1){
				s2e()->getDebugStream() << "----------kfree info BEGIN----------\n";
				uint64_t kfreeobjp = 0;
				uint64_t kAddr = 0;

				if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ESP]), &kAddr, 4))
					s2e()->getDebugStream() << "#####get esp fail\n";
				kAddr += 0x4;
//				if (enable_paging == false)
//					kAddr -= 0xc0000000;
				if (!state->readMemoryConcrete(kAddr, &kfreeobjp, 4))
					s2e()->getDebugStream() << "#####get kfree objp addr fail\n";
				char buf[9];
				sprintf(buf, "%08lx", kfreeobjp);
				s2e()->getDebugStream() << "#####kfree objp addr:" << buf << "\n";
			}
			else if (kfreeCall == 2){
				s2e()->getDebugStream() << "----------kfree info END----------\n";

//				kfreeCall = 0;	//enable print each kmalloc pagesize;
			}
		}
	}

	void UCoreMemoryManagement::getKfreeInfo(S2EExecutionState *state, uint64_t pc){
			if (pmmInitDone == 2){
				++kfreeInfoCall;
				if (kfreeInfoCall == 1){
					uint64_t kAddr = 0;
	//				char buf[9];
	//				sprintf(buf, "%08lx", pc);
	//				s2e()->getDebugStream() << "#####pc:" << buf << '\n';

					if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ESP]), &kAddr, 4))
						s2e()->getDebugStream() << "#####get esp fail\n";
					kAddr += 0x4;
					uint64_t kmemCacheAddr = 0;
					if (!state->readMemoryConcrete(kAddr, &kmemCacheAddr, 4))
						s2e()->getDebugStream() << "#####get kmem_cache_t addr fail\n";
					kmem_cache_t kmemCache;
					if (!state->readMemoryConcrete(kmemCacheAddr, &kmemCache, sizeof(kmem_cache_t)))
						s2e()->getDebugStream() << "#####get kmem_cache_t struct fail\n";

					s2e()->getDebugStream() << "kmem_cache_t objsize:" << kmemCache.objsize << '\n';

					kAddr += 0x4;
					uint64_t kmemSlabAddr = 0;
					if (!state->readMemoryConcrete(kAddr, &kmemSlabAddr, 4))
						s2e()->getDebugStream() << "#####get slab_t addr fail\n";
					slab_t kmemSlab;
					if (!state->readMemoryConcrete(kmemSlabAddr, &kmemSlab, sizeof(slab_t)))
						s2e()->getDebugStream() << "#####get slab_t struct fail\n";

					char buf[9];
					sprintf(buf, "%x", kmemSlab.s_mem);
					s2e()->getDebugStream() << "slab_t mem:" << buf << '\n';
					s2e()->getDebugStream() << "slab_t free:" << kmemSlab.free << '\n';
				}
				else if (kfreeInfoCall == 2){
	//				kfreeInfoCall = 0;		//enable print each kmalloc info;
				}
			}
		}

	void UCoreMemoryManagement::print_pgdir(ExecutionSignal *signal,
			 S2EExecutionState *state, TranslationBlock *tb, uint64 pc){
		uint64_t vpc = pc;
		if (enable_paging == false)
			vpc += 0xc0000000;

		if (enable_paging == false){
			uint32_t cr0;
			cr0 = state->readCpuState(CPU_OFFSET(cr[0]), 8*sizeof(target_ulong));
			if ((cr0 & 0x80000000) != 0)
				enable_paging = true;
		}

		if (vpc == printPc){
			if (pmmInitDone != 2){
				s2e()->getDebugStream() << "pmm not init!\n";
				return;
			}

			uint32_t *vpd, *vpt;
//			uint64_t vptaddr = VPTADDR;
			if (enable_paging == false)
				vptaddr -= 0xc0000000;
			state->readMemoryConcrete(vptaddr, &vpt, 4);
//			uint64_t vpdaddr = VPDADDR;
			if (enable_paging == false)
				vpdaddr -= 0xc0000000;
			state->readMemoryConcrete(vpdaddr, &vpd, 4);
			char buf1[9], buf2[9], buf3[9], buf4[9];

			s2e()->getDebugStream() << "#####print_pgdir\n";
			s2e()->getDebugStream() << "----------print_pgdir BEGIN----------\n";
			uint32_t left, right = 0, perm;
			while ((perm = get_pgtable_items(state, 0, NPDEENTRY, right, vpd, &left, &right)) != 0) {
				printf("PDE(%03x) %08x-%08x %08x %s\n", right - left,
						left * PTSIZE, right * PTSIZE, (right - left) * PTSIZE, perm2str(perm));
				sprintf(buf1, "%03x", right - left);
				sprintf(buf2, "%08x", left * PTSIZE);
				sprintf(buf3, "%08x", right * PTSIZE);
				sprintf(buf4, "%08x", (right - left) * PTSIZE);

				s2e()->getDebugStream() << "PDE(" <<  buf1 << ")" << buf2 << "-"
						<< buf3 << " " << buf4 << perm2str(perm) << '\n';
				uint32_t l, r = left * NPTEENTRY;
				while ((perm = get_pgtable_items(state, left * NPTEENTRY, right * NPTEENTRY, r, vpt, &l, &r)) != 0) {
					printf("  |-- PTE(%05x) %08x-%08x %08x %s\n", r - l,
							l * PGSIZE, r * PGSIZE, (r - l) * PGSIZE, perm2str(perm));
					sprintf(buf1, "%05x", r - l);
					sprintf(buf2, "%08x", l * PGSIZE);
					sprintf(buf3, "%08x", r * PGSIZE);
					sprintf(buf4, "%08x", (r - l) * PGSIZE);

					s2e()->getDebugStream() << "|-- PTE(" <<  buf1 << ")" << buf2 << "-"
							<< buf3 << " "<< buf4 << perm2str(perm) << '\n';
				}
			}
			s2e()->getDebugStream() << "----------print_pgdir END----------\n";
		}
	}

	char* UCoreMemoryManagement::perm2str(int perm) {
	    static char str[4];
	    str[0] = (perm & PTE_U) ? 'u' : '-';
	    str[1] = 'r';
	    str[2] = (perm & PTE_W) ? 'w' : '-';
	    str[3] = '\0';
	    return str;
	}

	int UCoreMemoryManagement::get_pgtable_items(S2EExecutionState *state, uint32_t left,
			uint32_t right, uint32_t start, uint32_t *table, uint32_t *left_store, uint32_t *right_store) {
	    if (start >= right) {
	        return 0;
	    }
	    uint64_t addr;
	    uint32_t temp;
	    while (start < right){
	    	addr = (uint64_t)&table[start];
	    	state->readMemoryConcrete(addr, &temp, 4);
	    	if (temp & PTE_P)
	    		break;
	        start ++;
	    }
	    if (start < right) {
	        if (left_store != NULL) {
	            *left_store = start;
	        }
	        addr = (uint64_t)&table[start ++];
	        state->readMemoryConcrete(addr, &temp, 4);
	        int perm = temp & PTE_USER;
	        while (start < right){
	        	addr = (uint64_t)&table[start];
	        	state->readMemoryConcrete(addr, &temp, 4);
	        	if ((temp & PTE_USER) != perm)
	        		break;
	            start ++;
	        }
	        if (right_store != NULL) {
	            *right_store = start;
	        }
	        return perm;
	    }
	    return 0;
	}

	void UCoreMemoryManagement::print_pginfo(Page *page){
		s2e()->getDebugStream() << "ref:" << page->ref << "\n"
								<< "flags:" << page->flags << "\n"
								<< "property:" << page->property << "\n"
								<< "zone_num:" << page->zone_num << "\n"
								<< "pra_vaddr:" << page->pra_vaddr << "\n";
	}

}
}
