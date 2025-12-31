#include <xtos.h>
#include <exception.h>
#include <console.h>
#include <memory.h>
#include <stdbool.h>

void main()
{
    excp_init();          /*中断初始化*/
    set_timer_period(25); /*设置中断周期250ms*/
    int_on();             /*开中断*/
    con_init();           /*初始化分屏*/

    current_focus = &top_region;
    printk("Type something here...\n");

    while (1)
    {
    }
}