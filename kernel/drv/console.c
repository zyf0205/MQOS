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

char digits_map[] = "0123456789abcdef";

// 全局变量定义：使用更大的虚拟缓冲区
char video_buffer[VIRT_BUFFER_HEIGHT][NR_CHAR_X]; 
ConsoleRegion top_region;
ConsoleRegion bottom_region;
ConsoleRegion *current_focus;
enum COLOR current_color;

/* 底层写字符函数 (保持不变) */
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
                *pos++ = col_rgb.r; *pos++ = col_rgb.g; *pos++ = col_rgb.b; *pos++ = col_rgb.a;
            }
            else
            {
                *pos++ = 0; *pos++ = 0; *pos++ = 0; *pos++ = 0;
            }
        }
        pos += (NR_PIX_X - CHAR_WIDTH) * NR_BYTE_PIX;
    }
}

void write_char(char ascii, int xx, int yy) {
    write_char_with_color(ascii, xx, yy, current_color);
}

void erase_char(int xx, int yy) {
    write_char_with_color(' ', xx, yy, BLACK);
}

/* 核心功能：刷新屏幕 (重构) */
void flush_screen() {
    for(int y=0; y<NR_CHAR_Y; y++) {
        // 1. 确定当前物理行属于哪个区域
        ConsoleRegion *reg;
        if (y <= top_region.end_line) reg = &top_region;
        else if (y >= bottom_region.start_line) reg = &bottom_region;
        else {
            // 分界线
            for(int x=0; x<NR_CHAR_X; x++) write_char_with_color('-', x, y, WHITE);
            continue;
        }

        // 2. 计算对应的虚拟缓冲区行号
        int virt_y = reg->mem_start + reg->view_offset + (y - reg->start_line);

        // 3. 绘制
        for(int x=0; x<NR_CHAR_X; x++) {
            char c = video_buffer[virt_y][x];
            enum COLOR color = reg->color;
            
            // 检查是否是光标位置
            if (current_focus == reg && x == reg->cur_x && y == reg->cur_y) {
                // 光标位置：绘制反色块 (背景色为原前景色，前景色为黑色)
                // 这里我们需要修改 write_char_with_color 或者临时实现一个反色绘制逻辑
                // 简单做法：使用一个特殊的颜色组合，或者修改 write_char_with_color 支持背景色
                
                // 方案 A: 如果你的 write_char_with_color 只支持单色字+黑背景
                // 我们可以临时画一个实心块作为背景，然后再画黑色的字
                // 但由于 write_char_with_color 可能会清空背景，这比较麻烦。
                
                // 方案 B (推荐): 修改 write_char_with_color 的调用方式
                // 既然我们不能轻易改底层，我们可以用一种显眼的颜色来表示光标，比如红色
                // 或者，如果字符是空格，画一个下划线；如果不是空格，画字符本身但换个颜色
                
                if (c == ' ') {
                    write_char_with_color('_', x, y, WHITE); // 空格处显示下划线
                } else {
                    // 有字的地方，用反色或高亮色显示字符
                    // 比如：原本是青色，光标在上面时显示为白色或黄色
                    write_char_with_color(c, x, y, WHITE); 
                }
            } else {
                // 非光标位置：正常绘制
                write_char_with_color(c, x, y, color);
            }
        }
    }
}

/* 辅助：获取当前光标对应的虚拟缓冲区行号 */
int get_cursor_virt_y(ConsoleRegion *reg) {
    return reg->mem_start + reg->view_offset + (reg->cur_y - reg->start_line);
}

