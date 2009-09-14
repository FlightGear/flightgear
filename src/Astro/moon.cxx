/**************************************************************************
 * moon.cxx
 * Written by Durk Talsma. Originally started October 1997, for distribution  
 * with the FlightGear project. Version 2 was written in August and 
 * September 1998. This code is based upon algorithms and data kindly 
 * provided by Mr. Paul Schlyter. (pausch@saaf.se). 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 **************************************************************************/

#include <FDM/flight.hxx>

#include <string.h>
#include "moon.hxx"

#include <Debug/logstream.hxx>
#include <Main/options.hxx>
#include <Misc/fgpath.hxx>
#include <Objects/texload.h>

#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <math.h>


/*************************************************************************
 * Moon::Moon(FGTime *t)
 * Public constructor for class Moon. Initializes the orbital elements and 
 * sets up the moon texture.
 * Argument: The current time.
 * the hard coded orbital elements for Moon are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Moon::Moon(FGTime *t) :
  CelestialBody(125.1228, -0.0529538083,
		5.1454,    0.00000,
		318.0634,  0.1643573223,
		60.266600, 0.000000,
		0.054900,  0.000000,
		115.3654,  13.0649929509, t)
{
  int width, height;
  
  FG_LOG( FG_GENERAL, FG_INFO, "Initializing Moon Texture");
#ifdef GL_VERSION_1_1
  xglGenTextures(1, &moon_texid);
  xglBindTexture(GL_TEXTURE_2D, moon_texid);
#elif GL_EXT_texture_object
  xglGenTexturesEXT(1, &moon_texid);
  xglBindTextureEXT(GL_TEXTURE_2D, moon_texid);
#else
#  error port me
#endif

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // load in the texture data
  FGPath tpath( current_options.get_fg_root() );
  tpath.append( "Textures" );
  tpath.append( "moon.rgb" );

  if ( (moon_texbuf = read_rgb_texture(tpath.c_str(), &width, &height)) 
       == NULL )
  {
    // Try compressed
    FGPath fg_tpath = tpath;
    fg_tpath.append( ".gz" );
    if ( (moon_texbuf = read_rgb_texture(fg_tpath.c_str(), &width, &height)) 
	 == NULL )
    {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"Error in loading moon texture " << tpath.str() );
	exit(-1);
    } 
  } 

  glTexImage2D( GL_TEXTURE_2D,
		0,
		GL_RGB,
		256, 256,
		0,
		GL_RGB, GL_UNSIGNED_BYTE,
		moon_texbuf);

  // setup the halo texture
  FG_LOG( FG_GENERAL, FG_INFO, "Initializing Moon Texture");
#ifdef GL_VERSION_1_1
  xglGenTextures(1, &moon_halotexid);
  xglBindTexture(GL_TEXTURE_2D, moon_halotexid);
#elif GL_EXT_texture_object
  xglGenTexturesEXT(1, &moon_halotexid);
  xglBindTextureEXT(GL_TEXTURE_2D, moon_halotexid);
#else
#  error port me
#endif

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  setHalo();
  glTexImage2D( GL_TEXTURE_2D,
		0,
		GL_RGBA,
		256, 256,
		0,
		GL_RGBA, GL_UNSIGNED_BYTE,
		moon_halotexbuf);
  moonObject = gluNewQuadric();
}

Moon::~Moon()
{
  //delete moonObject;
  delete moon_texbuf;
  delete moon_halotexbuf;
}


static int texWidth = 256;	/* 64x64 is plenty */

void Moon::setHalo()
{
  int texSize;
  //void *textureBuf;
  GLubyte *p;
  int i,j;
  double radius;
  
  texSize = texWidth*texWidth;
  
  moon_halotexbuf = new GLubyte[texSize*4];
  if (!moon_halotexbuf) 
    return;  // Ugly!
  
  p = moon_halotexbuf;
  
  radius = (double)(texWidth / 2);
  
  for (i=0; i < texWidth; i++) {
    for (j=0; j < texWidth; j++) {
      double x, y, d;
	    
      x = fabs((double)(i - (texWidth / 2)));
      y = fabs((double)(j - (texWidth / 2)));

      d = sqrt((x * x) + (y * y));
      if (d < radius) 
	{
	  double t = 1.0 - (d / radius); // t is 1.0 at center, 0.0 at edge */
	  // inverse square looks nice 
	  *p = (int)((double)0xff * (t * t));
	  *(p+1) = (int)((double) 0xff * (t*t));
	  *(p+2) = (int)((double) 0xff * (t*t));
	  *(p+3) = 0x11;
	} 
      else
	{
	  *p = 0x00;
	  *(p+1) = 0x00;
	  *(p+2) = 0x00;
	  *(p+3) = 0x11;
	}
      p += 4;
    }
  }
  //gluBuild2DMipmaps(GL_TEXTURE_2D, 1, texWidth, texWidth, 
  //	    GL_LUMINANCE,
  //	    GL_UNSIGNED_BYTE, textureBuf);
  //free(textureBuf);
}


