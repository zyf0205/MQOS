#include <xtos.h>
#include <exception.h>
#include <console.h>
#include <memory.h>
#include <stdbool.h>

/*全局变量定义*/
bool cursor_flash = 0;
bool caps_locked = false;

/* 键盘映射表 */
char keys_map[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '`', 0,
    0, 0, 0, 0, 0, 'q', '1', 0, 0, 0, 'z', 's', 'a', 'w', '2', 0,
    0, 'c', 'x', 'd', 'e', '4', '3', 0, 0, 32, 'v', 'f', 't', 'r', '5', 0,
    0, 'n', 'b', 'h', 'g', 'y', '6', 0, 0, 0, 'm', 'j', 'u', '7', '8', 0,
    0, ',', 'k', 'i', 'o', '0', '9', 0, 0, '.', '/', 'l', ';', 'p', '-', 0,
    0, 0, '\'', 0, '[', '=', 0, 0, 0, 0, 13, ']', 0, '\\', 0, 0,
    0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, '`', 0};

/* ==========================================================================
 * 辅助函数 (Helpers)
 * ========================================================================== */

/* 获取指定虚拟行的长度 (最后一个非空字符的下标 + 1) */
int get_line_length(int virt_y)
{
    int x = NR_CHAR_X - 1;
    while (x >= 0)
    {
        if (video_buffer[virt_y][x] != ' ')
        {
            return x + 1;
        }
        x--;
    }
    return 0;
}

/* 获取当前光标对应的虚拟行号 */
int get_current_virt_y()
{
    return current_focus->mem_start + current_focus->view_offset + (current_focus->cur_y - current_focus->start_line);
}

/* 获取区域内最后一行的虚拟行号 */
int get_last_content_virt_y()
{
    int max_y = current_focus->mem_start; // 默认为区域起始
    int limit = current_focus->mem_start + current_focus->mem_height;

    // 从缓冲区末尾向前扫描，找到第一个非空行
    for (int y = limit - 1; y >= current_focus->mem_start; y--)
    {
        if (get_line_length(y) > 0)
        {
            max_y = y;
            break;
        }
    }
    return max_y;
}

/*输入处理逻辑*/
/* 处理编辑器输入逻辑 */
void handle_editor_input(int scan_code)
{
    int virt_y = get_current_virt_y();
    int line_len = get_line_length(virt_y);
    int screen_height = current_focus->end_line - current_focus->start_line + 1;

    // 处理方向键
    if (scan_code == 0x75)
    { // Up (向上移动)
        if (current_focus->cur_y > current_focus->start_line)
        {
            current_focus->cur_y--;
            /* 移动后检查光标是否超出上一行的长度，如果超出则对齐到行末 */
            int prev_virt_y = get_current_virt_y();
            int prev_len = get_line_length(prev_virt_y);
            if (current_focus->cur_x > prev_len)
                current_focus->cur_x = prev_len;
        }
    }
    else if (scan_code == 0x72)
    { // Down (向下移动)
        if (current_focus->cur_y < current_focus->end_line)
        {
            /* 只有当下一行有内容时才允许向下移动 */
            int next_virt_y = get_current_virt_y() + 1;
            if (get_line_length(next_virt_y) > 0)
            {
                current_focus->cur_y++;
                /* 同样检查并修正光标 X 坐标 */
                int next_len = get_line_length(next_virt_y);
                if (current_focus->cur_x > next_len)
                    current_focus->cur_x = next_len;
            }
        }
    }
    else if (scan_code == 0x6B)
    { // Left (向左移动)
        if (current_focus->cur_x > 0)
        {
            current_focus->cur_x--;
        }
        else
        {
            /* 如果在行首，尝试跳转到上一行的末尾 */
            if (current_focus->cur_y > current_focus->start_line)
            {
                current_focus->cur_y--;
                int prev_virt_y = get_current_virt_y();
                current_focus->cur_x = get_line_length(prev_virt_y);
            }
        }
    }
    else if (scan_code == 0x74)
    { // Right (向右移动)
        /* 限制光标不能超过当前行的长度 */
        if (current_focus->cur_x < line_len)
        {
            current_focus->cur_x++;
        }
        else
        {
            /* 如果在行末，尝试跳转到下一行的开头 (前提是下一行有内容) */
            if (current_focus->cur_y < current_focus->end_line)
            {
                int next_virt_y = get_current_virt_y() + 1;
                if (get_line_length(next_virt_y) > 0)
                {
                    current_focus->cur_y++;
                    current_focus->cur_x = 0;
                }
            }
        }
    }
    else if (scan_code == 0x0D)
    { // Tab (切换分屏焦点)
        if (current_focus == &top_region)
            current_focus = &bottom_region;
        else
            current_focus = &top_region;
    }
    else if (scan_code == 0x05)
    { // F1: 向上滚屏 (查看历史)
        // 只有当视口偏移大于0时才允许向上滚
        if (current_focus->view_offset > 0)
        {
            console_scroll(-1);
        }
        return;
    }
    else if (scan_code == 0x06)
    { // F2: 向下滚屏 (查看新内容)
        // 计算内容总高度 (相对于 mem_start)
        int last_content_y = get_last_content_virt_y();
        int content_height = last_content_y - current_focus->mem_start + 1;

        // 只有当内容高度超过屏幕高度，且还没滚到底时，才允许向下滚
        if (content_height > screen_height &&
            (current_focus->view_offset + screen_height < content_height))
        {
            console_scroll(1);
        }
        return;
    }
    else
    {
        // 普通字符处理
        char c = keys_map[scan_code];
        /* 处理大写锁定 */
        if (caps_locked && c >= 'a' && c <= 'z')
            c -= 32;

        if (c != 0)
        {
            region_putc(current_focus, c);
        }
    }

    flush_screen();
}

