#ifndef CONSOLE_H
#define CONSOLE_H

/*console宏定义---------------------------------------------------------------------*/
#define NR_PIX_X 1280                      /*宽*/
#define NR_PIX_Y 800                       /*高*/
#define CHAR_HEIGHT 16                     /*每个字符高*/
#define CHAR_WIDTH 8                       /*每个字符宽*/
#define NR_CHAR_X (NR_PIX_X / CHAR_WIDTH)  /*每行装160个字符*/
#define NR_CHAR_Y (NR_PIX_Y / CHAR_HEIGHT) /*每列装50个字符*/
#define NR_BYTE_PIX 4                      /*每个像素占四字节rgb+透明度*/
#define VRAM_BASE 0x40000000UL             /*视频显存基地址*/

/*console相关申明---------------------------------------------------------------------*/
extern char fonts[]; /*字体数组*/

struct RGB
{
  unsigned char b, g, r, a;
};

enum COLOR
{
  BLACK = 0,
  WHITE = 1,
  RED = 2,
  GREEN = 3,
  BLUE = 4,
  YELLOW = 5,
  CYAN = 6,
  MAGENTA = 7
};
extern int x, y; /*光标位置*/

void raw_write_char(char ascii, int xx, int yy);
void write_char(char ascii, int xx, int yy);
void erase_char(int xx, int yy);
void printk(char *);
void con_init();
void panic(char *);
void print_debug(char *, unsigned long);
void set_color(enum COLOR color); /*对字符颜色和透明度封装*/
void print_duck();
/* 飞机相关函数 */
void init_plane(void);
void move_plane(int dx, int dy);
/*光标移动*/
void draw_cursor();
void restore_char_at_cursor();
void console_move_cursor(int dx, int dy);
/* 插入字符：将当前位置及之后的字符右移 */
void console_insert_char(char c);
#endif