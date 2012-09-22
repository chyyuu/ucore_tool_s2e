ucore_tool_s2e
==============
UCoreMM by fwl 2012.09.17
-------------------------
阅读了物理内存管理，准备从一级页表开始构建UCoreMM工具框架<br>
1. 建好页表时发出信号。<br>
2. 获得allocpage的信息。<br>
3. 获得freepage的信息。<br>
4. 对整个页目录表和页表内容扫描打印（方便随时调用）。<br>

**目前问题：**<br>
需要UCoreMonitor的监控函数功能，测试的时候发现有些函数监控不到：如pmm_init。<br>

上传文件UCoreMemoryManagement.h,UCoreMemoryManagement.cpp。