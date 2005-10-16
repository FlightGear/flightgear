// hud_rwy.cxx -- An instrument that renders a virtual runway on the HUD
//
// Written by Aaron Wilson & Phillip Merritt, Nov 2004.
//
// Copyright (C) 2004 Aaron Wilson, Aaron.I.Wilson@nasa.gov
// Copyright (C) 2004 Phillip Merritt, Phillip.M.Merritt@nasa.gov
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <simgear/compiler.h>

#include "hud.hxx"

#include <math.h>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Aircraft/aircraft.hxx>
#include <Environment/environment.hxx>
#include <Environment/environment_mgr.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/polar3d.hxx>
#include SG_GLU_H
#include <ATC/ATCutils.hxx>

runway_instr::runway_instr(int x,
						   int y,
						   int width,
						   int height,
						   float scale_data,
						   bool working):instr_item(x,y,width,height,NULL,scale_data,0,working)
{
	stippleOut=0xFFFF;
	stippleCen=0xFFFF;
	arrowScale = 1.0;
	arrowRad = 0.0;
	view[0] = 0;
	view[1] = 0;
	view[2] = 640;
	view[3] = 480;	
	center.x = view[2]>>1;
	center.y = view[3]>>1;
	location.left = center.x-(width>>1)+x;
	location.right = center.x+(width>>1)+x;
	location.bottom = center.y-(height>>1)+y;
	location.top = center.y+(height>>1)+y;
	cockpit_view = globals->get_viewmgr()->get_view(0);	
	default_heading = fgGetDouble("/sim/view[0]/config/pitch-heading-deg",0.0);
	default_pitch = fgGetDouble("/sim/view[0]/config/pitch-pitch-deg",0.0);
}

void runway_instr::draw() {
	if (!is_broken() && get_active_runway(runway)) {	
		glPushAttrib(GL_LINE_STIPPLE | GL_LINE_STIPPLE_PATTERN | GL_LINE_WIDTH);
		float modelView[4][4],projMat[4][4]; 	
		bool anyLines;
		//Get the current view
		FGViewer* curr_view = globals->get_viewmgr()->get_current_view();		 
		int curr_view_id = globals->get_viewmgr()->get_current();		
		double gpo = curr_view->getGoalPitchOffset_deg();
		double gho = curr_view->getGoalHeadingOffset_deg();
		double po = curr_view->getPitchOffset_deg();
		double ho = curr_view->getHeadingOffset_deg();
		
		double yaw = -(cockpit_view->getHeadingOffset_deg()-default_heading)*SG_DEGREES_TO_RADIANS;
		double pitch = (cockpit_view->getPitchOffset_deg()-default_pitch)*SG_DEGREES_TO_RADIANS;		
		//double roll = fgGetDouble("/sim/view[0]/config/roll-offset-deg",0.0) //TODO: adjust for default roll offset
		double sPitch = sin(pitch), cPitch = cos(pitch),
			   sYaw = sin(yaw), cYaw = cos(yaw);

		//Assuming that the "Cockpit View" is always at position zero!!!
		if (curr_view_id != 0) {
			globals->get_viewmgr()->set_view(0);
			globals->get_viewmgr()->copyToCurrent();			
		}		
		//Set the camera to the cockpit view to get the view of the runway from the cockpit
		ssgSetCamera((sgVec4 *)cockpit_view->get_VIEW());
		get_rwy_points(points3d);	 
		//Get the current project matrix
		ssgGetProjectionMatrix(projMat);
//		const sgVec4 *viewMat = globals->get_current_view()->get_VIEW();
		//Get the current model view matrix (cockpit view)
		ssgGetModelviewMatrix(modelView);
		//Create a rotation matrix to correct for any offsets (other than default offsets) to the model view matrix
		sgMat4 xy; //rotation about the Rxy, negate the sin's on Ry
		xy[0][0]=cYaw;			xy[1][0]=0.0f;		xy[2][0]=-sYaw;			xy[3][0]=0.0f;
		xy[0][1]=sPitch*-sYaw;	xy[1][1]=cPitch;	xy[2][1]=-sPitch*cYaw;	xy[3][1]=0.0f;
		xy[0][2]=cPitch*sYaw;	xy[1][2]=sPitch;	xy[2][2]=cPitch*cYaw;	xy[3][2]=0.0f;
		xy[0][3]=0.0f;			xy[1][3]=0.0f;		xy[2][3]=0.0f;			xy[3][3]=1.0f;
		//Re-center the model view
		sgPostMultMat4(modelView,xy);
		//copy float matrices to double		
		for (int i=0; i<4; i++) {
			for (int j=0; j<4; j++) {
				int idx = (i*4)+j;
				mm[idx] = (double)modelView[i][j];
				pm[idx] = (double)projMat[i][j];								
			}
		}	
		//Calculate the 2D points via gluProject
		int result = GL_TRUE;
		for (int i=0; i<6; i++) {
			result = gluProject(points3d[i][0],points3d[i][1],points3d[i][2],mm,pm,view,&points2d[i][0],&points2d[i][1],&points2d[i][2]);										
		}
		//set the line width based on our distance from the runway
		setLineWidth();
		//Draw the runway lines on the HUD
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(1,stippleOut);	
		anyLines = 
		drawLine(points3d[0],points3d[1],points2d[0],points2d[1]) | //draw top
		drawLine(points3d[2],points3d[1],points2d[2],points2d[1]) | //draw right
		drawLine(points3d[2],points3d[3],points2d[2],points2d[3]) | //draw bottom
		drawLine(points3d[3],points3d[0],points2d[3],points2d[0]);  //draw left
		glLineStipple(1,stippleCen);
		anyLines |=	drawLine(points3d[5],points3d[4],points2d[5],points2d[4]); //draw center			
		//Check to see if arrow needs drawn
		if ((!anyLines && drawIA) || drawIAAlways) {
			drawArrow(); //draw indication arrow
		}
		//Restore the current view and any offsets
		if (curr_view_id != 0) {
			globals->get_viewmgr()->set_view(curr_view_id);
			globals->get_viewmgr()->copyToCurrent();
			curr_view->setHeadingOffset_deg(ho);
			curr_view->setPitchOffset_deg(po);
			curr_view->setGoalHeadingOffset_deg(gho);
			curr_view->setGoalPitchOffset_deg(gpo);
		}
		//Set the camera back to the current view
		ssgSetCamera((sgVec4 *)curr_view);
		glPopAttrib();
	}//if not broken
}