/*中断处理函数*/
void timer_interrupt()
{
    static int systick = 0;
    systick++;
    if (systick % 2 == 0)
        cursor_flash = !cursor_flash;
}

void key_interrupt()
{
    unsigned char c;
    static bool is_break = false;    /* 标记是否为断码 (按键释放) */
    static bool is_extended = false; /* 标记是否为扩展码 (如方向键) */

    /* 从键盘数据寄存器读取扫描码 */
    c = *(volatile unsigned char *)L7A_I8042_DATA;

    /* 0xE0 是扩展码的前缀 (Extended Key) */
    /* 例如方向键、Home/End 等功能键通常以 E0 开头 */
    if (c == 0xE0)
    {
        is_extended = true;
        return; /* 等待下一个字节 */
    }

    /* 0xF0 是断码的前缀 (Break Code) */
    /* 表示按键被释放。PS/2 键盘按下发送通码，释放发送 F0 + 通码 */
    if (c == 0xF0)
    {
        is_break = true;
        return; /* 等待下一个字节 */
    }

    /* 如果当前处于断码状态 (即上一个字节是 F0) */
    if (is_break)
    {
        /* 这是一个按键释放事件，我们忽略它，只处理按键按下 */
        is_break = false;
        is_extended = false; /* 重置扩展状态 */
        return;
    }

    /* 处理 CapsLock (大小写锁定键) */
    /* 0x58 是 CapsLock 的通码 */
    if (c == 0x58)
        caps_locked = !caps_locked;

    /* 如果是扩展键 (如方向键) */
    if (is_extended)
    {
        is_extended = false;
        // 直接传递扩展码对应的扫描码给处理函数
        handle_editor_input(c);
        return;
    }

    // 传递普通扫描码 (如字母、数字)
    handle_editor_input(c);
}

void do_exception()
{
    unsigned int estat; /* 异常状态寄存器的值 */
    unsigned long irq;  /* 存储是否发生键盘中断 */

    estat = read_csr_32(CSR_ESTAT); /* 获取中断状态 */

    if (estat & CSR_ESTAT_IS_TI) /* 定时器中断 */
    {
        /* 清除中断标志 */
        write_csr_32(CSR_TICLR_CLR, CSR_TICLR);
        /* 执行定时器中断处理函数 */
        timer_interrupt();
    }

    if (estat & CSR_ESTAT_IS_HWI0)
    {
        irq = read_iocsr(IOCSR_EXT_IOI_SR);
        if (irq & (1UL << KEYBOARD_IRQ_HT)) /* 为1发生键盘中断 */
        {
            write_iocsr(1UL << KEYBOARD_IRQ_HT, IOCSR_EXT_IOI_SR); /* W1C机制，写1清除键盘中断 */
            key_interrupt();                                       /* 执行键盘中断处理 */
        }
    }
}

/*初始化与配置*/
/* 启用全局中断函数 */
void int_on()
{
    unsigned int crmd; /* 当前模式寄存器的值 */
    crmd = read_csr_32(CSR_CRMD);
    /* crmd | CSR_CRMD_IE：将第 2 位设为 1（或运算） */
    write_csr_32(crmd | CSR_CRMD_IE, CSR_CRMD);
}

/* 异常处理初始化函数，在系统启动时调用，配置异常处理系统 */
void excp_init()
{
    unsigned int val;

    val = read_cpucfg(CC_FREQ); /* 定时周期250ms */
    write_csr_64((unsigned long)val | CSR_TCFG_EN | CSR_TCFG_PER, CSR_TCFG);
    write_csr_64((unsigned long)exception_handler, CSR_EENTRY);                  /* 中断入口 */
    *(volatile unsigned long *)(L7A_INT_MASK) = ~(0x1UL << KEYBOARD_IRQ);        /* 打开键盘中断，设置为0，不屏蔽 */
    *(volatile unsigned char *)(L7A_HTMSI_VEC + KEYBOARD_IRQ) = KEYBOARD_IRQ_HT; /* 将中断3(键盘）映射到硬件线索0，即写为0 */
    write_iocsr(0x1UL << KEYBOARD_IRQ_HT, IOCSR_EXT_IOI_EN);                     /* 启动外部中断 */
    write_csr_32(CSR_ECFG_LIE_TI | CSR_ECFG_LIE_HWI0, CSR_ECFG);                 /* 启用定时器和硬件中断 */
}

/*
 * 设置定时器周期
 * period_ms: 定时周期，单位毫秒(ms)
 */
void set_timer_period(unsigned int period_ms)
{
    unsigned long val;
    unsigned int cpu_freq;

    /* 读取 CPU 频率 (单位: Hz) */
    cpu_freq = read_cpucfg(CC_FREQ);

    /* 计算定时器计数值 */
    val = (unsigned long)cpu_freq * period_ms / 1000;

    /* 写入定时器配置寄存器，启用定时器和周期模式 */
    write_csr_64(val | CSR_TCFG_EN | CSR_TCFG_PER, CSR_TCFG);
}