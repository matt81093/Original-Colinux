/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */
 /* OS independent console support functions */

#include <colinux/os/current/memory.h>
#include <colinux/os/alloc.h>

#include "console.h"

/*
 * Console initialization.
 */
static void blank_char(co_console_cell_t *cell)
{
	cell->ch   = 0x20;  /* space 		*/
	cell->attr = 0x07;  /* white on black 	*/
}

/* Create the console object */
co_rc_t co_console_create(long		 x,
			  long	         y,
			  long           max_y,
			  int		 curs_size_prc,
			  co_console_t** console_out)
{
	unsigned long struct_size;
	co_console_t* console;

	if(max_y < y) {
		max_y = y;
	}
	
        if(curs_size_prc < 0 || curs_size_prc > 100) {
		return CO_RC(INVALID_PARAMETER);
	}

	/*
	 * Use only one allocation for the entire console object so it would 
	 * be more managable (and less code).
	 */
	struct_size = sizeof(co_console_cell_t) * max_y * x;
	
        if (struct_size <= 0 || 
            struct_size > sizeof(co_console_cell_t) * CO_CONSOLE_MAX_CHARS) {
		return CO_RC(INVALID_PARAMETER);
	}
	struct_size += sizeof(co_console_t);

	console = co_os_malloc(struct_size);
	if (console == NULL)
		return CO_RC(OUT_OF_MEMORY);

	memset(console, 0, struct_size);

	console->size       = struct_size;
	console->x          = x;
	console->y          = y;
	console->max_y      = max_y;
	console->screen     = ((co_console_cell_t*)((char*)console + sizeof(co_console_t)));
	console->backlog    = console->screen + y * x;
	
	/* Set initial cursor size. 'curs_size_prc' and
	 * 'height' in 'cocon.c' have exactly the same meaning.
	 */
	console->cursor.height = curs_size_prc;
	*console_out 	       = console;

	return CO_RC(OK);
}

/* Free memory of the console object */
void co_console_destroy(co_console_t* console)
{
	co_os_free(console);
}

/*
 * Console message processor. 
 *
 * These messages come from the cocon driver of the coLinux kernel:
 *   CO_OPERATION_CONSOLE_PUTC		- Put single char-attr pair
 *   CO_OPERATION_CONSOLE_PUTCS	        - Put char-attr pair array
 *   CO_OPERATION_CONSOLE_CURSOR_MOVE	- Move cursor
 *   CO_OPERATION_CONSOLE_CURSOR_DRAW	- Move cursor
 *   CO_OPERATION_CONSOLE_CURSOR_ERASE  - Move cursor
 *   CO_OPERATION_CONSOLE_SCROLL_UP	- Scroll lines up
 *   CO_OPERATION_CONSOLE_SCROLL_DOWN	- Scroll lines down
 *   CO_OPERATION_CONSOLE_BMOVE		- Move region up/down
 *   CO_OPERATION_CONSOLE_CLEAR		- Clear region
 *   CO_OPERATION_CONSOLE_STARTUP	- Set screen dimension, ignored in all consoles, used only in cocon.c
 *   CO_OPERATION_CONSOLE_BLANK		- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_SWITCH	- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_FONT_OP	- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_SET_PALETTE	- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_SCROLLDELTA	- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_SET_ORIGIN	- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_SAVE_SCREEN	- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_INVERT_REGION	- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_SIZES		- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_INIT		- Ignored here and in all consoles
 *   CO_OPERATION_CONSOLE_DEINIT	- Ignored here and in all consoles
 */
