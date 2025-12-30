#include <xtos.h>
#include <exception.h>
#include <console.h>
#include <memory.h>
#include <stdbool.h>

void main()
{
	// mem_init();
	excp_init(); /*中断初始化*/
	set_timer_period(250);
	int_on(); /*开中断*/
	con_init();
	init_plane();

	bool last_cursor_flash = 0;
	bool last_plane_move = 0;
	while (1)
	{
		if (cursor_flash && !last_cursor_flash)
		{
			write_char('_', x, y);
			last_cursor_flash = cursor_flash;
		}
		else if (!cursor_flash && last_cursor_flash)
		{
			erase_char(x, y);
			last_cursor_flash = cursor_flash;
		}


		if (plane_move && !last_plane_move)
		{
			move_plane(1, 0);
		}
		last_plane_move = plane_move;
	}
}