/*****************************************************************************
 * void Moon::updatePosition(FGTime *t, Star *ourSun)
 * this member function calculates the actual topocentric position (i.e.) 
 * the position of the moon as seen from the current position on the surface
 * of the moon. 
 ****************************************************************************/
void Moon::updatePosition(FGTime *t, Star *ourSun)
{
  double 
    eccAnom, ecl, actTime,
    xv, yv, v, r, xh, yh, zh, xg, yg, zg, xe, ye, ze,
    Ls, Lm, D, F, mpar, gclat, rho, HA, g,
    geoRa, geoDec;
  
  fgAIRCRAFT *air;
  FGInterface *f;

  air = &current_aircraft;
  f = air->fdm_state;
 
  updateOrbElements(t);
  actTime = fgCalcActTime(t);

  // calculate the angle between ecliptic and equatorial coordinate system
  // in Radians
  ecl = ((DEG_TO_RAD * 23.4393) - (DEG_TO_RAD * 3.563E-7) * actTime);  
  eccAnom = fgCalcEccAnom(M, e);  // Calculate the eccentric anomaly
  xv = a * (cos(eccAnom) - e);
  yv = a * (sqrt(1.0 - e*e) * sin(eccAnom));
  v = atan2(yv, xv);               // the moon's true anomaly
  r = sqrt (xv*xv + yv*yv);       // and its distance
  
  // estimate the geocentric rectangular coordinates here
  xh = r * (cos(N) * cos (v+w) - sin (N) * sin(v+w) * cos(i));
  yh = r * (sin(N) * cos (v+w) + cos (N) * sin(v+w) * cos(i));
  zh = r * (sin(v+w) * sin(i));

  // calculate the ecliptic latitude and longitude here
  lonEcl = atan2 (yh, xh);
  latEcl = atan2(zh, sqrt(xh*xh + yh*yh));

  /* Calculate a number of perturbatioin, i.e. disturbances caused by the 
   * gravitational infuence of the sun and the other major planets.
   * The largest of these even have a name */
  Ls = ourSun->getM() + ourSun->getw();
  Lm = M + w + N;
  D = Lm - Ls;
  F = Lm - N;
  
  lonEcl += DEG_TO_RAD * (-1.274 * sin (M - 2*D)
			  +0.658 * sin (2*D)
			  -0.186 * sin(ourSun->getM())
			  -0.059 * sin(2*M - 2*D)
			  -0.057 * sin(M - 2*D + ourSun->getM())
			  +0.053 * sin(M + 2*D)
			  +0.046 * sin(2*D - ourSun->getM())
			  +0.041 * sin(M - ourSun->getM())
			  -0.035 * sin(D)
			  -0.031 * sin(M + ourSun->getM())
			  -0.015 * sin(2*F - 2*D)
			  +0.011 * sin(M - 4*D)
			  );
  latEcl += DEG_TO_RAD * (-0.173 * sin(F-2*D)
			  -0.055 * sin(M - F - 2*D)
			  -0.046 * sin(M + F - 2*D)
			  +0.033 * sin(F + 2*D)
			  +0.017 * sin(2*M + F)
			  );
  r += (-0.58 * cos(M - 2*D)
	-0.46 * cos(2*D)
	);
  FG_LOG(FG_GENERAL, FG_INFO, "Running moon update");
  xg = r * cos(lonEcl) * cos(latEcl);
  yg = r * sin(lonEcl) * cos(latEcl);
  zg = r *               sin(latEcl);
  
  xe = xg;
  ye = yg * cos(ecl) -zg * sin(ecl);
  ze = yg * sin(ecl) +zg * cos(ecl);

  geoRa  = atan2(ye, xe);
  geoDec = atan2(ze, sqrt(xe*xe + ye*ye));

  /* FG_LOG( FG_GENERAL, FG_INFO, 
	  "(geocentric) geoRa = (" << (RAD_TO_DEG * geoRa) 
	  << "), geoDec= (" << (RAD_TO_DEG * geoDec) << ")" ); */


  // Given the moon's geocentric ra and dec, calculate its 
  // topocentric ra and dec. i.e. the position as seen from the
  // surface of the earth, instead of the center of the earth

  // First calculate the moon's parrallax, that is, the apparent size of the 
  // (equatorial) radius of the earth, as seen from the moon 
  mpar = asin ( 1 / r);
  // FG_LOG( FG_GENERAL, FG_INFO, "r = " << r << " mpar = " << mpar );
  // FG_LOG( FG_GENERAL, FG_INFO, "lat = " << f->get_Latitude() );

  gclat = f->get_Latitude() - 0.003358 * 
      sin (2 * DEG_TO_RAD * f->get_Latitude() );
  // FG_LOG( FG_GENERAL, FG_INFO, "gclat = " << gclat );

  rho = 0.99883 + 0.00167 * cos(2 * DEG_TO_RAD * f->get_Latitude());
  // FG_LOG( FG_GENERAL, FG_INFO, "rho = " << rho );
  
  if (geoRa < 0)
    geoRa += (2*FG_PI);
  
  HA = t->getLst() - (3.8197186 * geoRa);
  /* FG_LOG( FG_GENERAL, FG_INFO, "t->getLst() = " << t->getLst() 
	  << " HA = " << HA ); */

  g = atan (tan(gclat) / cos ((HA / 3.8197186)));
  // FG_LOG( FG_GENERAL, FG_INFO, "g = " << g );

  rightAscension = geoRa - mpar * rho * cos(gclat) * sin(HA) / cos (geoDec);
  declination = geoDec - mpar * rho * sin (gclat) * sin (g - geoDec) / sin(g);

  /* FG_LOG( FG_GENERAL, FG_INFO, 
	  "Ra = (" << (RAD_TO_DEG *rightAscension) 
	  << "), Dec= (" << (RAD_TO_DEG *declination) << ")" ); */
}


