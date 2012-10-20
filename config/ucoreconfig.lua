-- config.lua

s2e = {
   kleeArgs = {
      -- Whatever options you like
   }
}

plugins = {
   "BaseInstructions",
   "UCoreMonitor",
   "UCoreMemoryManagement"
}

pluginsConfig = {
}


pluginsConfig.BaseInstructions = {
}

pluginsConfig.UCoreMonitor = {
   kernelBase  = 0xc0100000,
   kernelEnd = 0xc01000ff,
   MonitorFunction = true,
   system_map_file = "/home/fwl/ucore/ucore_tool_s2e/build/lab8/obj/kernel.sym"
}

pluginsConfig.UCoreMemoryManagement = {
   print_pgdir_pc = 0xc010008f
}
