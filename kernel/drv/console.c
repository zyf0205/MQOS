#include <xtos.h>
#include "console.h"

struct RGB color_map[] = {
		{0, 0, 0, 0},				// BLACK
		{255, 255, 255, 0}, // WHITE
		{255, 0, 0, 0},			// RED    <- 修正：红色是 RGB(255,0,0)
		{0, 255, 0, 0},			// GREEN
		{0, 0, 255, 0},			// BLUE   <- 修正：蓝色是 RGB(0,0,255)
		{255, 255, 0, 0},		// YELLOW <- 修正：黄色是 RGB(255,255,0)
		{0, 255, 255, 0},		// CYAN   <- 修正：青色是 RGB(0,255,255)
		{255, 0, 255, 0}		// MAGENTA
};

char digits_map[] = "0123456789abcdef"; /*供printk_debug打印16进制*/

int x, y;									 /*光标当前位置 0<=x<=160 0<=y<=50*/
int sum_char_x[NR_CHAR_Y]; /*记录每行有多少字符*/
enum COLOR current_color;	 /*字体颜色*/

/*在指定位置写入一个字符*/
void write_char(char ascii, int xx, int yy)
{
	char *font_byte; /*指向显示字符的指针*/
	int row, col;		 /*行列循环变量*/
	char *pos;			 /*指向显存中的像素位置*/
	struct RGB col_rgb = color_map[current_color];

	/*每个字符占16字节，所以要*16,得到要显示的字符的首地址*/
	font_byte = &fonts[(ascii - 32) * CHAR_HEIGHT];
	/*索引从0开始，所以行列不需要再减一*/
	pos = (char *)(VRAM_BASE + (yy * CHAR_HEIGHT * NR_PIX_X + xx * CHAR_WIDTH) * NR_BYTE_PIX);

	for (row = 0; row < CHAR_HEIGHT; row++, font_byte++) /*数据字节++*/
	{
		for (col = 0; col < CHAR_WIDTH; col++) /*一行一行扫描*/
		{
			if (*font_byte & (1 << (7 - col))) /*该像素==1就染色*/
			{
				*pos++ = col_rgb.r;
				*pos++ = col_rgb.g;
				*pos++ = col_rgb.b;
				*pos++ = col_rgb.a;
			}
			else /*否则pos跳过该像素对应显存中的四个字节*/
				pos += NR_BYTE_PIX;
		}
		pos += (NR_PIX_X - CHAR_WIDTH) * NR_BYTE_PIX; /*每一行扫描完后+一行-字符宽的显存地址*/
	}
}

/*擦掉指定位置字符 填充背景颜色即可 指针位置不变*/
void erase_char(int xx, int yy)
{
	int row, col;
	char *pos;

	/* 计算要擦除的字符在显存中的起始地址 */
	pos = (char *)(VRAM_BASE + (yy * CHAR_HEIGHT * NR_PIX_X + xx * CHAR_WIDTH) * NR_BYTE_PIX);
	for (row = 0; row < CHAR_HEIGHT; row++)
	{
		for (col = 0; col < CHAR_WIDTH; col++)
		{
			*pos++ = 0;
			*pos++ = 0;
			*pos++ = 0;
			*pos++ = 0;
		}
		pos += (NR_PIX_X - CHAR_WIDTH) * NR_BYTE_PIX;
	}
}

/*屏幕向上滚动一行 即将显存内容后面一行复制到前一行*/
void scrup()
{
	int i;
	char *from, *to;

	to = (char *)VRAM_BASE;																							 /*第0行*/
	from = (char *)(VRAM_BASE + (CHAR_HEIGHT * NR_PIX_X * NR_BYTE_PIX)); /*第1行*/

	// （800-16)*1280*4->要移动的次数
	/*依次将前一行替换为后一行*/
	for (i = 0; i < (NR_PIX_Y - CHAR_HEIGHT) * NR_PIX_X * NR_BYTE_PIX; i++, to++, from++)
		*to = *from;

	/*擦出最后一行 每行160个字符，擦出160次*/
	for (i = 0; i < NR_CHAR_X; i++)
		erase_char(i, NR_CHAR_Y - 1);

	/*更新每行字符*/
	for (i = 0; i < NR_CHAR_Y - 1; i++)
		sum_char_x[i] = sum_char_x[i + 1];

	sum_char_x[i] = 0; /*最后一行字符为0*/
}

/*回车换行字符 处理*/
void cr_lf()
{
	x = 0;								 /*x设置为0*/
	if (y < NR_CHAR_Y - 1) /*如果y<49,则+1*/
		y++;
	else /*否则卷屏*/
		scrup();
}

/*删除字符*/
void del()
{
	if (x) /*光标不在每行最开始*/
	{
		x--;
		sum_char_x[y] = x; /*更新字符个数*/
	}
	else if (y) /*光标在每行最开始*/
	{
		sum_char_x[y] = 0;
		y--;							 /*退到上一行*/
		x = sum_char_x[y]; /*更新x位置到上一行最后一个字符处*/
	}
	erase_char(x, y); // 光标位置设置成功，擦出字符
}