co_rc_t co_console_op(co_console_t* console, co_console_message_t* message)
{
	switch (message->type) 
	{
	case CO_OPERATION_CONSOLE_SCROLL_UP:
	case CO_OPERATION_CONSOLE_SCROLL_DOWN: {
		unsigned long t     = message->scroll.top;
		unsigned long b     = message->scroll.bottom + 1;
		unsigned long dir;
		unsigned long lines = message->scroll.lines;
		unsigned long x;
		unsigned long y;

		if(message->type == CO_OPERATION_CONSOLE_SCROLL_UP)
			dir = 1;
		else
			dir = 2;

		if (b > console->y)
			return CO_RC(ERROR);

		if (t + lines >= console->y)
			return CO_RC(ERROR);

		if (dir == 1) {
			memmove(&console->screen[console->x * t],
				&console->screen[console->x * (t + lines)],
				console->x*(b - t - lines) * sizeof(co_console_cell_t));

			for (y = b - lines; y < b; y++)
				for (x = 0; x < console->x; x++)
					blank_char(&console->screen[y * console->x + x]);
		}
		else {
			memmove(&console->screen[console->x * (t + lines)],
				&console->screen[console->x * (t)],
				console->x*(b - t - lines) * sizeof(co_console_cell_t));

			for (y = t; y < t + lines; y++)
				for (x = 0; x < console->x; x++)
					blank_char(&console->screen[y*console->x + x]);
		}

		break;
	}
	case CO_OPERATION_CONSOLE_PUTCS: {
		/* Copy array of char-attr pairs to screen */
		int                x     = message->putcs.x, y = message->putcs.y;
		int                count = message->putcs.count;
		co_console_cell_t* cells = (co_console_cell_t *)&message->putcs.data;

		if (y >= console->y  ||  x >= console->x)
			return CO_RC(ERROR);

		while (x < console->x && count > 0) {
			console->screen[y * console->x + x] = *cells;

			cells++;
			x++;
			count--;
		}
		break;
	}
	case CO_OPERATION_CONSOLE_PUTC: {
		/* Copy single char-attr pair to screen */
		int x = message->putc.x;
		int y = message->putc.y;

		if (y >= console->y  ||  x >= console->x)
			return CO_RC(ERROR);
		
		console->screen[y * console->x + x] = 
			*(co_console_cell_t*)(&message->putc.charattr);

		break;
	}
	case CO_OPERATION_CONSOLE_CURSOR_DRAW:
	case CO_OPERATION_CONSOLE_CURSOR_ERASE:
	case CO_OPERATION_CONSOLE_CURSOR_MOVE: {
		console->cursor = message->cursor;
		break;
	}

	case CO_OPERATION_CONSOLE_CLEAR: {
		int		   t = message->clear.top;
		int		   b = message->clear.bottom;
		int		   r = message->clear.right;
		int 		   l;
		co_console_cell_t* cell;

		while (t <= b) {
			l    = message->clear.left;
			cell = &console->screen[t * console->x + l];
			while (l++ <= r) {
				cell->attr = message->clear.charattr >> 8;
				cell->ch   = message->clear.charattr & 0xff;
				cell++;
			}
			t++;
		}
		break;
	}

	case CO_OPERATION_CONSOLE_BMOVE: {
		int y = message->bmove.row;
		int x = message->bmove.column;
		int t = message->bmove.top;
		int l = message->bmove.left;
		int b = message->bmove.bottom;
		int r = message->bmove.right;

		if (y < t) {
			while (t <= b) {
				memmove(&console->screen[y * console->x + x],
					&console->screen[t * console->x + l],
					(r - l + 1) * sizeof(co_console_cell_t));
				t++;
				y++;
			}
		} else	{
			y += b-t;
			while (t <= b) {
				memmove(&console->screen[y * console->x + x],
					&console->screen[b * console->x + l],
					(r - l + 1) * sizeof(co_console_cell_t));
				b--;
				y--;
			}
		}
		break;
	}

	case CO_OPERATION_CONSOLE_STARTUP:
		message->type       = CO_OPERATION_CONSOLE_SIZES;
		message->sizes.cols = console->x;
		message->sizes.rows = console->y;
		
		/* 'backbuf' not used in the guest linux kernel, or in the host, so far */
		message->sizes.backbuf = console->max_y - console->x * console->y;
		
		break;

	case CO_OPERATION_CONSOLE_INIT:
	case CO_OPERATION_CONSOLE_DEINIT:
	case CO_OPERATION_CONSOLE_SWITCH:
	case CO_OPERATION_CONSOLE_BLANK:
	case CO_OPERATION_CONSOLE_FONT_OP:
	case CO_OPERATION_CONSOLE_SET_PALETTE:
	case CO_OPERATION_CONSOLE_SCROLLDELTA:
	case CO_OPERATION_CONSOLE_SET_ORIGIN:
	case CO_OPERATION_CONSOLE_SAVE_SCREEN:
	case CO_OPERATION_CONSOLE_INVERT_REGION:
	case CO_OPERATION_CONSOLE_SIZES:
		break;
	}

	return CO_RC(OK);
}

#if defined UNUSED

void co_console_pickle(co_console_t* console)
{
	console->screen  = (co_console_cell_t*)(((char*)console->screen)  - (unsigned long)console);
	console->backlog = (co_console_cell_t*)(((char*)console->backlog) - (unsigned long)console);
}

void co_console_unpickle(co_console_t* console)
{
	console->screen  = (co_console_cell_t*)(((char*)console->screen)  + (unsigned long)console);
	console->backlog = (co_console_cell_t*)(((char*)console->backlog) + (unsigned long)console);
}

#endif

