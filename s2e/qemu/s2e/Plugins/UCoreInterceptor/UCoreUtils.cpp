
extern "C" {
#include "config.h"
#include "qemu-common.h"
}

#include "UCoreUtils.h"

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

namespace s2e{
  namespace plugins{

    S2E_DEFINE_PLUGIN(UCoreUtils, "Utils For UCore Plugins", "",);

    UCoreUtils::~UCoreUtils(){
    }

    void UCoreUtils::initialize(){
      first = true;
      bool ok;
      system_map_file = s2e()
        ->getConfig()->getString(getConfigKey() + ".system_map_file",
                                 "", &ok);
      if(!ok){
        s2e()->getWarningsStream() << "No kernel.sym file provided. System.map is needed for UCoreMonitor to work properly. Quit.\n";
        exit(-1);
      }
      parseSymbolMap();
    }

    void UCoreUtils::parseSymbolMap(){
      ifstream system_map_stream;
      system_map_stream.open(system_map_file.c_str());
      if(!system_map_stream){
        s2e()->getWarningsStream() << "Unable to open System.map file"
                                   << system_map_file << ".\n";
        exit(-1);
      }

      char line[255];
      uint64_t addr;
      string kernel_symbol;
      while(system_map_stream){
        system_map_stream.getline(line, 255);
        char temp[200];
        sscanf(line, "%lx %s", &addr, temp);
        Addr2Sym[addr] = temp;
        Sym2Addr[temp] = addr;
      }
      return;
    }
  }
}
