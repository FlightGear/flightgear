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
 * (Log is kept at end of this file)
 **************************************************************************/
#include <Flight/flight.hxx>

#include <string.h>
#include "moon.hxx"
#include <Debug/logstream.hxx>
#include <Objects/texload.h>


#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <math.h>

static GLuint moon_texid;
static GLubyte *moon_texbuf;

/*************************************************************************
 * Moon::Moon(fgTIME *t)
 * Public constructor for class Moon. Initializes the orbital elements and 
 * sets up the moon texture.
 * Argument: The current time.
 * the hard coded orbital elements for Moon are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Moon::Moon(fgTIME *t) :
  CelestialBody(125.1228, -0.0529538083,
		5.1454,    0.00000,
		318.0634,  0.1643573223,
		60.266600, 0.000000,
		0.054900,  0.000000,
		115.3654,  13.0649929509, t)
{
  string tpath, fg_tpath;
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
  tpath = current_options.get_fg_root() + "/Textures/" + "moon.rgb";
  
  if ( (moon_texbuf = read_rgb_texture(tpath.c_str(), &width, &height)) 
       == NULL )
  {
    // Try compressed
    fg_tpath = tpath + ".gz";
    if ( (moon_texbuf = read_rgb_texture(fg_tpath.c_str(), &width, &height)) 
	 == NULL )
    {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"Error in loading moon texture " << tpath );
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
}
/*****************************************************************************
 * void Moon::updatePosition(fgTIME *t, Star *ourSun)
 * this member function calculates the actual topocentric position (i.e.) 
 * the position of the moon as seen from the current position on the surface
 * of the moon. 
 ****************************************************************************/
void Moon::updatePosition(fgTIME *t, Star *ourSun)
{
  double 
    eccAnom, ecl, lonecl, latecl, actTime,
    xv, yv, v, r, xh, yh, zh, xg, yg, zg, xe, ye, ze,
    Ls, Lm, D, F, mpar, gclat, rho, HA, g,
    geoRa, geoDec;
  
  fgAIRCRAFT *air;
  FGState *f;

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
  lonecl = atan2 (yh, xh);
  latecl = atan2(zh, sqrt(xh*xh + yh*yh));

  /* Calculate a number of perturbatioin, i.e. disturbances caused by the 
   * gravitational infuence of the sun and the other major planets.
   * The largest of these even have a name */
  Ls = ourSun->getM() + ourSun->getw();
  Lm = M + w + N;
  D = Lm - Ls;
  F = Lm - N;
  
  lonecl += DEG_TO_RAD * (-1.274 * sin (M - 2*D)
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
  latecl += DEG_TO_RAD * (-0.173 * sin(F-2*D)
			  -0.055 * sin(M - F - 2*D)
			  -0.046 * sin(M + F - 2*D)
			  +0.033 * sin(F + 2*D)
			  +0.017 * sin(2*M + F)
			  );
  r += (-0.58 * cos(M - 2*D)
	-0.46 * cos(2*D)
	);
  FG_LOG(FG_GENERAL, FG_INFO, "Running moon update");
  xg = r * cos(lonecl) * cos(latecl);
  yg = r * sin(lonecl) * cos(latecl);
  zg = r *               sin(latecl);
  
  xe = xg;
  ye = yg * cos(ecl) -zg * sin(ecl);
  ze = yg * sin(ecl) +zg * cos(ecl);

  geoRa  = atan2(ye, xe);
  geoDec = atan2(ze, sqrt(xe*xe + ye*ye));

  // Given the moon's geocentric ra and dec, calculate its 
  // topocentric ra and dec. i.e. the position as seen from the
  // surface of the earth, instead of the center of the earth

  // First calculates the moon's parrallax, that is, the apparent size of the 
  // (equatorial) radius of the earth, as seen from the moon 
  mpar = asin ( 1 / r);
  gclat = f->get_Latitude() - 0.003358 * 
      sin (2 * DEG_TO_RAD * f->get_Latitude() );
  rho = 0.99883 + 0.00167 * cos(2 * DEG_TO_RAD * f->get_Latitude());
  if (geoRa < 0)
    geoRa += (2*FG_PI);
  
  HA = t->lst - (3.8197186 * geoRa);
  g = atan (tan(gclat) / cos ((HA / 3.8197186)));
  rightAscension = geoRa - mpar * rho * cos(gclat) * sin(HA) / cos (geoDec);
  declination = geoDec - mpar * rho * sin (gclat) * sin (g - geoDec) / sin(g);
}


/************************************************************************
 * void Moon::newImage(float ra, float dec)
 *
 * This function regenerates a new visual image of the moon, which is added to
 * solarSystem display list.
 *
 * Arguments: Right Ascension and declination
 *
 * return value: none
 **************************************************************************/
void Moon::newImage(float ra, float dec)
{
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
  glBindTexture(GL_TEXTURE_2D, moon_texid);

  //xglRotatef(-90, 0.0, 0.0, 1.0);
  xglRotatef(((RAD_TO_DEG * ra)- 90.0), 0.0, 0.0, 1.0);
  xglRotatef((RAD_TO_DEG * dec), 1.0, 0.0, 0.0);

  FG_LOG( FG_GENERAL, FG_INFO, 
	  "Ra = (" << (RAD_TO_DEG *ra) 
	  << "), Dec= (" << (RAD_TO_DEG *dec) << ")" );
  xglTranslatef(0.0, 58600.0, 0.0);
  Object = gluNewQuadric();
  gluQuadricTexture( Object, GL_TRUE );   
  gluSphere( Object,  1367, 12, 12 );
  glDisable(GL_TEXTURE_2D);
}