/************************************************************************
 * void Moon::newImage()
 *
 * This function regenerates a new visual image of the moon, which is added to
 * solarSystem display list.
 *
 * Arguments: Right Ascension and declination
 *
 * return value: none
 **************************************************************************/
void Moon::newImage()
{
  fgLIGHT *l = &cur_light_params;
  float moon_angle = l->moon_angle;
  
  /*double x_2, x_4, x_8, x_10;
  GLfloat ambient;
  GLfloat amb[4];*/
  int moonSize = 750;

  GLfloat moonColor[4] = {0.85, 0.75, 0.35, 1.0};
  GLfloat black[4] = {0.0, 0.0, 0.0, 1.0};
  GLfloat white[4] = {1.0, 1.0, 1.0, 0.0};
  
  if( moon_angle*RAD_TO_DEG < 100 ) 
    {
      FG_LOG( FG_ASTRO, FG_INFO, "Generating Moon Image" );

      xglPushMatrix();
      {
	xglRotatef(((RAD_TO_DEG * rightAscension)- 90.0), 0.0, 0.0, 1.0);
	xglRotatef((RAD_TO_DEG * declination), 1.0, 0.0, 0.0);
	
	FG_LOG( FG_GENERAL, FG_INFO, 
		"Ra = (" << (RAD_TO_DEG *rightAscension) 
		<< "), Dec= (" << (RAD_TO_DEG *declination) << ")" );
	xglTranslatef(0.0, 60000.0, 0.0);
	glEnable(GL_BLEND);	// BLEND ENABLED

	// Draw the halo...
	if (current_options.get_textures())
	  {
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	    glEnable(GL_TEXTURE_2D); // TEXTURE ENABLED
	    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  
	    glBindTexture(GL_TEXTURE_2D, moon_halotexid);
	  
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.0f, 0.0f); glVertex3f(-5000, 0.0, -5000);
	    glTexCoord2f(1.0f, 0.0f); glVertex3f( 5000, 0.0, -5000);
	    glTexCoord2f(1.0f, 1.0f); glVertex3f( 5000, 0.0,  5000);
	    glTexCoord2f(0.0f, 1.0f); glVertex3f(-5000, 0.0,  5000);
	    glEnd();
	  }
	
	xglEnable(GL_LIGHTING);	// LIGHTING ENABLED
	xglEnable( GL_LIGHT0 );
	// set lighting parameters
	xglLightfv(GL_LIGHT0, GL_AMBIENT, white );
	xglLightfv(GL_LIGHT0, GL_DIFFUSE, white );
	// xglEnable( GL_CULL_FACE );
	xglMaterialfv(GL_FRONT, GL_AMBIENT, black);
	xglMaterialfv(GL_FRONT, GL_DIFFUSE, moonColor); 
	
	//glEnable(GL_TEXTURE_2D);
	
	glBlendFunc(GL_ONE, GL_ONE);
	
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
	// Draw the moon-proper
	
	if (current_options.get_textures())
	  {
	    glBindTexture(GL_TEXTURE_2D, moon_texid);                         
	    gluQuadricTexture(moonObject, GL_TRUE );
	  }
	gluSphere(moonObject,  moonSize, 12, 12 );
	glDisable(GL_TEXTURE_2D); // TEXTURE DISABLED
	glDisable(GL_BLEND);	// BLEND DISABLED
      }
      xglPopMatrix();
      glDisable(GL_LIGHTING);	// Lighting Disabled.
      
    }
  else
    {
    }
}
