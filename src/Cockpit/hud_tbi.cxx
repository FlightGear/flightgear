#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/fg_random.h>
#include <simgear/mat3.h>
#include <simgear/polar3d.hxx>

#include <Aircraft/aircraft.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>

#include "hud.hxx"


//============ Top of fgTBI_instr class member definitions ==============

fgTBI_instr ::
fgTBI_instr( int              x,
             int              y,
             UINT             width,
             UINT             height,
             FLTFNPTR  chn1_source,
             FLTFNPTR  chn2_source,
             float    maxBankAngle,
             float    maxSlipAngle,
             UINT      gap_width,
             bool      working ) :
               dual_instr_item( x, y, width, height,
                                chn1_source,
                                chn2_source,
                                working,
                                HUDS_TOP),
               BankLimit      ((int)(maxBankAngle)),
               SlewLimit      ((int)(maxSlipAngle)),
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
//  Draws a Turn Bank Indicator on the screen
//

 void fgTBI_instr :: draw( void )
{
     float bank_angle, sideslip_angle;
     float ss_const; // sideslip angle pixels per rad
     float cen_x, cen_y, bank, fspan, tee, hole;

     int span = get_span();
     
     float zero = 0.0;
  
     RECT My_box = get_location();
     POINT centroid = get_centroid();
     int tee_height = My_box.bottom;

     bank_angle     = current_ch2();  // Roll limit +/- 30 degrees
     
     if( bank_angle < -FG_PI_2/3 ) {
         bank_angle = -FG_PI_2/3;
     } else if( bank_angle > FG_PI_2/3 ) {
             bank_angle = FG_PI_2/3;
     }
     
     sideslip_angle = current_ch1(); // Sideslip limit +/- 20 degrees
     
     if( sideslip_angle < -FG_PI/9 ) {
         sideslip_angle = -FG_PI/9;
     } else if( sideslip_angle > FG_PI/9 ) {
             sideslip_angle = FG_PI/9;
     }

     cen_x = centroid.x;
     cen_y = centroid.y;
     bank  = bank_angle * RAD_TO_DEG;
     tee   = -tee_height;
     fspan = span;
     hole  = scr_hole;
     ss_const = 2 * sideslip_angle * fspan/(FG_2PI/9);  // width represents 40 degrees
     
//   printf("side_slip: %f   fspan: %f\n", sideslip_angle, fspan);
//   printf("ss_const: %f   hole: %f\n", ss_const, hole);
     
     glPushMatrix();
     glTranslatef(cen_x, cen_y, zero);
     glRotatef(-bank, zero, zero, 1.0);
    
     glBegin(GL_LINES);
     
     if( !scr_hole )  
     {
         glVertex2f( -fspan, zero );
         glVertex2f(  fspan, zero );
     } else {
         glVertex2f( -fspan, zero );
         glVertex2f( -hole,  zero );
         glVertex2f(  hole,  zero );
         glVertex2f(  fspan, zero );
     }
     // draw teemarks
     glVertex2f(  hole, zero );
     glVertex2f(  hole, tee );
     glVertex2f( -hole, zero );
     glVertex2f( -hole, tee );
     
     glEnd();
     
     glBegin(GL_LINE_LOOP);
         glVertex2f( ss_const,        -hole);
         glVertex2f( ss_const + hole,  zero);
         glVertex2f( ss_const,         hole);
         glVertex2f( ss_const - hole,  zero);
     glEnd();

     glPopMatrix();
}
