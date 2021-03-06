/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "widget.h"
	
static int default_red[] = {0x00,0xaa,0x00,0xaa,0x00,0xaa,0x00,0xaa,
			    0x55,0xff,0x55,0xff,0x55,0xff,0x55,0xff};

static int default_grn[] = {0x00,0x00,0xaa,0xaa,0x00,0x00,0xaa,0xaa,
			    0x55,0x55,0xff,0xff,0x55,0x55,0xff,0xff};

static int default_blu[] = {0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,
			    0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff};

#define FL_RGB_COLOR(i) fl_color(default_blu[i], \
				 default_grn[i], \
				 default_red[i])

console_widget_t::console_widget_t(int		x,
			           int		y,
			           int		w,
			           int 		h,
			           const char* 	label)
: Fl_Widget(x, y, w, h, label)
{
	font_size 	      = 18;
	letter_x	      = font_size;
	letter_y	      = font_size;
	cursor_blink_interval = 0.1;
	cursor_blink_state    = 1;
	console               = NULL;
	fit_x                 = 0;
	fit_y		      = 0;

	Fl::add_timeout(cursor_blink_interval,
		        (Fl_Timeout_Handler)(console_widget_t::static_blink_handler),
		        this);
}

void console_widget_t::static_blink_handler(console_widget_t* widget)
{
	widget->blink_handler();
}

void console_widget_t::blink_handler()
{
	if (console) {
		damage_console(console->cursor.x, console->cursor.y, 1, 1);

		/* 

		For the cursor to blink we would do: cursor_blink_state = !cursor_blink_state

		However, we need to fix a problem with console_idle() which causes timers not
		to execute unless there are input events.
		
		*/
	}

	Fl::add_timeout(cursor_blink_interval,
		        (Fl_Timeout_Handler)(console_widget_t::static_blink_handler), 
		        this);
}

void console_widget_t::set_font_size(int size)
{
	font_size = size;
}

void console_widget_t::damage_console(int x_, int y_, int w_, int h_)
{
	int cx;
	int cy;

	cx = x();
	cy = y();

	cx -= (fit_x - w()) / 2;
	cy -= (fit_y - h()) / 2;

	damage(1, 
	       cx + x_ * letter_x, 
	       cy + y_ * letter_y, 
	       w_ * letter_x, 
	       h_ * letter_y);
}

