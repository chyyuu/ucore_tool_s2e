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
	S2E_DEFINE_PLUGIN(UCoreMemoryManagement, "MemoryManagement of UCore", "UCoreMemoryManagement", "UCoreMonitor");
	void UCoreMemoryManagement::initialize(){
		count = 0;
		((UCoreMonitor *)s2e()->getPlugin("UCoreMonitor"))->onFunctionTransition.connect(
				sigc::mem_fun(*this, &UCoreMemoryManagement::onPmminitTransition));
		// UCoreMonitor *um = (UCoreMonitor *)s2e()->getPlugin("UCoreMonitor");
		// um->UCoreMapPrint();
		// um->onFunctionTransition.connect(sigc::mem_fun(*this, &UCoreMemoryManagement::onPmminitTransition));
		// s2e()->getDebugStream() << um << '\n';
		// s2e()->getCorePlugin()->onTranslateInstructionStart.connect(
		//		sigc::mem_fun(*this, &UCoreMemoryManagement::onTranslateInstruction));
	}
	/*
	 * void UCoreMemoryManagement::onTranslateInstruction(ExecutionSignal *signal, S2EExecutionState *state,
	 * TranslationBlock *tb, uint64 pc){
	 *	count ++;
	 *	if (count == 1000){
	 *		s2e()->getDebugStream() << "###test 100!" << '\n';
	 *		count = 0;
	 *	}
	} */
	void UCoreMemoryManagement::onPmminitTransition(S2EExecutionState *state, string fname, uint64_t pc){
		string name = "pmm_init";
		//s2e()->getDebugStream() << "*****function:" << fname << '\n';
		if (fname.compare(name) == 0){
			++count;
			if (count == 2){
				onPmmDone.emit(state, pc);
				s2e()->getDebugStream() << "#####Memory:page created." << '\n';
			}
		}
	}
}
}
