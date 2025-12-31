#include <xtos.h>
#include <exception.h>
#include <console.h>
#include <memory.h>
#include <stdbool.h>

void main()
{
    excp_init();           /*中断初始化*/
    set_timer_period(250); /*设置中断周期250ms*/
    int_on();              /*开中断*/
    con_init();            /*初始化分屏*/

    current_focus = &top_region;
    printk("This is the TOP region (Cyan).\n");
    printk("Type something here...\n");

    current_focus = &bottom_region;
    printk("This is the BOTTOM region (Green).\n");
    printk("Use TAB to switch focus.\n");

    current_focus = &top_region;

    bool last_cursor_flash = 0;

    while (1)
    {
        // 简单的光标闪烁逻辑
        if (cursor_flash != last_cursor_flash)
        {
            flush_screen();
            last_cursor_flash = cursor_flash;
        }
    }
}