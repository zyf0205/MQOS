#ifndef MEMORY_H
#define MEMORY_H

#define CSR_DMW0 0x180                       // CSR 直接映射窗口 0 寄存器地址
#define CSR_DMW0_PLV0 (1UL << 0)             // DMW0 特权级 0（内核态）使能位
#define MEMORY_SIZE 0x8000000                // 物理内存总大小：128 MiB (0x8000000 = 134,217,728 字节)
#define PAGE_SIZE 4096                       // 页大小：4 KiB (标准页框大小)
#define NR_PAGE (MEMORY_SIZE >> 12)          // 总页数：128 MiB / 4 KiB = 32768 页 (0x8000)
#define KERNEL_START_PAGE (0x200000UL >> 12) // 内核起始页号：0x200000 / 4096 = 512
#define KERNEL_END_PAGE (0x300000UL >> 12)   // 内核结束页号：0x300000 / 4096 = 768
                                             // 内核占用 256 页 (1 MiB 空间)
/*memory相关申明---------------------------------------------------------------------*/
void mem_init();
unsigned long get_page();
void free_page(unsigned long page);

#endif