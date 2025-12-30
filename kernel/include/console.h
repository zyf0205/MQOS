#ifndef CONSOLE_H
#define CONSOLE_H

/*console宏定义---------------------------------------------------------------------*/
#define NR_PIX_X 1280
#define NR_PIX_Y 800
#define CHAR_HEIGHT 16
#define CHAR_WIDTH 8
#define NR_CHAR_X (NR_PIX_X / CHAR_WIDTH)  /* 160 */
#define NR_CHAR_Y (NR_PIX_Y / CHAR_HEIGHT) /* 50 */
#define NR_BYTE_PIX 4
#define VRAM_BASE 0x40000000UL

/* 新增：虚拟缓冲区高度 (比物理屏幕大很多) */
#define VIRT_BUFFER_HEIGHT 200 

/* 定义屏幕区域结构体 */
typedef struct {
    // 物理屏幕坐标 (绝对值)
    int start_line; 
    int end_line;   
    
    // 虚拟缓冲区管理
    int mem_start;   // 该区域在 video_buffer 中的起始行下标
    int mem_height;  // 该区域分配的缓冲区总行数
    int view_offset; // 当前视口相对于 mem_start 的偏移量 (实现了滚屏的关键)

    // 光标位置 (物理屏幕坐标，绝对值)
    int cur_x;      
    int cur_y;      
    
    int color;
} ConsoleRegion;

/*console相关申明---------------------------------------------------------------------*/
extern char fonts[];

struct RGB { unsigned char b, g, r, a; };
enum COLOR { BLACK=0, WHITE=1, RED=2, GREEN=3, BLUE=4, YELLOW=5, CYAN=6, MAGENTA=7 };

/* 全局变量声明 */
// 注意：这里改为 VIRT_BUFFER_HEIGHT
extern char video_buffer[VIRT_BUFFER_HEIGHT][NR_CHAR_X]; 
extern ConsoleRegion top_region;
extern ConsoleRegion bottom_region;
extern ConsoleRegion *current_focus;

/* 函数声明 */
void raw_write_char(char ascii, int xx, int yy);
void write_char_with_color(char ascii, int xx, int yy, enum COLOR color);
void erase_char(int xx, int yy);
void con_init();
void flush_screen();
void region_putc(ConsoleRegion *region, char c);
void console_scroll(int direction);
void handle_editor_input(int key);
void panic(char *);
void printk(char *buf);
void print_debug(char *, unsigned long);
void set_color(enum COLOR color);

#endif