/**/
void printk(char *buf)
{
	char c;
	int nr = 0;

	while (buf[nr] != '\0') /*计算字符串长度*/
		nr++;

	erase_char(x, y); /*擦除旧光标*/

	/*打印每个字符*/
	while (nr--)
	{
		c = *buf++;
		if (c > 31 && c < 127) /*可打印字符*/
		{
			write_char(c, x, y); /*显示字符*/
			sum_char_x[y] = x;	 /*更新该行最后字符位置，即字符个数*/
			x++;								 /*更新光标位置*/
			if (x >= NR_CHAR_X)	 /*超过行宽*/
				cr_lf();					 /*换行*/
		}
		else if (c == 10 || c == 13) /*换行符 \n \r*/
			cr_lf();
		else if (c == 127) /*退格*/
			del();
		else																		/*无效字符*/
			panic("panic: unsurpported char!\n"); /*打印错误信息，进入死循环*/
	}
	write_char('_', x, y); /*在当前光标位置绘制等待符号*/
}

/*发生错误时调用，打印错误并进入死循环*/
void panic(char *s)
{
	printk(s);
	while (1)
		;
}

void print_debug(char *str, unsigned long val)
{
	int i, j;
	char buffer[20];

	printk(str); /*打印前缀字符串*/
	buffer[0] = '0';
	buffer[1] = 'x';											/*十六进制前缀*/
	for (j = 0, i = 17; j < 16; j++, i--) /*j控制循环次数*/
	{
		buffer[i] = (digits_map[val & 0xfUL]); /*得到低四位*/
		val >>= 4;														 /*右移动四位，即更新低四位*/
	}
	buffer[18] = '\n';
	buffer[19] = '\0'; /*添加回车结束字符*/
	printk(buffer);
}

/*光标位置初始化为0,0*/
void con_init()
{
	x = 0;
	y = 0;
	set_color(BLUE);
}

/*设置颜色*/
void set_color(enum COLOR color)
{
	if (color >= 0 && color < 8)
	{
		current_color = color;
	}
}

#define PLANE_HEIGHT 4
#define PLANE_WIDTH 7

/* 飞机图案 */
char plane_image[PLANE_HEIGHT][PLANE_WIDTH + 1] = {
		"   ^   ",
		"  /|\\  ",
		" /_|_\\ ",
		"  | |  "};

/* 飞机当前位置 */
int plane_x = 70;
int plane_y = 40;

/* 在指定位置绘制飞机 */
void draw_plane(int px, int py)
{
	int i, j;

	for (i = 0; i < PLANE_HEIGHT; i++)
	{
		for (j = 0; j < PLANE_WIDTH; j++)
		{
			if (plane_image[i][j] != ' ')
			{
				write_char(plane_image[i][j], px + j, py + i);
			}
		}
	}
}

/* 清除指定位置的飞机 */
void clear_plane(int px, int py)
{
	int i, j;
	for (i = 0; i < PLANE_HEIGHT; i++)
	{
		for (j = 0; j < PLANE_WIDTH; j++)
		{
			erase_char(px + j, py + i);
		}
	}
}

/* 移动飞机 (dx, dy 是移动方向: -1向左/上, 1向右/下, 0不动) */
void move_plane(int dx, int dy)
{
	/* 清除旧位置 */
	clear_plane(plane_x, plane_y);

	/* 更新坐标 */
	plane_x += dx;
	plane_y += dy;

	/* 边界检查 */
	if (plane_x < 0)
		plane_x = 0;
	if (plane_y < 0)
		plane_y = 0;
	if (plane_x > NR_CHAR_X - PLANE_WIDTH)
		plane_x = NR_CHAR_X - PLANE_WIDTH;
	if (plane_y > NR_CHAR_Y - PLANE_HEIGHT)
		plane_y = NR_CHAR_Y - PLANE_HEIGHT;

	/* 绘制新位置 */
	draw_plane(plane_x, plane_y);
}

/* 初始化并显示飞机 */
void init_plane()
{
	set_color(WHITE);
	plane_x = 0;
	plane_y = 25;
	draw_plane(plane_x, plane_y);
}

/* 打印小鸭子 */
void print_duck()
{
	printk(
			"\n"
			"  _   \n"
			">(.)__\n"
			" (___/\n"
			" '   '\n"
			"\n");
}

/*光标移动*/
void console_move_cursor(int dx, int dy)
{
	// 1. 清除旧光标 (可选：如果你实现了光标闪烁或下划线)
	// 如果没有屏幕缓冲区，这里擦除会导致字符消失，所以暂时只更新坐标
	// 或者绘制一个空格/黑色块来擦除光标形状
	erase_char(x, y);

	// 2. 更新坐标
	x += dx;
	y += dy;

	// 3. 边界检查 (防止移出屏幕)
	if (x < 0)
		x = 0;
	if (x >= NR_CHAR_X)
		x = NR_CHAR_X - 1;

	if (y < 0)
		y = 0;
	if (y >= NR_CHAR_Y)
		y = NR_CHAR_Y - 1;

	// 4. 绘制新光标 (可选)
	// 例如绘制一个下划线表示光标位置
	write_char('_', x, y);

}