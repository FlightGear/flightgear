
#include "hud.hxx"


UINT instr_item :: instances = 0;  // Initial value of zero
int  instr_item :: brightness = 5;/*HUD_BRT_MEDIUM*/
//glRGBTRIPLE instr_item :: color = {0.1, 0.7, 0.0};
glRGBTRIPLE instr_item :: color = {0.0, 1.0, 0.0};

// constructor    ( No default provided )
instr_item::instr_item(
        int       x,
        int       y,
        UINT      width,
        UINT      height,
        FLTFNPTR  data_source,
        float     data_scaling,
        UINT      options,
        bool      working,
        int       digit) :
    handle         ( ++instances  ),
    load_value_fn  ( data_source  ),
    disp_factor    ( data_scaling ),
    opts           ( options      ),
    is_enabled     ( working      ),
    broken         ( FALSE        ),
    digits         ( digit        )
{
    scrn_pos.left   = x;
    scrn_pos.top    = y;
    scrn_pos.right  = width;
    scrn_pos.bottom = height;

    // Set up convenience values for centroid of the box and
    // the span values according to orientation

    if (opts & HUDS_VERT) { // Vertical style
        // Insure that the midpoint marker will fall exactly at the
        // middle of the bar.
        if (!(scrn_pos.bottom % 2))
            scrn_pos.bottom++;

        scr_span = scrn_pos.bottom;

    } else {
        // Insure that the midpoint marker will fall exactly at the
        // middle of the bar.
        if (!(scrn_pos.right % 2))
            scrn_pos.right++;

        scr_span = scrn_pos.right;
    }

    // Here we work out the centroid for the corrected box.
    mid_span.x = scrn_pos.left   + (scrn_pos.right  >> 1);
    mid_span.y = scrn_pos.top + (scrn_pos.bottom >> 1);
}


// copy constructor
instr_item::instr_item( const instr_item & image ) :
    handle       ( ++instances        ),
    scrn_pos     ( image.scrn_pos     ),
    load_value_fn( image.load_value_fn),
    disp_factor  ( image.disp_factor  ),
    opts         ( image.opts         ),
    is_enabled   ( image.is_enabled   ),
    broken       ( image.broken       ),
    scr_span     ( image.scr_span     ),
    mid_span     ( image.mid_span     )
{
}


instr_item::~instr_item ()
{
    if (instances)
        instances--;
}


void instr_item::update( void )
{
}

// break_display       This is emplaced to provide hooks for making
//                     instruments unreliable. The default behavior is
// to simply not display, but more sophisticated behavior is available
// by over riding the function which is virtual in this class.

void instr_item::break_display ( bool bad )
{
    broken = !!bad;
    is_enabled = FALSE;
}


void instr_item::SetBrightness ( int level  )
{
    brightness = level;   // This is all we will do for now. Later the
                          // brightness levels will be sensitive both to
                          // the control knob and the outside light levels
                          // to emulated night vision effects.
}


UINT instr_item::get_Handle( void )
{
    return handle;
}


