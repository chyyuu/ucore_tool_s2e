/**
   UCore Monitor
   By Nuk, Tsinghua University.
*/

extern "C" {
#include "config.h"
#include "qemu-common.h"
}

#include "UCoreProfiler.h"
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

using namespace std;
using namespace s2e;
using namespace s2e::plugins;

S2E_DEFINE_PLUGIN(UCoreProfiler, "Profiler of UCore",
                    "UCoreProfiler", "UCoreMonitor");

UCoreProfiler::~UCoreProfiler(){
}

void UCoreProfiler::initialize(){
  UCoreMonitor* monitor = static_cast<UCoreMonitor*>
    (s2e()->getPlugin("UCoreMonitor"));
  assert(monitor);
  monitor->onThreadSwitching.
    connect(sigc::mem_fun(*this, &UCoreProfiler::slotThreadSwitch));
  monitor->onThreadCreating.
    connect(sigc::mem_fun(*this, &UCoreProfiler::slotThreadSwitch));
  monitor->onThreadExiting.
    connect(sigc::mem_fun(*this, &UCoreProfiler::slotThreadSwitch));

}
