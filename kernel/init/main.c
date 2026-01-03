#include <xtos.h>
#include <exception.h>
#include <console.h>
#include <memory.h>
#include <stdbool.h>

void delay(int count)
{
    volatile int i;
    for (i = 0; i < count * 10000; i++)
        ;
}

/* 自动化测试脚本 */
void run_startup_tests()
{
    printk("Starting System Self-Test...\n");

    // 颜色测试
    printk("Testing Colors: ");
    set_color(RED);
    delay(100);
    printk("R ");
    set_color(GREEN);
    delay(100);
    printk("G ");
    set_color(YELLOW);
    delay(100);
    printk("Y ");
    set_color(CYAN);
    delay(100);
    printk("C ");
    set_color(WHITE);
    delay(100);
    printk("W\n");

    // 填充测试 (快速填满上半屏)
    printk("Filling screen to test scrolling...\n");
    for (int i = 0; i < 30; i++)
    {
        // 打印行号，方便观察滚屏
        char buf[10];
        int temp = i;
        int len = 0;
        if (temp == 0)
            buf[len++] = '0';
        while (temp > 0)
        {
            buf[len++] = (temp % 10) + '0';
            temp /= 10;
        }

        printk("Line ");
        while (len > 0)
            region_putc(current_focus, buf[--len]);
        printk(": This is a long line to test auto-wrapping functionality in the console.\n");

        delay(100);
    }

    printk(">> Test Done. System Ready. <<\n");
}

void main()
{
    excp_init();          /*中断初始化*/
    set_timer_period(25); /*设置中断周期250ms*/
    int_on();             /*开中断*/
    con_init();           /*初始化分屏*/

    /*测试上半屏*/
    current_focus = &top_region;
    for (int i = 0; i < 30; i++)
    {
        for (int j = 0; j < 160; j++)
        {
            printk("1");
        }
    }

    // run_startup_tests();

    // /*测试下半屏*/
    // current_focus = &bottom_region;
    // run_startup_tests();

    while (1)
    {
    }
}