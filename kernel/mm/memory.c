#include <xtos.h>
#include <memory.h>
#include <console.h>
#include <exception.h>

char mem_map[NR_PAGE]; // 内存映射数组，为1表示对应页被占用，为0表示没有被占用

/*内存初始化函数*/
void mem_init()
{
    int i;
    /*初始化页状态*/
    for (i = 0; i < NR_PAGE; i++)
    {
        if (i >= KERNEL_START_PAGE && i < KERNEL_END_PAGE)
        {
            mem_map[i] = 1;/*占用*/
        }
        else
        {
            mem_map[i] = 0;/*未占用*/
        }
    }

    write_csr_64(DMW_MASK|CSR_DMW0_PLV0,CSR_DMW0);
}


/*申请空闲物理页*/
unsigned long get_page()
{
    unsigned long page;/*八字节*/
    unsigned long i;
    for(i=NR_PAGE-1;i>=0;i--)/*从最后一页开始便利*/
    {
        if(mem_map[i]!=0)/*被占用*/
        {
            continue;
        }
        mem_map[i]=1;/*没被占用->被占用*/
        page=(i<<12)|DMW_MASK;/*先得到物理页的开始物理地址，再转换为虚拟地址*/
                
        set_mem((char *)page,0,PAGE_SIZE);/*将物理页清空*/
        return page;/*返回请求到的页虚拟地址*/
    }
    /*执行到这里说明没有申请成功,打印错误信息*/
    panic("panic: out of memory!\n");
    return 0;
}


/*释放申请到的物理页*/
void free_page(unsigned long page)
{
    unsigned long i;
    i=(page&~DMW_MASK)>>12;/*得到页数，即数组索引*/
    if(!mem_map[i])/*该页是空闲页*/
    {
        panic("panic: try to free free page!\n");
    }
    /*不是空闲页*/
    mem_map[i]--;/*设置为空闲页，释放成功*/
}