bool runway_instr::get_active_runway(FGRunway& runway) {
  FGEnvironment stationweather =
      ((FGEnvironmentMgr *)globals->get_subsystem("environment"))->getEnvironment();
  double hdg = stationweather.get_wind_from_heading_deg();  
  return globals->get_runways()->search( fgGetString("/sim/presets/airport-id"), int(hdg), &runway);
}

void runway_instr::get_rwy_points(sgdVec3 *points3d) {	
	static Point3D center = globals->get_scenery()->get_center();
	
	//Get the current tile center
	Point3D currentCenter = globals->get_scenery()->get_center();
	Point3D tileCenter = currentCenter;
	if (center != currentCenter) //if changing tiles
		tileCenter = center; //use last center
	double alt = current_aircraft.fdm_state->get_Runway_altitude()*SG_FEET_TO_METER;
	double length = (runway._length/2.0)*SG_FEET_TO_METER;
	double width = (runway._width/2.0)*SG_FEET_TO_METER;
	double frontLat,frontLon,backLat,backLon,az,tempLat,tempLon;
	
	geo_direct_wgs_84(alt,runway._lat,runway._lon,runway._heading,length,&backLat,&backLon,&az);
	sgGeodToCart(backLat*SG_DEGREES_TO_RADIANS,backLon*SG_DEGREES_TO_RADIANS,alt,points3d[4]);

	geo_direct_wgs_84(alt,runway._lat,runway._lon,runway._heading+180,length,&frontLat,&frontLon,&az);	
	sgGeodToCart(frontLat*SG_DEGREES_TO_RADIANS,frontLon*SG_DEGREES_TO_RADIANS,alt,points3d[5]);

	geo_direct_wgs_84(alt,backLat,backLon,runway._heading+90,width,&tempLat,&tempLon,&az);
	sgGeodToCart(tempLat*SG_DEGREES_TO_RADIANS,tempLon*SG_DEGREES_TO_RADIANS,alt,points3d[0]);

	geo_direct_wgs_84(alt,backLat,backLon,runway._heading-90,width,&tempLat,&tempLon,&az);
	sgGeodToCart(tempLat*SG_DEGREES_TO_RADIANS,tempLon*SG_DEGREES_TO_RADIANS,alt,points3d[1]);

	geo_direct_wgs_84(alt,frontLat,frontLon,runway._heading-90,width,&tempLat,&tempLon,&az);
	sgGeodToCart(tempLat*SG_DEGREES_TO_RADIANS,tempLon*SG_DEGREES_TO_RADIANS,alt,points3d[2]);

	geo_direct_wgs_84(alt,frontLat,frontLon,runway._heading+90,width,&tempLat,&tempLon,&az);
	sgGeodToCart(tempLat*SG_DEGREES_TO_RADIANS,tempLon*SG_DEGREES_TO_RADIANS,alt,points3d[3]);
	
	for(int i = 0; i < 6; i++)
	{
		points3d[i][0] -= tileCenter.x();
		points3d[i][1] -= tileCenter.y();
		points3d[i][2] -= tileCenter.z();
	}
	center = currentCenter;
}

