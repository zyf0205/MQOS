#include <xtos.h>
#include "console.h"
#include <stdbool.h>

struct RGB color_map[] = {
    {0, 0, 0, 0},       // BLACK
    {255, 255, 255, 0}, // WHITE
    {255, 0, 0, 0},     // RED
    {0, 255, 0, 0},     // GREEN
    {0, 0, 255, 0},     // BLUE
    {255, 255, 0, 0},   // YELLOW
    {0, 255, 255, 0},   // CYAN
    {255, 0, 255, 0}    // MAGENTA
};

char video_buffer[VIRT_BUFFER_HEIGHT][NR_CHAR_X]; /* 缓冲区200x160 */
ConsoleRegion top_region;                         /* 上半屏幕 */
ConsoleRegion bottom_region;                      /* 下半屏幕 */
ConsoleRegion *current_focus;                     /* 当前聚焦 */
enum COLOR current_color;                         /* 当前颜色 */

/* 底层写字符函数 */
void write_char_with_color(char ascii, int xx, int yy, enum COLOR color)
{
    char *font_byte;
    int row, col;
    char *pos;
    struct RGB col_rgb = color_map[color];

    font_byte = &fonts[(ascii - 32) * CHAR_HEIGHT];
    pos = (char *)(VRAM_BASE + (yy * CHAR_HEIGHT * NR_PIX_X + xx * CHAR_WIDTH) * NR_BYTE_PIX);

    for (row = 0; row < CHAR_HEIGHT; row++, font_byte++)
    {
        for (col = 0; col < CHAR_WIDTH; col++)
        {
            if (*font_byte & (1 << (7 - col)))
            {
                *pos++ = col_rgb.r;
                *pos++ = col_rgb.g;
                *pos++ = col_rgb.b;
                *pos++ = col_rgb.a;
            }
            else
            {
                *pos++ = 0;
                *pos++ = 0;
                *pos++ = 0;
                *pos++ = 0;
            }
        }
        pos += (NR_PIX_X - CHAR_WIDTH) * NR_BYTE_PIX;
    }
}

/* 刷新屏幕 */
void flush_screen()
{
    /* y：当前绘制行号 */
    for (int y = 0; y < NR_CHAR_Y; y++)
    {
        /* 确定当前物理行属于哪个窗口 */
        ConsoleRegion *reg;
        if (y <= top_region.end_line)
            reg = &top_region;
        else if (y >= bottom_region.start_line)
            reg = &bottom_region;
        else /* 绘制分界线 */
        {
            for (int x = 0; x < NR_CHAR_X; x++)
                write_char_with_color('-', x, y, WHITE);
            continue;
        }

        /* 计算对应的虚拟缓冲区行号 */
        /* (y - reg->start_line)  相对行号 */
        /* reg->view_offset       往下滚动行数 */
        /* reg->mem_start         当前区域的缓冲区基地址 */
        int virt_y = reg->mem_start + reg->view_offset + (y - reg->start_line);

        /* 绘制当前行 */
        for (int x = 0; x < NR_CHAR_X; x++)
        {
            char c = video_buffer[virt_y][x]; /* 取出缓冲区存放的字符 */
            enum COLOR color = reg->color;

            /* 判断是否到光标位置，特殊处理 */
            if (current_focus == reg && x == reg->cur_x && y == reg->cur_y)
            {
                if (c == ' ') /* 如果是空格就绘制白色下划线 */
                {
                    write_char_with_color('_', x, y, WHITE);
                }
                else /* 如果有可视字符就用白色显示 */
                {
                    write_char_with_color(c, x, y, WHITE);
                }
            }
            else /* 非光标位置，正常绘制 */
            {
                write_char_with_color(c, x, y, color);
            }
        }
    }
}

/* 获取当前光标对应的虚拟缓冲区行号 */
int get_cursor_virt_y(ConsoleRegion *reg)
{
    return reg->mem_start + reg->view_offset + (reg->cur_y - reg->start_line);
}

