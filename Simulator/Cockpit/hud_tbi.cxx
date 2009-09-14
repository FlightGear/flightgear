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
//============ Top of fgTBI_instr class member definitions ==============

fgTBI_instr ::
fgTBI_instr( int              x,
             int              y,
             UINT             width,
             UINT             height,
             DBLFNPTR  chn1_source,
             DBLFNPTR  chn2_source,
             double    maxBankAngle,
             double    maxSlipAngle,
             UINT      gap_width,
             bool      working ) :
               dual_instr_item( x, y, width, height,
                                chn1_source,
                                chn2_source,
                                working,
                                HUDS_TOP),
               BankLimit      (maxBankAngle),
               SlewLimit      (maxSlipAngle),
               scr_hole       (gap_width   )
{
}

fgTBI_instr :: ~fgTBI_instr() {}

fgTBI_instr :: fgTBI_instr( const fgTBI_instr & image):
                 dual_instr_item( (const dual_instr_item &) image),
                 BankLimit( image.BankLimit),
                 SlewLimit( image.SlewLimit),
                 scr_hole ( image.scr_hole )
{
}

fgTBI_instr & fgTBI_instr ::
operator = (const fgTBI_instr & rhs )
{
  if( !(this == &rhs)) {
    dual_instr_item::operator = (rhs);
    BankLimit = rhs.BankLimit;
    SlewLimit = rhs.SlewLimit;
    scr_hole  = rhs.scr_hole;
    }
   return *this;
}

//
//	Draws a Turn Bank Indicator on the screen
//

 void fgTBI_instr :: draw( void )
{
  int x_inc1, y_inc1;
  int x_inc2, y_inc2;
  int x_t_inc1, y_t_inc1;

  int d_bottom_x, d_bottom_y;
  int d_right_x, d_right_y;
  int d_top_x, d_top_y;
  int d_left_x, d_left_y;

  int inc_b_x, inc_b_y;
  int inc_r_x, inc_r_y;
  int inc_t_x, inc_t_y;
  int inc_l_x, inc_l_y;
  RECT My_box = get_location();
  POINT centroid = get_centroid();
  int tee_height = My_box.bottom;

//	struct fgFLIGHT *f = &current_aircraft.flight;
  double sin_bank, cos_bank;
  double bank_angle, sideslip_angle;
  double ss_const; // sideslip angle pixels per rad

  bank_angle     = current_ch2();  // Roll limit +/- 30 degrees
  if( bank_angle < -FG_PI_2/3 ) {
    bank_angle = -FG_PI_2/3;
    }
  else
    if( bank_angle > FG_PI_2/3 ) {
      bank_angle = FG_PI_2/3;
      }
  sideslip_angle = current_ch1(); // Sideslip limit +/- 20 degrees
  if( sideslip_angle < -FG_PI/9 ) {
    sideslip_angle = -FG_PI/9;
    }
  else
    if( sideslip_angle > FG_PI/9 ) {
      sideslip_angle = FG_PI/9;
      }

	// sin_bank = sin( FG_2PI-FG_Phi );
	// cos_bank = cos( FG_2PI-FG_Phi );
  sin_bank = sin(FG_2PI-bank_angle);
  cos_bank = cos(FG_2PI-bank_angle);

  x_inc1 = (int)(get_span() * cos_bank);
  y_inc1 = (int)(get_span() * sin_bank);
  x_inc2 = (int)(scr_hole  * cos_bank);
  y_inc2 = (int)(scr_hole  * sin_bank);

  x_t_inc1 = (int)(tee_height * sin_bank);
  y_t_inc1 = (int)(tee_height * cos_bank);

  d_bottom_x = 0;
  d_bottom_y = (int)(-scr_hole);
  d_right_x  = (int)(scr_hole);
  d_right_y  = 0;
  d_top_x    = 0;
  d_top_y    = (int)(scr_hole);
  d_left_x   = (int)(-scr_hole);
  d_left_y   = 0;

  ss_const = (get_span()*2)/(FG_2PI/9);  // width represents 40 degrees

  d_bottom_x += (int)(sideslip_angle*ss_const);
  d_right_x  += (int)(sideslip_angle*ss_const);
  d_left_x   += (int)(sideslip_angle*ss_const);
  d_top_x    += (int)(sideslip_angle*ss_const);

  inc_b_x = (int)(d_bottom_x*cos_bank-d_bottom_y*sin_bank);
  inc_b_y = (int)(d_bottom_x*sin_bank+d_bottom_y*cos_bank);
  inc_r_x = (int)(d_right_x*cos_bank-d_right_y*sin_bank);
  inc_r_y = (int)(d_right_x*sin_bank+d_right_y*cos_bank);
  inc_t_x = (int)(d_top_x*cos_bank-d_top_y*sin_bank);
  inc_t_y = (int)(d_top_x*sin_bank+d_top_y*cos_bank);
  inc_l_x = (int)(d_left_x*cos_bank-d_left_y*sin_bank);
  inc_l_y = (int)(d_left_x*sin_bank+d_left_y*cos_bank);

  if( scr_hole == 0 )
    {
    drawOneLine( centroid.x - x_inc1, centroid.y - y_inc1, \
                 centroid.x + x_inc1, centroid.y + y_inc1 );
    }
  else
    {
    drawOneLine( centroid.x - x_inc1, centroid.y - y_inc1, \
                 centroid.x - x_inc2, centroid.y - y_inc2 );
    drawOneLine( centroid.x + x_inc2, centroid.y + y_inc2, \
                 centroid.x + x_inc1, centroid.y + y_inc1 );
    }

  // draw teemarks
  drawOneLine( centroid.x + x_inc2,            \
                 centroid.y + y_inc2,          \
               centroid.x + x_inc2 + x_t_inc1, \
                 centroid.y + y_inc2 - y_t_inc1 );
  drawOneLine( centroid.x - x_inc2,            \
                 centroid.y - y_inc2,          \
               centroid.x - x_inc2 + x_t_inc1, \
                 centroid.y - y_inc2 - y_t_inc1 );

  // draw sideslip diamond (it is not yet positioned correctly )
  drawOneLine( centroid.x + inc_b_x, \
               centroid.y + inc_b_y, \
               centroid.x + inc_r_x, \
               centroid.y + inc_r_y );
  drawOneLine( centroid.x + inc_r_x, \
               centroid.y + inc_r_y, \
               centroid.x + inc_t_x, \
               centroid.y + inc_t_y );
  drawOneLine( centroid.x + inc_t_x, \
               centroid.y + inc_t_y, \
               centroid.x + inc_l_x, \
               centroid.y + inc_l_y );
  drawOneLine( centroid.x + inc_l_x, \
               centroid.y + inc_l_y, \
               centroid.x + inc_b_x, \
               centroid.y + inc_b_y );

}
