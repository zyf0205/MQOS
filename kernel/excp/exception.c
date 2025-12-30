#include <xtos.h>
#include <exception.h>
#include <console.h>
#include <memory.h>
#include <stdbool.h>

bool cursor_flash = 0; /*光标*/
bool plane_move = 0;   /*飞机移动*/

bool caps_locked = false; /*shift按键状态*/

/*键盘映射表，数组的下标对应扫描码，数组内容对应ascll码*/
char keys_map[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '`', 0,
    0, 0, 0, 0, 0, 'q', '1', 0, 0, 0, 'z', 's', 'a', 'w', '2', 0,
    0, 'c', 'x', 'd', 'e', '4', '3', 0, 0, 32, 'v', 'f', 't', 'r', '5', 0,
    0, 'n', 'b', 'h', 'g', 'y', '6', 0, 0, 0, 'm', 'j', 'u', '7', '8', 0,
    0, ',', 'k', 'i', 'o', '0', '9', 0, 0, '.', '/', 'l', ';', 'p', '-', 0,
    0, 0, '\'', 0, '[', '=', 0, 0, 0, 0, 13, ']', 0, '\\', 0, 0,
    0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, '`', 0};

void do_keyboard(unsigned char c)
{
    // static unsigned long stack[10];
    // static int index = 0; /*stack索引*/
    // c = keys_map[c];
    // if (c == 'a' && index < 10)
    // {
    //     stack[index] = get_page();
    //     print_debug("get a page: ", stack[index]);
    //     index++;
    // }
    // else if (c == 's' && index > 0)
    // {
    //     index--;
    //     free_page(stack[index]);
    //     print_debug("free a page: ", stack[index]);
    // }
    /*大写锁定处理*/
    if (c == 0x58)
    {
        caps_locked = !caps_locked;
    }

    /*根据映射表得到ascll码*/
    c = keys_map[c];

    /*小写转大写处理*/
    if (caps_locked && c >= 'a' && c <= 'z')
    {
        c -= 32;
    }
    if (c != 0)
    {
        char buf[2] = {c, '\0'};
        printk(buf);
    }
}

/*定时器中断处理函数*/
void timer_interrupt()
{
    static int systick = 0;
    systick++;
    if (systick % 2 == 0)
    {
        cursor_flash = !cursor_flash;
    }
    if (systick % 4 == 0)
    {
        plane_move = !plane_move;
    }
}

/*键盘中断处理函数*/
void key_interrupt()
{
    /*按下键盘和释放键盘都会产生中断，但是释放中断不处理，防止重复处理一个按键*/
    /*按下：读取一次得到该键的扫描码*/
    /*释放：读取两次，第一次得到0xf0,第二次得到扫描码*/
    unsigned char c;
    static bool is_break = false;    /*是否收到f0*/
    static bool is_extended = false; /*是否收到E0扩展码*/
    /*键盘按下处理，得到扫描码*/
    c = *(volatile unsigned char *)L7A_I8042_DATA;

    if (c == 0xE0)
    {
        is_extended = true;
        return;
    }
    if (c == 0xF0)
    {
        is_break = true;
        return;
    }
    /*释放状态，上一次接收到了f0*/
    if (is_break)
    {
        is_break = false; /*重置状态*/
        is_extended = false;
        /*有释放案件处理可以在这里实现*/
        return;
    }

    if (is_extended)
    {
        is_extended = false;
        switch (c)
        {
        case 0x75:
            console_move_cursor(0, -1);
            break; // Up
        case 0x72:
            console_move_cursor(0, 1);
            break; // Down
        case 0x6B:
            console_move_cursor(-1, 0);
            break; // Left
        case 0x74:
            console_move_cursor(1, 0);
            break; // Right
        }
        return;
    }
    do_keyboard(c);
}

/*通用异常处理函数,在汇编中被调用*/
void do_exception()
{
    unsigned int estat; /* 异常状态寄存器的值 */
    unsigned long irq;  /*存储是否发生键盘中断*/

    estat = read_csr_32(CSR_ESTAT); /*获取中断状态，判断13个位哪个位为1,即产生中断*/

    if (estat & CSR_ESTAT_IS_TI) /*定时器中断*/
    {
        /*清除中断标志*/
        write_csr_32(CSR_TICLR_CLR, CSR_TICLR);
        /*执行定时器中断处理函数*/
        timer_interrupt();
    }

    if (estat & CSR_ESTAT_IS_HWI0)
    {
        irq = read_iocsr(IOCSR_EXT_IOI_SR);
        if (irq & (1UL << KEYBOARD_IRQ_HT)) /*为1发生键盘中断*/
        {
            write_iocsr(1UL << KEYBOARD_IRQ_HT, IOCSR_EXT_IOI_SR); /*W1C机制，写1清除键盘中断*/
            key_interrupt();                                       /*执行键盘中断处理*/
        }
    }
}

/*启用全局中断函数*/
void int_on()
{
    unsigned int crmd; /* 当前模式寄存器的值 */
    crmd = read_csr_32(CSR_CRMD);
    /*crmd | CSR_CRMD_IE：将第 2 位设为 1（或运算）*/
    write_csr_32(crmd | CSR_CRMD_IE, CSR_CRMD);
}

/*异常处理初始化函数，在系统启动时调用，配置异常处理系统*/
void excp_init()
{
    unsigned int val;

    val = read_cpucfg(CC_FREQ); /*定时周期250ms*/
    write_csr_64((unsigned long)val | CSR_TCFG_EN | CSR_TCFG_PER, CSR_TCFG);
    write_csr_64((unsigned long)exception_handler, CSR_EENTRY);                  /*中断入口*/
    *(volatile unsigned long *)(L7A_INT_MASK) = ~(0x1UL << KEYBOARD_IRQ);        /*打开键盘中断，设置为0，不屏蔽*/
    *(volatile unsigned char *)(L7A_HTMSI_VEC + KEYBOARD_IRQ) = KEYBOARD_IRQ_HT; /*将中断3(键盘）映射到硬件线索0，即写为0*/
    write_iocsr(0x1UL << KEYBOARD_IRQ_HT, IOCSR_EXT_IOI_EN);                     /*启动外部中断,刚刚映射到了位0,所以将位0设置为1,表示使能中断*/
    write_csr_32(CSR_ECFG_LIE_TI | CSR_ECFG_LIE_HWI0, CSR_ECFG);                 /*启用定时器和硬件中断*/
}

/*
 * 设置定时器周期
 * period_ms: 定时周期，单位毫秒(ms)
 * 例如: set_timer_period(250) 表示 250ms
 *       set_timer_period(1000) 表示 1s
 *       set_timer_period(100) 表示 0.1s
 */
void set_timer_period(unsigned int period_ms)
{
    unsigned long val;
    unsigned int cpu_freq;

    /* 读取 CPU 频率 (单位: Hz) */
    cpu_freq = read_cpucfg(CC_FREQ);

    /* 计算定时器计数值
     * cpu_freq 是 1 秒的计数值
     * period_ms/1000.0 是要设置的秒数
     * 所以: val = cpu_freq * (period_ms / 1000)
     */
    val = (unsigned long)cpu_freq * period_ms / 1000;

    /* 写入定时器配置寄存器，启用定时器和周期模式 */
    write_csr_64(val | CSR_TCFG_EN | CSR_TCFG_PER, CSR_TCFG);
}