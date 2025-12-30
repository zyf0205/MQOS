#include <xtos.h>
#include <exception.h>
#include <console.h>
#include <memory.h>
#include <stdbool.h>

void main()
{
    // mem_init();
    excp_init(); /*中断初始化*/
    set_timer_period(250);
    int_on(); /*开中断*/
    
    // 初始化分屏显示系统
    con_init();
    // init_plane(); // 暂时注释掉飞机，先测试分屏

    // 测试输出
    current_focus = &top_region;
    printk("Welcome to MYMQOS Split Screen Mode!\n");
    printk("This is the TOP region (Cyan).\n");
    printk("Type something here...\n");

    current_focus = &bottom_region;
    printk("This is the BOTTOM region (Green).\n");
    printk("Use TAB (if mapped) or modify code to switch focus.\n");
    
    // 切回上半屏作为默认输入
    current_focus = &top_region;

    bool last_cursor_flash = 0;
    
    while (1)
    {
        // 简单的光标闪烁逻辑
        if (cursor_flash != last_cursor_flash)
        {
            // 每次闪烁状态改变时刷新屏幕
            // 注意：全屏刷新开销较大，实际使用中最好只重绘光标位置
            // 这里为了简单演示直接刷新
            flush_screen(); 
            last_cursor_flash = cursor_flash;
        }
    }
}