bool runway_instr::drawLine(sgdVec3 a1, sgdVec3 a2, sgdVec3 point1, sgdVec3 point2) {
	sgdVec3 p1, p2;
	sgdCopyVec3(p1, point1);
	sgdCopyVec3(p2, point2);
	bool p1Inside = (p1[0]>=location.left && p1[0]<=location.right && p1[1]>=location.bottom && p1[1]<=location.top); 
	bool p1Insight = (p1[2] >= 0.0 && p1[2] < 1.0);
	bool p1Valid = p1Insight && p1Inside;
	bool p2Inside = (p2[0]>=location.left && p2[0]<=location.right && p2[1]>=location.bottom && p2[1]<=location.top);
	bool p2Insight = (p2[2] >= 0.0 && p2[2] < 1.0);
	bool p2Valid = p2Insight && p2Inside;
	
	if (p1Valid && p2Valid) { //Both project points are valid, draw the line
		glBegin(GL_LINES);
		glVertex2d(p1[0],p1[1]);
		glVertex2d(p2[0],p2[1]);
		glEnd();
	}
	else if (p1Valid) { //p1 is valid and p2 is not, calculate a new valid point
		sgdVec3 vec = {a2[0]-a1[0], a2[1]-a1[1], a2[2]-a1[2]};
		//create the unit vector
		sgdScaleVec3(vec,1.0/sgdLengthVec3(vec));
		sgdVec3 newPt;
		sgdCopyVec3(newPt,a1);
		sgdAddVec3(newPt,vec);
		if (gluProject(newPt[0],newPt[1],newPt[2],mm,pm,view,&p2[0],&p2[1],&p2[2]) && (p2[2]>0&&p2[2]<1.0) ) {
			boundPoint(p1,p2);
			glBegin(GL_LINES);
			glVertex2d(p1[0],p1[1]);
			glVertex2d(p2[0],p2[1]);
			glEnd();
		}
	}
	else if (p2Valid) { //p2 is valid and p1 is not, calculate a new valid point
		sgdVec3 vec = {a1[0]-a2[0], a1[1]-a2[1], a1[2]-a2[2]};
		//create the unit vector
		sgdScaleVec3(vec,1.0/sgdLengthVec3(vec));
		sgdVec3 newPt;
		sgdCopyVec3(newPt,a2);
		sgdAddVec3(newPt,vec);
		if (gluProject(newPt[0],newPt[1],newPt[2],mm,pm,view,&p1[0],&p1[1],&p1[2]) && (p1[2]>0&&p1[2]<1.0)) {
			boundPoint(p2,p1);
			glBegin(GL_LINES);
			glVertex2d(p2[0],p2[1]);
			glVertex2d(p1[0],p1[1]);
			glEnd();
		}
	}
	else if (p1Insight && p2Insight) { //both points are insight, but not inside
		bool v = boundOutsidePoints(p1,p2);
		if (v) {
			glBegin(GL_LINES);
				glVertex2d(p1[0],p1[1]);
				glVertex2d(p2[0],p2[1]);
			glEnd();
		}
		return v;
	}
	//else both points are not insight, don't draw anything
    return (p1Valid && p2Valid);
}

void runway_instr::boundPoint(sgdVec3 v, sgdVec3 m) {
	double y = v[1];
	if(m[1] < v[1]) {
		y = location.bottom;
	}
	else if(m[1] > v[1]) {
		y = location.top;
	}
	if (m[0] == v[0]) {
		m[1]=y;
		return;  //prevent divide by zero
	}
	double slope = (m[1]-v[1])/(m[0]-v[0]);
	m[0] = (y-v[1])/slope + v[0];
	m[1] = y;
	if (m[0] < location.left) {
		m[0] = location.left;			
		m[1] = slope * (location.left-v[0])+v[1];
	}
	else if (m[0] > location.right) {
		m[0] = location.right;
		m[1] = slope * (location.right-v[0])+v[1];
	}
}

