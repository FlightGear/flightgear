
#include "hud.hxx"


//============ Top of dual_instr_item class member definitions ============

dual_instr_item::dual_instr_item(
        int          x,
        int          y,
        UINT         width,
        UINT         height,
        FLTFNPTR     chn1_source,
        FLTFNPTR     chn2_source,
        bool         working,
        UINT         options ) :
    instr_item( x, y, width, height,
                chn1_source, options, working),
                alt_data_source( chn2_source )
{
}


dual_instr_item::dual_instr_item( const dual_instr_item & image) :
    instr_item ((instr_item &) image ),
    alt_data_source( image.alt_data_source)
{
}

