UCoreMM
============================
将s2e/qemu/s2e/Plugins/UCoreInterceptor/UCoreMemoryManagement.h <br>
和s2e/qemu/s2e/Plugins/UCoreInterceptor/UCoreMemoryManagement.cpp <br>
添加到相应位置，并更新s2e/qemu/s2e/Plugins/UCoreInterceptor/UCoreMonitor.h <br>
和s2e/qemu/s2e/Plugins/UCoreInterceptor/UCoreMonitor.cpp <br>

config文件放在config文件夹下，请修改kernel.sym相应路径。 <br>

**问题** <br>
我在本机make的时候，只有将s2e/qemu/s2e/Plugins/UCoreInterceptor/下的文件 <br>
放在s2e/qemu/s2e/Plugins/下，并修改s2e/qemu/Makefile.target，才可以Make <br>
通过。如果make无法通过，可以尝试这种做法。 <br>

目前进展：
----------------------------
1.可以输出alloc_pages得到的Page信息。 <br>
2.在某一地址输出整个页目录表和页表内容print_pgdir。<br>