void console_widget_t::draw()
{
	if (console == NULL) {
		int x_, y_, w_, h_;
		fl_clip_box(x(),y(),w(),h(), x_, y_, w_, h_);
		fl_color(FL_BLACK);
		fl_rectf(x_, y_, w_, h_);
		return;
	}

	fl_font(FL_SCREEN, font_size);

	int x_, y_, w_, h_, cx, cy;	
	fl_clip_box(x(), y(), w(), h(), x_, y_, w_, h_);

	cx = x();
	cy = y();

	cx -= (fit_x - w()) / 2;
	cy -= (fit_y - h()) / 2;
	
	x_ -= cx;
	y_ -= cy;

	int x1 = x_ / letter_x;
	int x2 = (x_ + w_) / letter_x + 1;
	int y1 = y_ / letter_y;
	int y2 = (y_ + h_) / letter_y + 1;

	int yi = 0;

	if (x_ < 0) {
		fl_color(FL_BLACK);
		fl_rectf(x(), y(), -x_, h_);

		x1 = 0;
		cx += x1 * letter_x;
	}

	if (y_ < 0) {
		fl_color(FL_BLACK);
		fl_rectf(cx, y(), fit_x, -y_);
	}

	if (x_ + w_ > fit_x) {
		fl_color(FL_BLACK);
		fl_rectf(cx + fit_x, y(), x_ + w_ - fit_x, h_);

		x2 = console->config.x;
	}

	if (y_ + h_ > fit_y) {
		fl_color(FL_BLACK);
		fl_rectf(cx, cy + fit_y,
			 fit_x, y_ + h_ - fit_y);
	}

	fl_push_clip(x(), y(), w(), h());

	for (yi = y1; yi < y2; yi++) {
		if (yi < 0)
			continue;

		if (yi >= console->config.y)
			break;

		co_console_cell_t* row_start = &console->screen[yi * console->config.x];
		co_console_cell_t* cell;
		co_console_cell_t* start;
		co_console_cell_t* end;
		char               text_buff[0x100];

		start = row_start + x1;
		end   = row_start + x2;
		cell  = start;

		if (end > cell_limit) {
			// co_debug("BUG: end=%p limit=%p row=%p start=%p x1=%d x2=%d y1=%d y2=%d",
			//     end, limit, row_start, start, x1, x2, y1, y2);
			end = cell_limit; // Hack: Fix the overrun!
		}
		
		while (cell < end) {
			while (cell < end  &&  start->attr == cell->attr  &&
			       cell - start < (int)sizeof(text_buff)) {
				text_buff[cell - start] = cell->ch;
				cell++;
			}

			FL_RGB_COLOR((start->attr >> 4) & 0xf);
			fl_rectf(cx + letter_x * (start - row_start), 
				 cy + letter_y * (yi),
				 (cell - start) * letter_x,
				 letter_y);
			
			FL_RGB_COLOR((start->attr) & 0xf);
			fl_draw(text_buff, cell - start, 
				cx + letter_x * (start - row_start), 
				cy + letter_y * (yi + 1) - fl_descent());

			start = cell;
		}

		// The cell under the cursor
		if (cursor_blink_state && console->cursor.y == yi &&
		    console->cursor.x >= x1 && console->cursor.x <= x2 &&
		    console->cursor.height != CO_CUR_NONE) {
			int cursize;

			if (console->cursor.height <= CO_CUR_BLOCK &&
			    console->cursor.height > CO_CUR_NONE)
				cursize = cursize_tab[console->cursor.height];
			else
				cursize = cursize_tab[CO_CUR_DEF];

			fl_color(0xff, 0xff, 0xff);
			fl_rectf(cx + letter_x * console->cursor.x, 
				 cy + letter_y * console->cursor.y + letter_y - cursize,
				 letter_x,
				 cursize);
		}
	}
	
	fl_pop_clip();
}

void console_widget_t::set_console(co_console_t* _console)
{
	console = _console;

	fl_font(FL_SCREEN, font_size);

	// Calculate constats for this font
	letter_x = (int)fl_width('A');
	letter_y = (int)fl_height();

	fit_x = letter_x * console->config.x;
	fit_y = letter_y * console->config.y;

	cell_limit = &console->screen[console->config.y * console->config.x];

	cursize_tab[CO_CUR_UNDERLINE]	= letter_y / 6 + 1; /* round up 0.1667 */
	cursize_tab[CO_CUR_LOWER_THIRD]	= letter_y / 3;
	cursize_tab[CO_CUR_LOWER_HALF]	= letter_y / 2;
	cursize_tab[CO_CUR_TWO_THIRDS]	= (letter_y * 2) / 3 + 1; /* round up 0.667 */
	cursize_tab[CO_CUR_BLOCK]	= letter_y;
	cursize_tab[CO_CUR_DEF]		= cursize_tab[console->config.curs_type_size];
}

co_console_t* console_widget_t::get_console()
{
	co_console_t* _console = console;
	console = NULL;

	return _console;
}

