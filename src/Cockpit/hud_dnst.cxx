#include "hud.hxx"


dual_instr_item::dual_instr_item(
        int          x,
        int          y,
        UINT         width,
        UINT         height,
        FLTFNPTR     chn1_source,
        FLTFNPTR     chn2_source,
        bool         working,
        UINT         options) :
    instr_item(x, y, width, height,
                chn1_source, 1, options, working),
                alt_data_source(chn2_source)
{
}