/* 向指定区域输出字符 (重构：支持非破坏性滚动) */
void region_putc(ConsoleRegion *reg, char c) {
    int virt_y = get_cursor_virt_y(reg);
    int screen_height = reg->end_line - reg->start_line + 1;

    if (c == '\n' || c == '\r') {
        reg->cur_x = 0;
        reg->cur_y++; // 物理光标下移
        
        // 如果物理光标超出了区域底部
        if (reg->cur_y > reg->end_line) {
            reg->cur_y = reg->end_line; // 保持在底部
            
            // 核心：增加视口偏移 (向下滚动)
            // 只要缓冲区还没满，就只是移动视口，不覆盖数据
            if (reg->view_offset + screen_height < reg->mem_height) {
                reg->view_offset++;
            } else {
                // 缓冲区满了，必须丢弃最上面的一行 (整体上移)
                // 这是一个内存移动操作
                int start = reg->mem_start;
                int end = reg->mem_start + reg->mem_height - 1;
                for (int i = start; i < end; i++) {
                    for (int x = 0; x < NR_CHAR_X; x++) {
                        video_buffer[i][x] = video_buffer[i+1][x];
                    }
                }
                // 清空新的一行
                for (int x = 0; x < NR_CHAR_X; x++) video_buffer[end][x] = ' ';
            }
        }
    } 
    else if (c == '\b' || c == 127) { 
        if (reg->cur_x > 0) {
            reg->cur_x--;
            video_buffer[virt_y][reg->cur_x] = ' ';
        } else {
            // 需要回退到上一行
            bool can_move_up = false;
            
            if (reg->cur_y > reg->start_line) {
                // 物理上还能往上走
                reg->cur_y--;
                can_move_up = true;
            } else if (reg->view_offset > 0) {
                // 物理到顶了，但视口可以往上滚 (恢复历史内容！)
                reg->view_offset--;
                // cur_y 保持在 start_line 不变
                can_move_up = true;
            }

            if (can_move_up) {
                // 重新计算新的虚拟坐标
                virt_y = get_cursor_virt_y(reg);
                
                // 寻找上一行最后一个字符
                int x = NR_CHAR_X - 1;
                while (x >= 0) {
                    if (video_buffer[virt_y][x] != ' ') break;
                    x--;
                }
                reg->cur_x = x + 1;
                if (reg->cur_x >= NR_CHAR_X) reg->cur_x = NR_CHAR_X - 1;
            }
        }
    } 
    else {
        // 普通字符
        video_buffer[virt_y][reg->cur_x] = c;
        reg->cur_x++;
        
        if (reg->cur_x >= NR_CHAR_X) {
            // 自动换行逻辑，递归调用自身处理换行
            region_putc(reg, '\n');
        }
    }
}

/* 初始化分屏系统 */
void con_init() {
    // 1. 清空整个大缓冲区
    for(int y=0; y<VIRT_BUFFER_HEIGHT; y++)
        for(int x=0; x<NR_CHAR_X; x++)
            video_buffer[y][x] = ' ';

    // 2. 配置上半屏
    top_region.start_line = 0;
    top_region.end_line = 24;
    top_region.mem_start = 0;        // 使用缓冲区前100行
    top_region.mem_height = 100;
    top_region.view_offset = 0;
    top_region.cur_x = 0;
    top_region.cur_y = 0;
    top_region.color = CYAN; 

    // 3. 配置下半屏
    bottom_region.start_line = 26;
    bottom_region.end_line = 49;
    bottom_region.mem_start = 100;   // 使用缓冲区后100行
    bottom_region.mem_height = 100;
    bottom_region.view_offset = 0;
    bottom_region.cur_x = 0;
    bottom_region.cur_y = 26;
    bottom_region.color = GREEN; 

    current_focus = &top_region;
    current_color = WHITE;
    
    flush_screen();
}

/* 新增：手动滚动视口函数
 * direction: -1 向上滚 (看历史), 1 向下滚 (看新内容)
 */
void console_scroll(int direction) {
    if (!current_focus) return;
    
    ConsoleRegion *reg = current_focus;
    int screen_height = reg->end_line - reg->start_line + 1;

    if (direction < 0) { 
        // 向上滚动 (查看历史)
        if (reg->view_offset > 0) {
            reg->view_offset--;
        }
    } else { 
        // 向下滚动
        // 限制不能滚出缓冲区边界
        // 注意：这里可以根据需求限制，比如不能滚到超过当前光标太远的地方
        // 目前简单限制为不超过缓冲区总大小
        if (reg->view_offset + screen_height < reg->mem_height) {
            reg->view_offset++;
        }
    }
    flush_screen();
}

void printk(char *buf) {
    while (*buf) {
        region_putc(current_focus, *buf++);
    }
    flush_screen();
}

void panic(char *s) {
    current_focus->color = RED;
    printk(s);
    while (1);
}

void print_debug(char *str, unsigned long val) {
    printk(str);
    char buffer[20];
    buffer[0] = '0'; buffer[1] = 'x';
    int i, j;
    for (j = 0, i = 17; j < 16; j++, i--) {
        buffer[i] = (digits_map[val & 0xfUL]);
        val >>= 4;
    }
    buffer[18] = '\n'; buffer[19] = '\0';
    printk(buffer);
}

void set_color(enum COLOR color) {
    if (current_focus) current_focus->color = color;
    current_color = color;
}

void init_plane() {}
void move_plane(int dx, int dy) {}
void print_duck() {}