/* Process console messages:
	CO_OPERATION_CONSOLE_PUTC		- Put single char
	CO_OPERATION_CONSOLE_PUTCS		- Put char array
	
	CO_OPERATION_CONSOLE_CURSOR_MOVE	- Move cursor
	CO_OPERATION_CONSOLE_CURSOR_DRAW	- Move cursor
	CO_OPERATION_CONSOLE_CURSOR_ERASE	- Move cursor
	
	CO_OPERATION_CONSOLE_SCROLL_UP		- Scroll lines up
	CO_OPERATION_CONSOLE_SCROLL_DOWN	- Scroll lines down
	CO_OPERATION_CONSOLE_BMOVE		- Move region up/down
	
	CO_OPERATION_CONSOLE_CLEAR		- Clear region
	
	CO_OPERATION_CONSOLE_STARTUP		- Ignored
	CO_OPERATION_CONSOLE_INIT		- Ignored
	CO_OPERATION_CONSOLE_DEINIT		- Ignored
	CO_OPERATION_CONSOLE_SWITCH		- Ignored
	CO_OPERATION_CONSOLE_BLANK		- Ignored
	CO_OPERATION_CONSOLE_FONT_OP		- Ignored
	CO_OPERATION_CONSOLE_SET_PALETTE	- Ignored
	CO_OPERATION_CONSOLE_SCROLLDELTA	- Ignored
	CO_OPERATION_CONSOLE_SET_ORIGIN		- Ignored
	CO_OPERATION_CONSOLE_SAVE_SCREEN	- Ignored
	CO_OPERATION_CONSOLE_INVERT_REGION	- Ignored
	CO_OPERATION_CONSOLE_CONFIG		- Ignored
*/
co_rc_t console_widget_t::handle_console_event(co_console_message_t* message)
{
	co_rc_t		rc;
	co_cursor_pos_t saved_cursor = {0, };
	
	if (!console) {
		return CO_RC(ERROR);
	}

	switch (message->type) 
	{
	case CO_OPERATION_CONSOLE_CURSOR_DRAW:
	case CO_OPERATION_CONSOLE_CURSOR_ERASE:
	case CO_OPERATION_CONSOLE_CURSOR_MOVE:
		// Only for using below
		saved_cursor = console->cursor;
		break;
	default:
		break;
	}

	rc = co_console_op(console, message);
	if (!CO_OK(rc))
		return rc;

	switch (message->type) 
	{
	case CO_OPERATION_CONSOLE_SCROLL_UP:
	case CO_OPERATION_CONSOLE_SCROLL_DOWN: {
		unsigned long t = message->scroll.top;		/* Start of scroll region (row) */
		unsigned long b = message->scroll.bottom + 1;  	/* End of scroll region (row)	*/

		damage_console(0, t, console->config.x, b - t + 1);
		break;
	}
	case CO_OPERATION_CONSOLE_PUTCS: {
		int x       = message->putcs.x;
		int y	    = message->putcs.y;
		int count   = message->putcs.count;
		int  sx	    = x;
		int  scount = 0;

		while (x < console->config.x  &&  count > 0) {
			x++;
			count--;
			scount++;
		}

		damage_console(sx, y, scount, 1);
		break;
	}
	case CO_OPERATION_CONSOLE_PUTC: {
		int x = message->putc.x;
		int y = message->putc.y;
		
		damage_console(x, y, 1, 1);
		break;
	}
	case CO_OPERATION_CONSOLE_CURSOR_MOVE:
	case CO_OPERATION_CONSOLE_CURSOR_DRAW:
	case CO_OPERATION_CONSOLE_CURSOR_ERASE: {
		damage_console(saved_cursor.x, saved_cursor.y, 1, 1);
		damage_console(console->cursor.x, console->cursor.y, 1, 1);
		break;
	}

	case CO_OPERATION_CONSOLE_CLEAR:{
		unsigned t = message->clear.top;
		unsigned l = message->clear.left;
		unsigned b = message->clear.bottom;
		unsigned r = message->clear.right;
		damage_console(l, t, r - l + 1, b - t + 1);
	}

	case CO_OPERATION_CONSOLE_BMOVE:{
		unsigned y = message->bmove.row;
		unsigned x = message->bmove.column;
		unsigned t = message->bmove.top;
		unsigned l = message->bmove.left;
		unsigned b = message->bmove.bottom;
		unsigned r = message->bmove.right;
		damage_console(x, y, r - l + 1, b - t + 1);

	}

	case CO_OPERATION_CONSOLE_STARTUP:
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
	case CO_OPERATION_CONSOLE_CONFIG:
		break;
	}

	return CO_RC(OK);
}

