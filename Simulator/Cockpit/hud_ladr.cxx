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
//====================== Top of HudLadder Class =======================
HudLadder ::   HudLadder(  int       x,
               int       y,
               UINT      width,
               UINT      height,
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
    
inline static void _strokeString(float xx, float yy, char *msg, void *font)
{
    int c;
    if(*msg)
    {
        glTranslatef( xx, yy, 0);
        glScalef(.075, .075, 0.0);
        while ((c=*msg++)) {
            glutStrokeCharacter( font, c);
        }
    }
}

void HudLadder :: draw( void )
{
    float roll_value;
    float pitch_value;
    float    marker_y;
    float    x_ini;
    float    x_end;
    float    y_ini;
    float    y_end;

    int    i;
    POINT  centroid   = get_centroid();
    RECT   box        = get_location();

    int    half_span  = box.right >> 1;
    char   TextLadder[80];

    int    label_length;
    roll_value        = current_ch2();
    
    float hole = (float)((scr_hole)/2);
    
    GLfloat mat[16];
    
    pitch_value       = current_ch1() * RAD_TO_DEG;
    vmin              = pitch_value - (float)width_units/2.0;
    vmax              = pitch_value + (float)width_units/2.0;


    // We will do everything with glLoadMatrix :-)
    // to avoid multiple pushing and popping matrix stack
    glPushMatrix();
    //  glTranslatef( centroid.x, centroid.y, 0);
    //  glRotatef(roll_value * RAD_TO_DEG, 0.0, 0.0, 1.0);
    //  glGetFloatv(GL_MODELVIEW_MATRIX, mat);
    // this is the same as above
    float sinx = sin(roll_value);
    float cosx = cos(roll_value);
    mat[0] = cosx;
    mat[1] = sinx;
    mat[2] = mat[3] = 0.0;
    mat[4] = -sinx;
    mat[5] = cosx;
    mat[6] = mat[7] = mat[8] = mat[9];
    mat[10] = 1.0;
    mat[11] = 0.0;
    mat[12] = centroid.x;
    mat[13] = centroid.y;
    mat[14] = 0;
    mat[15] = 1.0;
    glLoadMatrixf( mat );
    
    // Draw the target spot.
#define CENTER_DIAMOND_SIZE 5.0
    glBegin(GL_LINE_LOOP);
        glVertex2f( -CENTER_DIAMOND_SIZE, 0.0);
        glVertex2f(0.0, CENTER_DIAMOND_SIZE);
        glVertex2f( CENTER_DIAMOND_SIZE, 0.0);
        glVertex2f(0.0, -CENTER_DIAMOND_SIZE);
    glEnd();
#if 0
    glBegin(GL_LINES);
        glVertex2f( -CENTER_DIAMOND_SIZE, 0);
        glVertex2f( -(CENTER_DIAMOND_SIZE*2), 0);
        glVertex2f(0, CENTER_DIAMOND_SIZE);
        glVertex2f(0, (CENTER_DIAMOND_SIZE*2));
        glVertex2f( CENTER_DIAMOND_SIZE, 0);
        glVertex2f( (CENTER_DIAMOND_SIZE*2), 0);
        glVertex2f(0, -CENTER_DIAMOND_SIZE);
        glVertex2f(0, -(CENTER_DIAMOND_SIZE*2));
    glEnd();
#endif // 0
#undef CENTER_DIAMOND_SIZE

    int last =FloatToInt(vmax)+1;
    i = FloatToInt(vmin);
    if( div_units ) {
        if( !scr_hole ) {
            for( ; i <last ; i++ ) {
                
                marker_y =  /*(int)*/(((float)(i - pitch_value) * factor) + .5);
                if( !(i % div_units ))    {        //  At integral multiple of div
                
                    label_length = sprintf( TextLadder, "%d", i );
                
                    if( i ) {
                        x_ini = -half_span;
                    } else {       // Make zero point wider on left
                        x_ini = -half_span - 10;
                    }
                    x_end = half_span;
                    
                    y_ini = marker_y;
                    y_end = marker_y;
                    
//                  printf("(%.1f %.1f) (%.1f %.1f)\n",x_ini,y_ini,x_end,y_end);
                    
                    if( i >= 0 ) { // Above zero draw solid lines
                        glBegin(GL_LINES);
                            glVertex2f( x_ini, y_ini);
                            glVertex2f( x_end, y_end );
                        glEnd();
                    } else { // Below zero draw dashed lines.
                        glEnable(GL_LINE_STIPPLE);
                        glLineStipple( 1, 0x00FF );
                        glBegin(GL_LINES);
                            glVertex2f( x_ini, y_ini);
                            glVertex2f( x_end, y_end );
                        glEnd();
                        glDisable(GL_LINE_STIPPLE);
                    }
                    
                    // Calculate the position of the left text and write it.
                    x_ini -= ( 8*label_length - 4);
                    y_ini -= 4;
                    _strokeString(x_ini, y_ini,
                                  TextLadder, GLUT_STROKE_ROMAN );
                    
                    glLoadMatrixf( mat );
                    // Calculate the position of the right text and write it.
                    x_end -= (24 - 8 * label_length);
                    y_end -= 4;
                    _strokeString(x_end, y_end,
                                  TextLadder, GLUT_STROKE_ROMAN );
                    glLoadMatrixf( mat );
                }
            }
        }
        else // if(scr_hole )
        {    // Draw ladder with space in the middle of the lines
            last =FloatToInt(vmax)+1;
            i = FloatToInt(vmin);
            for( ; i <last ; i++ )      {  
                marker_y =  /*(int)*/(((float)(i - pitch_value) * factor) + .5);
                if( !(i % div_units ))    {        //  At integral multiple of div
                
                    label_length = sprintf( TextLadder, "%d", i );
                
                    // Start by calculating the points and drawing the
                    // left side lines.
                    if( i != 0 )  {
                        x_ini =  -half_span;
                    } else {
                        x_ini = -half_span - 10;
                    }
                    x_end =  -half_span + hole;
                    
                    y_ini = marker_y;
                    y_end = marker_y;
                    
//                  printf("half_span(%d) scr_hole(%d)\n", half_span, scr_hole);
//                  printf("x_end(%f) %f\n",x_end,(float)(-half_span + scr_hole/2));
//                  printf("L: (%.1f %.1f) (%.1f %.1f)\n",x_ini,y_ini,x_end,y_end);
                    
                    if( i >= 0 ) { // Above zero draw solid lines
                        glBegin(GL_LINES);
                            glVertex2f( x_ini, y_ini);
                            glVertex2f( x_end, y_end );
                        glEnd();
                    } else { // Below zero draw dashed lines.
                        glEnable(GL_LINE_STIPPLE);
                        glLineStipple( 1, 0x00FF );
                        glBegin(GL_LINES);
                            glVertex2f( x_ini, y_ini);
                            glVertex2f( x_end, y_end );
                        glEnd();
                        glDisable(GL_LINE_STIPPLE);
                    }
                    
                    // Now calculate the location of the left side label using
                    // the previously calculated start of the left side line.
                    
                    x_ini -= (label_length + 32);
                    if( i < 0) {
                        x_ini -= 8;
                    } else {
                        if( i == 0 ) {
                            x_ini += 15; //20;
                        }
                    }
                    y_ini -= 4;

                    _strokeString(x_ini, y_ini,
                                  TextLadder, GLUT_STROKE_MONO_ROMAN );
                    glLoadMatrixf( mat );
                    
                    // Now calculate and draw the right side line location
                    x_ini = half_span - hole;
                    y_ini = marker_y;
                    if( i != 0 )  {
                        x_end = half_span;
                    } else {
                        x_end = half_span + 10;
                    }
                    y_end = marker_y;
                    
//                  printf("R: (%.1f %.1f) (%.1f %.1f)\n",x_ini,y_ini,x_end,y_end);
                    
                    if( i >= 0 ) { // Above zero draw solid lines
                        glBegin(GL_LINES);
                            glVertex2f( x_ini, y_ini);
                            glVertex2f( x_end, y_end );
                        glEnd();
                    } else { // Below zero draw dashed lines.
                        glEnable(GL_LINE_STIPPLE);
                        glLineStipple( 1, 0x00FF );
                        glBegin(GL_LINES);
                            glVertex2f( x_ini, y_ini);
                            glVertex2f( x_end, y_end );
                        glEnd();
                        glDisable(GL_LINE_STIPPLE);
                    }
                    
                    // Calculate the location and draw the right side label
                    // using the end of the line as previously calculated.
                    x_end -=  (label_length - 24);
                        
                    if( i <= 0 ) {
                        x_end -= 8;
                    } // else if( i==0)
//                  {
//                      x_end -=4;
//                  }
                    
                    y_end  = marker_y - 4;
                    _strokeString(x_end,  y_end,
                                  TextLadder, GLUT_STROKE_MONO_ROMAN );
                    glLoadMatrixf( mat );
                }
            }
        }
    }
    glPopMatrix();
}

