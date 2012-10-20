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

UCoreMM “complete print_pgdir in UCoreMM”
-------------------------
1.信号发送完成 <br>
2.allocpage基本完成 <br>
3.freepage基本完成 <br>
4.print_pgdir完成 <br>

**目前问题：** <br>
调试UCoreMonitor时发现，kern_init()函数在0xc010002c处，但是只能检测到kern_init()内部 pic_init()函数之后的函数（包括pic_init）
由于没有检测到pmm_init()函数，所以2 3目前没有调试。

上传文件UCoreMemoryManagement.h,UCoreMemoryManagement.cpp。
另修改了本地的UCoreMonitor，没有一起上传，需要通知赵旭在他的插件中添加相应的代码。

UCoreMM "add UCoreMM:print alloc_pages info and print_pgdir"
-------------------------
1.s2e + UCoreMonitor&UCoreMM push done <br>
2.解决之前的问题 <br>
3.可以输出alloc_pages得到的Page信息（为了测试，只输出了第一个alloc_pages的信息） <br>

**目前问题：**  <br>
在本机make的时候，只有将s2e/qemu/s2e/Plugins/UCoreInterceptor/下的文件 <br>
放在s2e/qemu/s2e/Plugins/下，并修改s2e/qemu/Makefile.target，才可以Make <br>
通过。否则会报错。

UCoreMM "add UCoreMM:free_pages info and modify"
-------------------------
1.解决之前的问题 <br>
2.可以输出free_pages得到的Page及size信息，添加alloc_pages的size信息，输出结果如下： <br>
----------alloc_pages info BEGIN---------- <br>
 #####alloc page size:1 <br>
ref:0 <br>
flags:1 <br>
property:32129 <br>
zone_num:0 <br>
pra_vaddr:0 <br>
----------alloc_pages info END---------- <br>
----------free_pages info BEGIN---------- <br>
 #####free page size:1 <br>
ref:0 <br>
flags:1 <br>
property:32129 <br>
zone_num:0 <br>
pra_vaddr:0 <br>
----------free_pages info END---------- <br>

UCoreMM "use cr0 to detect if enable_paging"
-------------------------
使用cr0来判断是否已经启用分页机制。

UCoreMM "print kmalloc alloc info(slab)"
-------------------------
基于slab算法获取kmalloc的信息，测试输出结果如下：  
----------kmalloc_pages info BEGIN----------  
 #####kmalloc page size:16  
kmem_cache_t objsize:32  
slab_t s_mem:c035d1e0  
slab_t free:0  
 #####objAddr:c035d1e0  
----------kmalloc_pages info END----------  
其中各项分别为ucore对应struct的各项

UCoreMM "print kfree info(slab) and write Tutorial for ucore_tool_s2e in README.md"
--------------------------
基于slab算法获取kfree的信息，测试输出结果如下：  
----------kmalloc info BEGIN----------  
 #####kmalloc page size:16  
kmem_cache_t objsize:32  
slab_t mem:c035d1e0  
slab_t free:0  
 #####objAddr:c035d1e0  
----------kmalloc info END----------  
----------kfree info BEGIN----------  
 #####kfree objp addr:c035d1e0  
kmem_cache_t objsize:32  
slab_t mem:c035d1e0  
slab_t free:2  
----------kfree info END----------  
同时输出kmalloc信息，方便观察kfree的正确性。

在根目录的README.md中写了Tutorial for ucore_tool_s2e