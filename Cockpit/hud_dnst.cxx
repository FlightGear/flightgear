#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <Aircraft/aircraft.hxx>
#include <Include/fg_constants.h>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>


#include "hud.hxx"
     
//============ Top of dual_instr_item class member definitions ============

dual_instr_item ::
  dual_instr_item ( int          x,
                    int          y,
                    UINT         width,
                    UINT         height,
                    DBLFNPTR     chn1_source,
                    DBLFNPTR     chn2_source,
                    bool         working,
                    UINT         options ):
                  instr_item( x, y, width, height,
                              chn1_source, options, working),
                  alt_data_source( chn2_source )
{
}

dual_instr_item ::
  dual_instr_item( const dual_instr_item & image) :
                 instr_item ((instr_item &) image ),
                 alt_data_source( image.alt_data_source)
{
}

dual_instr_item & dual_instr_item ::
  operator = (const dual_instr_item & rhs )
{
  if( !(this == &rhs)) {
    instr_item::operator = (rhs);
    alt_data_source = rhs.alt_data_source;
    }
  return *this;
}

// End of hud_dnst.cxx

