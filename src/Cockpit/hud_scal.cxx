
#include "hud.hxx"


//============== Top of instr_scale class memeber definitions ===============
//
// Notes:
// 1. instr_scales divide the specified location into half and then
//    the half opposite the read direction in half again. A bar is
//    then drawn along the second divider. Scale ticks are drawn
//    between the middle and quarter section lines (minor division
//    markers) or just over the middle line.
//
// 2.  This class was not intended to be instanciated. See moving_scale
//     and gauge_instr classes.
//============================================================================
instr_scale::instr_scale(
        int      x,
        int      y,
        UINT     width,
        UINT     height,
        FLTFNPTR load_fn,
        UINT     options,
        float    show_range,
        float    maxValue,
        float    minValue,
        float    disp_scale,
        UINT     major_divs,
        UINT     minor_divs,
        UINT     rollover,
        int      dp_showing,
        bool     working ) :
    instr_item( x, y, width, height, load_fn, disp_scale, options, working),
    range_shown  ( show_range ),
    Maximum_value( maxValue   ),
    Minimum_value( minValue   ),
    Maj_div      ( major_divs ),
    Min_div      ( minor_divs ),
    Modulo       ( rollover   ),
    signif_digits( dp_showing )
{
    int temp;

    scale_factor = (float)get_span() / range_shown;
    if (show_range < 0)
        range_shown = -range_shown;

    temp = FloatToInt(Maximum_value - Minimum_value) / 100;
    if (range_shown < temp)
        range_shown = temp;
}


instr_scale::instr_scale( const instr_scale & image ) :
    instr_item( (const instr_item &) image),
    range_shown  ( image.range_shown   ),
    Maximum_value( image.Maximum_value ),
    Minimum_value( image.Minimum_value ),
    scale_factor ( image.scale_factor  ),
    Maj_div      ( image.Maj_div       ),
    Min_div      ( image.Min_div       ),
    Modulo       ( image.Modulo        ),
    signif_digits( image.signif_digits )
{
}


instr_scale::~instr_scale ()
{
}


