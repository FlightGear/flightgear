#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/math/fg_random.h>
#include <simgear/math/polar3d.hxx>

#include <Aircraft/aircraft.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>
#include <GUI/gui.h>

#include "hud.hxx"

#define DO_PANEL_HACK

//====================== Top of HudLadder Class =======================
HudLadder ::   HudLadder(  int       x,
               int       y,
               UINT      width,
               UINT      height,
               UINT      mini,
               FLTFNPTR  ptch_source,
               FLTFNPTR  roll_source,
               float    span_units,
               float    major_div,
               float    minor_div,
               UINT      screen_hole,
               UINT      lbl_pos,
               bool      working) :
   dual_instr_item( x, y, width, height,
                    ptch_source,
                    roll_source,
                    working,
                    HUDS_RIGHT),
                    width_units    ( (int)(span_units)   ),
                    div_units      ( (int)(major_div < 0? -major_div: major_div) ),
                    minor_div      ( (int)(minor_div)    ),
                    label_pos      ( lbl_pos      ),
                    scr_hole       ( screen_hole  ),
                    vmax           ( span_units/2 ),
                    vmin           ( -vmax        )
   {
       if( !width_units ) {
           width_units = 45;
       }
       factor = (float)get_span() / (float) width_units;
       minimal = mini;
   }
   
HudLadder ::  ~HudLadder()
    {
    }
           
    HudLadder ::
            HudLadder( const HudLadder & image ) :
            dual_instr_item( (dual_instr_item &) image),
            width_units    ( image.width_units   ),
            div_units      ( image.div_units     ),
            label_pos      ( image.label_pos     ),
            scr_hole       ( image.scr_hole      ),
            vmax           ( image.vmax ),
            vmin           ( image.vmin ),
            factor         ( image.factor        )
    {
    }
HudLadder & HudLadder ::  operator = ( const HudLadder & rhs )
    {
        if( !(this == &rhs)) {
            (dual_instr_item &)(*this) = (dual_instr_item &)rhs;
            width_units  = rhs.width_units;
            div_units    = rhs.div_units;
            label_pos    = rhs.label_pos;
            scr_hole     = rhs.scr_hole;
            vmax         = rhs.vmax;
            vmin         = rhs.vmin;
            factor       = rhs.factor;
        }
        return *this;
    }
                           
//
//  Draws a climb ladder in the center of the HUD
//

void HudLadder :: draw( void )
{
    POINT  centroid    = get_centroid();
	
    float   roll_value = current_ch2();
    
    glPushMatrix();
    glTranslatef( centroid.x, centroid.y, 0);
	// Alex's panel patch
//	glScalef( current_options.get_fov()/55.0, 1.0, 1.0 );
    glRotatef(roll_value * RAD_TO_DEG, 0.0, 0.0, 1.0);

    // Draw the target spot.
#define CENTER_DIAMOND_SIZE 6.0f
	
    glBegin(GL_LINE_LOOP);
        glVertex2f( CENTER_DIAMOND_SIZE, 0.0);
        glVertex2f( 0.0, CENTER_DIAMOND_SIZE);
        glVertex2f( -CENTER_DIAMOND_SIZE, 0.0);
        glVertex2f( 0.0, -CENTER_DIAMOND_SIZE);
    glEnd();

    if( minimal || ! div_units ) {
        glPopMatrix();      
        return;
    }

	float  x_ini;
	float  x_end;
	float  y;

	float pitch_value = current_ch1() * RAD_TO_DEG;
	vmin              = pitch_value - (float)width_units * 0.5f;
	vmax              = pitch_value + (float)width_units * 0.5f;

	RECT   box         = get_location();

	float   half_span  = box.right * 0.5f ;

	char     TextLadder[8] ;
	float    label_length ;
	float    label_height ;
	float    left ;
	float    right ;
	float    bot ;
	float    top ;
	float    text_offset = 4.0f ;
	float    zero_offset = 10.0f ;
	
	fntFont *font      = HUDtext->getFont();
	float    pointsize = HUDtext->getPointSize();
	float    italic    = HUDtext->getSlant();
	
	TextList.setFont( HUDtext );
	TextList.erase();
	LineList.erase();
	StippleLineList.erase();
	
	int last = FloatToInt(vmax)+1;
	int i    = FloatToInt(vmin);
	
	if( !scr_hole ) {
		x_end =  half_span;
		for( ; i<last ; i++ ) {
                
			y =  (((float)(i - pitch_value) * factor) + .5f);
			if( !(i % div_units ))    {        //  At integral multiple of div
                
				sprintf( TextLadder, "%d", i );
				font->getBBox ( TextLadder, pointsize, italic,
								&left, &right, &bot, &top ) ;
				
				label_length  = right - left;
				label_length += text_offset;
				label_height  = (top - bot) * 0.5f;
                
				x_ini = -half_span;

				if( i >= 0 ) {
					// Make zero point wider on left
					if( i == 0 )
						x_ini -= zero_offset;
					// Zero or above draw solid lines
					Line(x_ini, y, x_end, y);
				} else {
					// Below zero draw dashed lines.
					StippleLine(x_ini, y, x_end, y);
				}
                    
				// Calculate the position of the left text and write it.
				Text( x_ini-label_length, y-label_height, TextLadder );
				Text( x_end+text_offset,  y-label_height, TextLadder );
			}
		}
	} else	{ // scr_hole != 0
		// Draw ladder with space in the middle of the lines
		float  x_ini2;
		float  x_end2;
		float hole = (float)((scr_hole)*0.5f);
		
		x_end  = -half_span + hole;
		x_ini2 =  half_span - hole;
		
		for( ; i<last ; i++ )      {
                
			y = (((float)(i - pitch_value) * factor) + .5);
			if( !(i % div_units ))    {        //  At integral multiple of div
                
				sprintf( TextLadder, "%d", i );
				font->getBBox ( TextLadder, pointsize, italic,
								&left, &right, &bot, &top ) ;
				label_length  = right - left;
				label_length += text_offset;
				label_height  = (top - bot) * 0.5f;
                
				// Start by calculating the points and drawing the
				// left side lines.
				x_ini  = -half_span;
				x_end2 =  half_span;
                    
				if( i >= 0 ) { 
					// Make zero point wider on left
					if( i == 0 ) {
						x_ini -= zero_offset;
						x_end2 += zero_offset;
					}
					// Zero or above draw solid lines
					Line(x_ini, y, x_end, y);
					Line(x_ini2, y, x_end2, y);
				} else {
					// Below zero draw dashed lines.
					StippleLine(x_ini,  y, x_end,  y);
					StippleLine(x_ini2, y, x_end2, y);
				}
				// Calculate the location of the left side label using
				Text( x_ini-label_length, y-label_height, TextLadder );
				// Calculate the location and draw the right side label
				Text( x_end2+text_offset, y-label_height, TextLadder );
			}
		}
	}
	TextList.draw();
	
	LineList.draw();

	glEnable(GL_LINE_STIPPLE);
#ifdef DO_PANEL_HACK
	glLineStipple( 1, current_options.get_panel_status() ? 0x0F0F : 0x00FF );
#else
	glLineStipple( 1, 0x00FF );
#endif
	
	StippleLineList.draw( );
	glDisable(GL_LINE_STIPPLE);

//   } // if(div_units)
    glPopMatrix();
}

