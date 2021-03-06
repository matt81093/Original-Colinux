/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#include "console.h"
//#include "screen_cocon.h"
#include "screen_cofb.h"
#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* ----------------------------------------------------------------------- */
/*
 * Class constructor.
 *
 * The size given will be used as the dimensions of the window when not
 * attached.
 *
 * On attach, the current font metrics will dictate the widget size if in
 * text mode, or the virtual framebuffer size if in cofb mode.
 *
 */
console_screen::console_screen( int x, int y, int w, int h )
    : super_( x,y, w,h )
    , video_buffer_( NULL )
    , last_redraw_( 0 )
    , render_( NULL )
{
    // Register check callback that will refresh the screen when needed.
    //remove to use old console Fl::add_check( refresh_callback, this );
}

/* ----------------------------------------------------------------------- */
/*
 * Class destructor.
 */
console_screen::~console_screen( )
{
    engine_clear();
}

/* ----------------------------------------------------------------------- */
/*
 * Checks if screen needs refresh, ordering the redraw if needed.
 *
 * To avoid wasting time redrawing too often, we only refresh the screen
 * if either of these happens:
 *
 *      1) The video buffer signature changed (new driver)
 *      2) The video buffer layout changed (resized or new font)
 *      3) Screen changed and more than 100 msecs elapsed since last redraw
 *      4) No change, but redraw anyway if 500 msecs elapsed (fb mmap case)
 *
 * FIXME: Make the harcoded values changeable and/or test for better ones.
 */
void console_screen::refresh_callback( void* v )
{   
    self_t& this_ = *(self_t*)v;
    bool refresh = false;
    bool resize_widget = false;
    clock_t now = clock();
    clock_t elapsed = 1000*(now - this_.last_redraw_)/CLOCKS_PER_SEC;

    // Lock video memory
    co_video_header * video = this_.video_lock( );
    if ( video == NULL )
    {
        // FIXME: Implement co_os_msleep() and use it
        ::Sleep( 10 );  // Let other threads run (so the lock is released)
        return; // Not attached or already locked. Retry on next callback.
    }
#define IDLE_REFRESH_RATE 100
    // Check buffer signature
    if ( video->magic != this_.engine_id() )
    {   // Set new rendering engine
        refresh = this_.engine_init( video );
    }
    else if ( video->flags & CO_VIDEO_FLAG_INFO_CHANGE )
    {   // Rendering engine layout changed
        this_.render_->reset( );
        video->flags &= ~CO_VIDEO_FLAG_INFO_CHANGE; // clear flag
        resize_widget = true;
    }
    else if ( video->flags & CO_VIDEO_FLAG_DIRTY )
    {   // Check if 100 msecs elapsed since last redraw
        refresh = elapsed > 100? true : false;
    }
    else if ( video->magic == CO_VIDEO_MAGIC_COFB && elapsed > IDLE_REFRESH_RATE )
    {
	/*
	 * This is for the case where linux fb programs write directly
	 * to the mmaped framebuffer memory.
	 * This will ensure we will always draw those changes.
	 */
	refresh = true;
    }

    // Release lock
    this_.video_unlock( video );

    // Redraw if any of the conditions met
    if ( refresh )
        this_.redraw( );

    // Resize widget acording to viewer
    if ( resize_widget )
    {
        this_.size( this_.render_->w(), this_.render_->h() );
        // Signal main window that we were resized, so he can refresh the
        // screen parent (to remove artifacts caused by the resize).
        typedef console_main_window::tm_data_t tm_data_t;
        tm_data_t* data = new tm_data_t(console_main_window::TMSG_VIEW_RESIZE);
        Fl::awake( data );
    }
}

/* ----------------------------------------------------------------------- */
/*
 * Attach shared video bufffer and start rendering it.
 */
void console_screen::attach( void* video_buffer )
{
    video_buffer_ = video_buffer;
    last_redraw_ = 0;
    // Register check callback that will refresh the screen when needed.
    Fl::add_check( refresh_callback, this );
}

