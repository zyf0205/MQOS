#include <xtos.h>
#include "console.h"
#include <stdbool.h>
#include <stddef.h>

struct RGB color_map[] = {
    {255, 0, 0, 0},     // 0: RED
    {0, 255, 0, 0},     // 1: GREEN
    {255, 255, 0, 0},   // 2: YELLOW
    {0, 255, 255, 0},   // 3: CYAN
    {255, 0, 255, 0},   // 4: MAGENTA
    {255, 255, 255, 0}, // 5: WHITE
    {0, 0, 0, 0}        // 6: BLACK
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

    /*拿到字符*/
    font_byte = &fonts[(ascii - 32) * CHAR_HEIGHT];
    /*得到显存位置*/
    pos = (char *)(VRAM_BASE + (yy * CHAR_HEIGHT * NR_PIX_X + xx * CHAR_WIDTH) * NR_BYTE_PIX);

    /*行循环*/
    for (row = 0; row < CHAR_HEIGHT; row++, font_byte++)
    {
        /*列循环*/
        for (col = 0; col < CHAR_WIDTH; col++)
        {
            /*通过掩码判断当前像素点是否要绘制*/
            if (*font_byte & (1 << (7 - col)))
            {
                /*设置为相对应的rgba分量*/
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
        /*显存地址跳到下一行的左边起始位置*/
        pos += (NR_PIX_X - CHAR_WIDTH) * NR_BYTE_PIX;
    }
}

/* 绘制状态栏 (第 0 行) */
void update_status_bar()
{
    int y = 0; // 状态栏固定在第 0 行
    int x;

    // 绘制背景条 (使用深灰色或蓝色背景，白色文字)
    for (x = 0; x < NR_CHAR_X; x++)
    {
        write_char_with_color(' ', x, y, BLACK); // 清空背景
    }

    // 显示左侧信息：系统名称
    const char *sys_name = "MYMQOS Kernel v1.0";
    for (int i = 0; sys_name[i]; i++)
    {
        write_char_with_color(sys_name[i], 1 + i, y, YELLOW);
    }

    // 显示中间信息：当前聚焦区域提示
    const char *focus_msg;
    enum COLOR focus_color;

    if (current_focus == &top_region)
    {
        focus_msg = "FOCUS: TOP REGION";
        focus_color = top_region.color;
    }
    else
    {
        focus_msg = "FOCUS: BOTTOM REGION";
        focus_color = bottom_region.color;
    }

    /*得到中间坐标*/
    int msg_len = 0;
    while (focus_msg[msg_len])
        msg_len++;
    int center_x = (NR_CHAR_X - msg_len) / 2;

    for (int i = 0; i < msg_len; i++)
    {
        write_char_with_color(focus_msg[i], center_x + i, y, focus_color);
    }

    // 显示右侧信息：操作提示
    const char *hint = "[F3] Switch Colors  [TAB] Switch Focus  [F1/F2] Scroll";
    int hint_len = 0;
    while (hint[hint_len])
        hint_len++;

    int right_x = NR_CHAR_X - hint_len - 1;
    for (int i = 0; i < hint_len; i++)
    {
        write_char_with_color(hint[i], right_x + i, y, WHITE);
    }
}

/* 刷新屏幕 只刷新指定区域 */
/* 如果 region 为 NULL，则刷新全屏 */
void flush_screen(ConsoleRegion *region)
{
    int start_y, end_y;

    if (region == NULL)
    {
        /* 刷新全屏 */
        update_status_bar();
        start_y = 1;
        end_y = NR_CHAR_Y;
    }
    else
    {
        /* 只刷新指定区域 */
        start_y = region->start_line;
        end_y = region->end_line + 1;
    }

    /* y：当前绘制行号 */
    for (int y = start_y; y < end_y; y++)
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
        /* (y - reg->start_line)  相对上下半屏幕的相对行号 */
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

/* 从指定位置开始删除一个字符，后续内容前移 */
void delete_char_at(ConsoleRegion *reg, int virt_y, int x)
{
    // 行内左移：把 x 之后的所有字符往前挪一位
    for (int i = x; i < NR_CHAR_X - 1; i++)
    {
        video_buffer[virt_y][i] = video_buffer[virt_y][i + 1];
    }
    // 最后一个字符位置先置空，等待后面填补
    video_buffer[virt_y][NR_CHAR_X - 1] = ' ';

    // 尝试从下一行“借”一个字符填补本行末尾
    int next_virt_y = virt_y + 1;
    /*下一行在虚拟缓冲中*/
    if (next_virt_y < reg->mem_start + reg->mem_height)
    {
        char next_first = video_buffer[next_virt_y][0];

        if (next_first != ' ')
        {
            /*找到上一行最后一个不为空格的地方*/
            int tail_pos = NR_CHAR_X - 1;
            while (tail_pos >= 0 && video_buffer[virt_y][tail_pos] == ' ')
            {
                tail_pos--;
            }

            /* 借来的字符应该放在 tail_pos + 1 的位置 */
            /* 如果整行都是满的，就放在最后 */
            if (tail_pos < NR_CHAR_X - 1)
            {
                video_buffer[virt_y][tail_pos + 1] = next_first;
            }
            else
            {
                video_buffer[virt_y][NR_CHAR_X - 1] = next_first;
            }

            // 递归：下一行的第一个字符被借走了，所以要删除它
            // 这会触发下一行去借下下行的字符，形成连锁反应
            delete_char_at(reg, next_virt_y, 0);
        }
    }
}

/* 向指定屏幕区域输出字符 */
void region_putc(ConsoleRegion *reg, char c)
{
    int virt_y = get_cursor_virt_y(reg);
    int screen_height = reg->end_line - reg->start_line + 1;

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
                // 清空最下面的一行
                for (int x = 0; x < NR_CHAR_X; x++)
                    video_buffer[end][x] = ' ';
            }
        }
    }
    else if (c == '\b' || c == 127) /*退格操作*/
    {
        if (reg->cur_x > 0)
        {
            // 情况1：行内删除
            reg->cur_x--;
            delete_char_at(reg, virt_y, reg->cur_x);
        }
        else
        {
            // 情况2：行首删除 (需要合并到上一行)
            bool can_move_up = false;
            if (reg->cur_y > reg->start_line)
            {
                reg->cur_y--;
                can_move_up = true;
            }
            else if (reg->view_offset > 0)
            {
                reg->view_offset--;
                can_move_up = true;
            }

            if (can_move_up)
            {
                /*获取当前光标虚拟行号*/
                int prev_virt_y = get_cursor_virt_y(reg);

                // 找到上一行内容的末尾
                int last_x = NR_CHAR_X - 1;
                while (last_x >= 0 && video_buffer[prev_virt_y][last_x] == ' ')
                    last_x--;

                // 光标移动到上一行末尾的下一个位置
                reg->cur_x = last_x + 1;

                /* 防止光标越界导致消失 (最大只能是 159) */
                if (reg->cur_x >= NR_CHAR_X)
                {
                    reg->cur_x = NR_CHAR_X - 1;
                }

                // 如果上一行没满，就开始从下一行(原来的当前行)拉取内容
                if (reg->cur_x < NR_CHAR_X)
                {
                    int next_virt_y = prev_virt_y + 1;

                    // 只要下一行还有内容，且上一行还没满，就一直搬运
                    while (reg->cur_x < NR_CHAR_X &&
                           next_virt_y < reg->mem_start + reg->mem_height &&
                           video_buffer[next_virt_y][0] != ' ')
                    {
                        // 取出下一行第一个字
                        char move_char = video_buffer[next_virt_y][0];

                        // 填入上一行当前光标位置
                        video_buffer[prev_virt_y][reg->cur_x] = move_char;
                        reg->cur_x++;

                        // 从下一行删除这个字
                        delete_char_at(reg, next_virt_y, 0);
                    }

                    // 搬运完成后，光标回到合并点
                    reg->cur_x = last_x + 1;

                    /* 再次检查边界 */
                    if (reg->cur_x >= NR_CHAR_X)
                        reg->cur_x = NR_CHAR_X - 1;
                }
            }
        }
    }
    else /*普通字符*/
    {
        video_buffer[virt_y][reg->cur_x] = c;
        reg->cur_x++;
        if (reg->cur_x >= NR_CHAR_X)
            region_putc(reg, '\n');
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
    top_region.start_line = 1;   /* 起始行号 */
    top_region.end_line = 24;    /* 结束行号 */
    top_region.mem_start = 0;    /* 缓冲区起始行 */
    top_region.mem_height = 100; /* 缓冲区结束行 */
    top_region.view_offset = 0;  /* 屏幕滚动量 */
    top_region.cur_x = 0;        /* 初始光标 */
    top_region.cur_y = 1;        /* 初始光标 */
    top_region.color = MAGENTA;  /* 字符颜色 */

    /* 下半屏 */
    bottom_region.start_line = 26;
    bottom_region.end_line = 49;
    bottom_region.mem_start = 100;
    bottom_region.mem_height = 100;
    bottom_region.view_offset = 0;
    bottom_region.cur_x = 0;
    bottom_region.cur_y = 26;
    bottom_region.color = MAGENTA;

    current_focus = &top_region; /* 当前聚焦窗口 */
    current_color = WHITE;       /* 其他部分的颜色 */

    flush_screen(NULL); /*全屏刷新*/
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
    flush_screen(current_focus);
}

void printk(char *buf)
{
    while (*buf)
    {
        region_putc(current_focus, *buf++);
    }
    flush_screen(current_focus);
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
}