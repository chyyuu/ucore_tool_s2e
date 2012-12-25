
extern "C" {
#include "config.h"
#include "qemu-common.h"
}

#include "UCoreFunctionMonitor.h"
#include "UCoreProcessMonitor.h"

#include <string>
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

using namespace std;
using namespace s2e;
using namespace s2e::plugins;

S2E_DEFINE_PLUGIN(UCoreProcessMonitor,
                  "Monitor of UCore Process",
                  "", "UCoreFunctionMonitor");

UCoreProcessMonitor::~UCoreProcessMonitor(){
}

void UCoreProcessMonitor::initialize(){
  utils = (UCoreUtils *)s2e()->getPlugin("UCoreUtils");
  ((UCoreFunctionMonitor *)s2e()->getPlugin("UCoreFunctionMonitor"))
    ->onFunCall
    .connect(sigc::mem_fun(*this, &UCoreProcessMonitor::slotFunCall));
}

void UCoreProcessMonitor::slotFunCall(S2EExecutionState *state, uint64_t pc){
  uint64_t vpc = state->getPc();
  if(utils->Addr2Sym.find(vpc) == utils->Addr2Sym.end()){
    // s2e()->getWarningsStream() << "Not Found Function @ ";
    // s2e()->getWarningsStream().write_hex(pc);
    // s2e()->getWarningsStream() << "\n";
    return;
  }else{
    string name = utils->Addr2Sym[vpc];
    if(name == "set_proc_name"){
    }else if(name == ""){
    }else if(name == ""){
    }
  }
}