/* ----------------------------------------------------------------------- */
/*
 * Dettach from shared video buffer.
 *
 * After this, any attempt to access it should mean a SEGFAULT.
 */
void console_screen::dettach( )
{
    // Clear current render engine
    engine_clear();
    video_buffer_ = NULL;
    Fl::remove_check( refresh_callback, this );
}

/* ----------------------------------------------------------------------- */
/*
 * Initialize rendering engine.
 */
void console_screen::engine_clear(){
    // Clear current rendering engine
    if ( render_ )
        delete render_;
    render_ = NULL;
    // assert video buffer is null
}
// TODO const video?
bool console_screen::engine_init( co_video_header* video )
{
    engine_clear();
    switch ( video->magic )
    {
    case 0:
        return false;   // Video buffer not initialized by kernel driver yet
    case CO_VIDEO_MAGIC_COCON:
        if ( video->size != sizeof(cocon_video_mem_info) )
        {
            Fl::error( "Unexpected video output header size!" );
            return false;
        }
	return false;
        //render_ = new screen_cocon_render( (cocon_video_mem_info*)video );
        break;
    case CO_VIDEO_MAGIC_COFB:
        if ( video->size != sizeof(cofb_video_mem_info) )
        {
            Fl::error( "Unexpected video output header size!" );
            return false;
        }
        render_ = new screen_cofb_render( (cofb_video_mem_info*)video );
        break;
    default:
        Fl::error( "Unexpected video output format!" );
        return false;
    }

    // Resize widget acording to viewer
    size( render_->w(), render_->h() );

    return true;
}

/* ----------------------------------------------------------------------- */
/*
 * Return current rendering engine ID.
 */
unsigned long console_screen::engine_id( ) const
{
    return render_? render_->id() : 0;
}

/* ----------------------------------------------------------------------- */
/*
 * Try to lock the shared video memory.
 *
 * If lock acquired, returns a pointer to the video memory header.
 * If not attached or already locked, returns NULL.
 */
co_video_header* console_screen::video_lock( )
{
    co_video_header* header = (co_video_header *) video_buffer_;
    if ( header == NULL || co_atomic_lock(&header->lock) )
        return NULL;

    // Success! Return pointer to header
    return header;
}

/* ----------------------------------------------------------------------- */
/*
 * Release lock to shared video memory.
 */
void console_screen::video_unlock( co_video_header* header )
{
    co_atomic_unlock( &header->lock );
}

/* ----------------------------------------------------------------------- */
/*
 * Returns true only if it is the cocon renderer active.
 */
bool console_screen::can_mark( ) const
{
    return render_ && render_->id() == CO_VIDEO_MAGIC_COCON;
}

/* ----------------------------------------------------------------------- */
/*
 * Mark text from (x1,y1) to (x2,y2).
 *
 * FIXME: Lock the shared video memory
 */
void console_screen::set_marked_text( int x1,int y1, int x2,int y2 )
{
    if ( render_ )
    {
	int x0 = x();
	int y0 = y();
	render_->mark_set( x1-x0,y1-y0, x2-x0,y2-y0 );
    }
}

/* ----------------------------------------------------------------------- */
/*
 * Get current marked text.
 *
 * FIXME: Lock the shared video memory
 */
unsigned console_screen::get_marked_text( char* buf, unsigned len )
{
    if ( !render_ )
	return 0;
    return render_->mark_get( buf, len );
}

/* ----------------------------------------------------------------------- */
/*
 * Render coLinux screen.
 */
void console_screen::draw( )
{
    // Check if not attached, unknown video driver or buffer not initialized
    if ( !render_ )
    {   // Fill screen with a black color
        fl_color( FL_BLACK );
        fl_rectf( x(),y(), w(), h() );
        return;
    }

    // Lock video memory
    co_video_header * video = video_lock( );
    if ( video == NULL )
        return; // Already locked or not attached. try redraw latter

    // Draw screen
    render_->draw( x(),y() );

    // All done, release lock
    video_unlock( video );
    last_redraw_ = clock();
}

/* ----------------------------------------------------------------------- */
