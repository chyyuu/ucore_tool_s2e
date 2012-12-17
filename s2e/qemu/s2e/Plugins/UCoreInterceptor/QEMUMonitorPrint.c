#include "QEMUMonitorInterface.h"

void print_threads(void){
  ucore_monitor_print_threads();
}

void print_panic_info(void){
  ucore_monitor_print_panic_info();
}
