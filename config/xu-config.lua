-- config.lua

s2e = {
   kleeArgs = {
      -- whatever options you like
   }
}

plugins = {
--   "BaseInstructions",
   "UCoreMonitor",
--   "UCoreMemoryManagement"
}

pluginsConfig = {
}

pluginsConfig.BaseInstructions = {
}

pluginsConfig.UCoreMonitor = {
   kernelBase = 0xc0000000,
   kernelEnd = 0xc01000ff,
   system_map_file = "/home/nuk/project/ucore_tool_s2e/test/lab4/obj/kernel.sym",
   MonitorFunction = true,
   MonitorThreads = true
}

--pluginsConfig.UCoreMemoryManagement = {
--   print_pgdir_pc = 0xc010008f
--}