/* 向指定屏幕区域输出字符 */
void region_putc(ConsoleRegion *reg, char c)
{
    int virt_y = get_cursor_virt_y(reg);                     /*获取光标虚拟缓冲区行号*/
    int screen_height = reg->end_line - reg->start_line + 1; /*当前半屏高度*/

    if (c == '\n' || c == '\r') /*换行回车*/
    {
        /*物理光标下移*/
        reg->cur_x = 0;
        reg->cur_y++;

        /*如果物理光标超出了区域底部*/
        if (reg->cur_y > reg->end_line)
        {
            /*保持在底部*/
            reg->cur_y = reg->end_line;

            /*增加视口偏移 (向下滚动)*/
            if (reg->view_offset + screen_height < reg->mem_height)
            {
                reg->view_offset++;
            }
            else /*缓冲区满了，必须丢弃最上面的一行*/
            {
                /*整体内存移动操作*/
                int start = reg->mem_start;
                int end = reg->mem_start + reg->mem_height - 1;
                for (int i = start; i < end; i++)
                {
                    for (int x = 0; x < NR_CHAR_X; x++)
                    {
                        video_buffer[i][x] = video_buffer[i + 1][x];
                    }
                }
                // 清空新的一行
                for (int x = 0; x < NR_CHAR_X; x++)
                    video_buffer[end][x] = ' ';
            }
        }
    }
    else if (c == '\b' || c == 127) /*退格操作*/
    {
        if (reg->cur_x > 0)
        {
            reg->cur_x--;
            video_buffer[virt_y][reg->cur_x] = ' ';
        }
        else
        {
            // 需要回退到上一行
            bool can_move_up = false;

            if (reg->cur_y > reg->start_line)
            {
                // 物理上还能往上走
                reg->cur_y--;
                can_move_up = true;
            }
            else if (reg->view_offset > 0)
            {
                /*物理到顶了，但视口可以往上滚，恢复历史内容*/
                reg->view_offset--;
                can_move_up = true;
            }

            if (can_move_up)
            {
                /*重新计算新的虚拟坐标*/
                virt_y = get_cursor_virt_y(reg);

                /*寻找上一行最后一个字符*/
                int x = NR_CHAR_X - 1;
                while (x >= 0)
                {
                    if (video_buffer[virt_y][x] != ' ')
                        break;
                    x--;
                }
                reg->cur_x = x + 1;
                if (reg->cur_x >= NR_CHAR_X)
                    reg->cur_x = NR_CHAR_X - 1;
            }
        }
    }
    else /*普通字符*/
    {
        video_buffer[virt_y][reg->cur_x] = c;
        reg->cur_x++;

        if (reg->cur_x >= NR_CHAR_X)
        {
            /*递归调用自身处理换行*/
            region_putc(reg, '\n');
        }
    }
}

/* 初始化分屏 */
void con_init()
{
    /* 清空整个虚拟屏幕的缓冲区，设置的200行远大于50行 */
    for (int y = 0; y < VIRT_BUFFER_HEIGHT; y++)
        for (int x = 0; x < NR_CHAR_X; x++)
            video_buffer[y][x] = ' ';

    /* 上半屏 */
    top_region.start_line = 0;   /* 起始行号 */
    top_region.end_line = 24;    /* 结束行号 */
    top_region.mem_start = 0;    /* 缓冲区起始行 */
    top_region.mem_height = 100; /* 缓冲区结束行 */
    top_region.view_offset = 0;  /* 屏幕滚动量 */
    top_region.cur_x = 0;        /* 初始光标 */
    top_region.cur_y = 0;        /* 初始光标 */
    top_region.color = CYAN;     /* 字符颜色 */

    /* 下半屏 */
    bottom_region.start_line = 26;
    bottom_region.end_line = 49;
    bottom_region.mem_start = 100;
    bottom_region.mem_height = 100;
    bottom_region.view_offset = 0;
    bottom_region.cur_x = 0;
    bottom_region.cur_y = 26;
    bottom_region.color = GREEN;

    current_focus = &top_region; /* 当前聚焦窗口 */
    current_color = WHITE;       /* 其他部分的颜色 */

    flush_screen();
}

/* 手动滚动视口函数  direction: -1 向上滚 (看历史), 1 向下滚 (看新内容) */
void console_scroll(int direction)
{
    if (!current_focus)
        return;

    ConsoleRegion *reg = current_focus;
    int screen_height = reg->end_line - reg->start_line + 1;

    if (direction < 0) /* 向上滚动 */
    {
        if (reg->view_offset > 0)
        {
            reg->view_offset--; /* 改变视差 */
        }
    }
    else /* 向下滚动 */
    {
        if (reg->view_offset + screen_height < reg->mem_height) /* 不超出缓冲区 */
        {
            reg->view_offset++;
        }
    }
    flush_screen();
}

void printk(char *buf)
{
    while (*buf)
    {
        region_putc(current_focus, *buf++);
    }
    flush_screen();
}

void panic(char *s)
{
    current_focus->color = RED;
    printk(s);
    while (1)
        ;
}

void set_color(enum COLOR color)
{
    if (current_focus)
        current_focus->color = color;
    current_color = color;
}