bool runway_instr::boundOutsidePoints(sgdVec3 v, sgdVec3 m) {
	bool pointsInvalid = (v[1]>location.top && m[1]>location.top) ||
		               (v[1]<location.bottom && m[1]<location.bottom) ||
					   (v[0]>location.right && m[0]>location.right) ||
					   (v[0]<location.left && m[0]<location.left);
	if (pointsInvalid)
		return false;
	if (m[0] == v[0]) {//x's are equal, vertical line	
		if (m[1]>v[1]) {
			m[1]=location.top;
			v[1]=location.bottom;
		}
		else {
			v[1]=location.top;
			m[1]=location.bottom;
		}
		return true;
	}
	if (m[1] == v[1]) { //y's are equal, horizontal line
		if (m[0] > v[0]) {
			m[0] = location.right;
			v[0] = location.left;
		}
		else {
			v[0] = location.right;
			m[0] = location.left;
		}
		return true;
	}
	double slope = (m[1]-v[1])/(m[0]-v[0]);
	double b = v[1]-(slope*v[0]);
	double y1 = slope * location.left + b;
	double y2 = slope * location.right + b;
	double x1 = (location.bottom - b) / slope;
	double x2 = (location.top - b) / slope;
	int counter = 0;
	if  (y1 >= location.bottom && y1 <= location.top) {
		v[0] = location.left;
		v[1] = y1;
		counter++;
	}
	if (y2 >= location.bottom && y2 <= location.top) {
		if (counter > 0) {
			m[0] = location.right;
			m[1] = y2;
		}
		else {
			v[0] = location.right;
			v[1] = y2;
		}
		counter++;
	}
	if (x1 >= location.left && x1 <= location.right) {
		if (counter > 0) {
			m[0] = x1;
			m[1] = location.bottom;
		}
		else {
			v[0] = x1;
			v[1] = location.bottom;
		}
		counter++;
	}
	if (x2 >= location.left && x2 <= location.right) {
		m[0] = x1;
		m[1] = location.bottom;
		counter++;
	}
	return (counter == 2);
}

void runway_instr::drawArrow() {
	Point3D ac(0.0), rwy(0.0);
	ac.setlat(current_aircraft.fdm_state->get_Latitude_deg());
	ac.setlon(current_aircraft.fdm_state->get_Longitude_deg());
	rwy.setlat(runway._lat);
	rwy.setlon(runway._lon);
	float theta = GetHeadingFromTo(ac,rwy);
	theta -= fgGetDouble("/orientation/heading-deg");
	theta = -theta;
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated((location.right+location.left)/2.0,(location.top+location.bottom)/2.0,0.0);	
	glRotated(theta,0.0,0.0,1.0);
	glTranslated(0.0,arrowRad,0.0);
	glScaled(arrowScale,arrowScale,0.0);
	glBegin(GL_TRIANGLES);		
		glVertex2d(-5.0,12.5);
		glVertex2d(0.0,25.0);
		glVertex2d(5.0,12.5);		
	glEnd();
	glBegin(GL_QUADS);
		glVertex2d(-2.5,0.0);
		glVertex2d(-2.5,12.5);
		glVertex2d(2.5,12.5);
		glVertex2d(2.5,0.0);
	glEnd();
	glPopMatrix();
}

void runway_instr::setLineWidth() {
	//Calculate the distance from the runway, A
	double course, distance;
	calc_gc_course_dist(Point3D(runway._lon*SGD_DEGREES_TO_RADIANS, runway._lat*SGD_DEGREES_TO_RADIANS, 0.0),
		Point3D(current_aircraft.fdm_state->get_Longitude(),current_aircraft.fdm_state->get_Latitude(), 0.0 ),
		&course, &distance);
	distance *= SG_METER_TO_NM;
	//Get altitude above runway, B
	double alt_nm = get_agl();
	static const SGPropertyNode *startup_units_node
    = fgGetNode("/sim/startup/units");
	if (!strcmp(startup_units_node->getStringValue(), "feet")) 
		alt_nm *= SG_FEET_TO_METER*SG_METER_TO_NM;
	else
		alt_nm *= SG_METER_TO_NM;
	//Calculate distance away from runway, C = v(A²+B²)	
	distance = sqrt(alt_nm*alt_nm + distance*distance);
	if (distance < scaleDist)
		glLineWidth( 1.0+( (lnScale-1)*( (scaleDist-distance)/scaleDist)));
	else 
		glLineWidth( 1.0 );
	
}

void runway_instr::setArrowRotationRadius(double radius) { arrowRad = radius; }
void runway_instr::setArrowScale(double scale) { arrowScale = scale; }
void runway_instr::setDrawArrow(bool draw) {drawIA = draw;}
void runway_instr::setDrawArrowAlways(bool draw) {drawIAAlways = draw;}
void runway_instr::setLineScale(double scale) {lnScale = scale;}
void runway_instr::setScaleDist(double dist_m) {scaleDist = dist_m;}
void runway_instr::setStippleOutline(unsigned short stipple) {stippleOut = stipple;}
void runway_instr::setStippleCenterline(unsigned short stipple){stippleCen = stipple;}

