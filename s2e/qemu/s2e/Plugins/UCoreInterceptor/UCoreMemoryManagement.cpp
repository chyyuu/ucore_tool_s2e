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
#define VPTADDR 0xc011961c
#define VPDADDR 0xc0119620
#define PMMINITEXIT 0xc0100086

        S2E_DEFINE_PLUGIN(UCoreMemoryManagement, "MemoryManagement of UCore",
                        "UCoreMemoryManagement", "UCoreMonitor");
        void UCoreMemoryManagement::initialize(){
                printPc = s2e()->getConfig()->getInt(getConfigKey() + ".print_pgdir_pc");

                ((UCoreMonitor *)s2e()->getPlugin("UCoreMonitor"))->onFunctionTransition.connect(
                                sigc::mem_fun(*this, &UCoreMemoryManagement::sortByFname));

                 s2e()->getCorePlugin()->onTranslateInstructionStart.connect(
                                sigc::mem_fun(*this, &UCoreMemoryManagement::print_pgdir));
        }
        /*	//for test
         *
         * void UCoreMemoryManagement::onTranslateInstruction(ExecutionSignal *signal,
         *  S2EExecutionState *state, TranslationBlock *tb, uint64 pc){
         *	count ++;
         *	if (count == 1000){
         *		s2e()->getDebugStream() << "###test 100!" << '\n';
         *		count = 0;
         *	}
        } */

        void UCoreMemoryManagement::sortByFname(ExecutionSignal *signal, S2EExecutionState *state,
                        string fname, uint64_t pc){
                //s2e()->getDebugStream() << "#Function:" << fname << " received\n";
                if (fname == "page_init")
                        signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::onPmminitTransition));
                else if (fname == "alloc_pages")
                        signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::getAllocPage));
                else if (fname == "free_pages")
                        signal->connect(sigc::mem_fun(*this, &UCoreMemoryManagement::getFreepageInfo));
//		else if (fname == "default_check")
//			s2e()->getDebugStream() << "!!!!!!!!!!\n";
//		else
//			signal->disconnect();
        }

        void UCoreMemoryManagement::onPmminitTransition(S2EExecutionState *state, uint64_t pc){
//		s2e()->getDebugStream() << "#Function:page_init received\n";	//test
                ++pmmInitDone;
                if (pmmInitDone == 2){
                        onPmmDone.emit(state, pc);
//			s2e()->getDebugStream() << "#####Memory:page created." << '\n';
                }
                else if (pmmInitDone > 2){
                        s2e()->getDebugStream() << "#####error pmmInit call/ret "
                                        << pmmInitDone << " times\n";
                }
        }

        void UCoreMemoryManagement::getAllocPage(S2EExecutionState *state, uint64_t pc){
//		s2e()->getDebugStream() << "#Function:alloc_pages received\n";
                if (pmmInitDone == 2){
                        ++allocPageRet;
                        if (allocPageRet == 2){
                                uint64_t pageAddr = 0;

                                Page upage;

                                s2e()->getDebugStream() << "#####alloc_pages info:\n";

                                if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]), &pageAddr, 4))
                                        s2e()->getDebugStream() << "#####get allocpage address fail\n";
                                if (pc >= 0x00010000 && pc <= 0x3fffffff)
                                        pageAddr -= 0xc0000000;

//				char buf[9];
//				sprintf(buf, "%08lx", pageAddr);
//				s2e()->getDebugStream() << "#####pageAddr:" << buf << '\n';

                                if (!state->readMemoryConcrete(pageAddr, &upage, sizeof(Page)))
                                        s2e()->getDebugStream() << "#####get allocpage struct fail\n";
                                print_pginfo(&upage);

//				allocPageRet = 0;		//enable print each alloc_page

//				sprintf(buf, "%08lx", pc);
//				s2e()->getDebugStream() << "#####pc:" << buf << '\n';
//				print_pgdir(NULL, state, NULL, printPc);
                        }
//			else if (allocPageRet == 1){
//				int allocPagen = 0;
//				uint64_t pAddr = 0;
//				char buf[9];
//				for (int i = 0;i != 2; ++i){
//				if (!state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_ESP]) + 4 * i, &pAddr, 4))
//					s2e()->getDebugStream() << "#####get esp fail\n";
//				sprintf(buf, "%08lx", pAddr);
//				s2e()->getDebugStream() << "#####pAddr:" << buf << "\n";
//				if (pc >= 0x00010000 && pc <= 0x3fffffff)
//					pAddr -= 0xc0000000;
//				if (!state->readMemoryConcrete(pAddr, &allocPagen, 4))
//					s2e()->getDebugStream() << "#####get allocpage size fail\n";
//				s2e()->getDebugStream() << "#####alloc page size:" << allocPagen << "\n";
//				}
//			}
                }
        }

        void UCoreMemoryManagement::getFreepageInfo(S2EExecutionState *state, uint64_t pc){
//		s2e()->getDebugStream() << "#Function:free_pages received\n";
        }

        void UCoreMemoryManagement::print_pgdir(ExecutionSignal *signal,
                         S2EExecutionState *state, TranslationBlock *tb, uint64_t pc){
                uint64_t vpc = pc;
                if (pc >= 0x00010000 && pc <= 0x3fffffff)
                        vpc += 0xc0000000;
                if (vpc == printPc){
                        if (pmmInitDone != 2){
                                s2e()->getDebugStream() << "pmm not init!\n";
                                return;
                        }

                        uint32_t *vpd, *vpt;
                        uint64_t vptaddr = VPTADDR;
                        state->readMemoryConcrete(vptaddr, &vpt, 4);
                        uint64_t vpdaddr = VPDADDR;
                        state->readMemoryConcrete(vpdaddr, &vpd, 4);
                        char buf1[9], buf2[9], buf3[9], buf4[9];

//			if (++pmmInitDone == 1)
                        s2e()->getDebugStream() << "#####print_pgdir\n";
                        s2e()->getDebugStream() << "-------------------- BEGIN --------------------\n";
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
                        s2e()->getDebugStream() << "--------------------- END ---------------------\n";
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
            while (start < right){// && !(table[start] & PTE_P)) {
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
                int perm = temp & PTE_USER;//(table[start ++] & PTE_USER);
                while (start < right){// && (table[start] & PTE_USER) == perm) {
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
