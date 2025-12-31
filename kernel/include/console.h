#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>

/* 屏幕物理参数 */
#define NR_PIX_X 1280
#define NR_PIX_Y 800
#define NR_BYTE_PIX 4
#define VRAM_BASE 0x40000000UL
/* 字符参数 */
#define CHAR_HEIGHT 16
#define CHAR_WIDTH 8
/* 屏幕字符容量 */
#define NR_CHAR_X (NR_PIX_X / CHAR_WIDTH)  /* 160 */
#define NR_CHAR_Y (NR_PIX_Y / CHAR_HEIGHT) /* 50 */
/* 虚拟缓冲区参数 */
#define VIRT_BUFFER_HEIGHT 200

/* 颜色枚举 */
enum COLOR
{
    RED = 0,
    GREEN = 1,
    YELLOW = 2,
    CYAN = 3,
    MAGENTA = 4,
    WHITE = 5,
    BLACK = 6
};
/* RGB 颜色结构 */
struct RGB
{
    unsigned char b, g, r, a;
};
/* 屏幕区域结构体*/
typedef struct
{
    /* 物理屏幕坐标 (绝对值) */
    int start_line;
    int end_line;

    /* 虚拟缓冲区管理 */
    int mem_start;   // 该区域在 video_buffer 中的起始行下标
    int mem_height;  // 该区域分配的缓冲区总行数
    int view_offset; // 当前视口相对于 mem_start 的偏移量

    /* 光标位置 (物理屏幕坐标，绝对值) */
    int cur_x;
    int cur_y;

    /* 区域属性 */
    int color;
} ConsoleRegion;

extern char fonts[];                                     /* 字体数据 */
extern char video_buffer[VIRT_BUFFER_HEIGHT][NR_CHAR_X]; /* 虚拟屏幕缓冲区 */
extern ConsoleRegion top_region;                         /* 上半屏幕区域 */
extern ConsoleRegion bottom_region;                      /* 下半屏幕区域 */
extern ConsoleRegion *current_focus;                     /* 当前聚焦区域指针 */

/* 初始化与核心功能 */
void con_init(void);
void flush_screen(ConsoleRegion *region);
void update_status_bar(void);
void console_scroll(int direction);
/* 输出与绘图 */
void write_char_with_color(char ascii, int xx, int yy, enum COLOR color);
void delete_char_at(ConsoleRegion *reg, int virt_y, int x);
void region_putc(ConsoleRegion *region, char c);
void set_color(enum COLOR color);
/* 输入处理 */
void handle_editor_input(int key);
/* 调试与辅助 */
void panic(char *s);
void printk(char *buf);

#endif