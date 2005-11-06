/**
 *      sgVec3Slider.cxx
 *
 *      Simple PUI wrapper around sgVec3 class
 *
 *      Created by: Norman Vine [NHV]
 *         nhv@yahoo.com, nhv@cape.com
 *
 *      Revision History
 *      ---------------- 
 *  Started 12:53 01/28/2001
 *
 */

#include "sgVec3Slider.hxx"
#include <Main/fg_props.hxx>
#include <simgear/sg_inlines.h>

static void
setPilotXOffset (float value)
{
	PilotOffsetSet(0, value);
}

static float
getPilotXOffset ()
{
	return( PilotOffsetGetSetting(0) );
}


static void
setPilotYOffset (float value)
{
	PilotOffsetSet(1, value);
}

static float
getPilotYOffset ()
{
	return( PilotOffsetGetSetting(1) );
}


static void
setPilotZOffset (float value)
{
	PilotOffsetSet(2, value);
}

static float
getPilotZOffset ()
{
	return( PilotOffsetGetSetting(2) );
}


class FloatSlider : public puSlider
{

protected:
	float maxValue;
        float minValue;
	float origValue;
	float Adjust;
	float MyValue;
	float TmpValue;
	float SaveValue;
	char buf[8];
	char _title[32];
	char _text[16];
public:
	FloatSlider ( int x, int y, int sz, float f, const char *title,
				  float max = 100.0f, float min = 0.0f);

	~FloatSlider () {;}

	static void adj( puObject *);
		
	void updateText() {
		sprintf( _text, "%05.2f", MyValue );
	}

	float get() { return( MyValue ); }

        // calc actual "setting" by multiplying the pui fraction by highest value setable
        void  set() { MyValue = maxValue * TmpValue; }

	float *getTmp() { return( &TmpValue ); }

        // adjust the TmpValue back to 0 to 1 range of pui widget
        void   setTmp() { TmpValue += 0.0f; }

        void incTmp() { TmpValue += 0.0001f; setTmp(); }
        void decTmp() { TmpValue -= 0.0001f; setTmp(); }

	// adjust actual "setting" value back to fraction for pui widget
	void init( float f ) {
                if (f > maxValue) f = maxValue;
                if (f < minValue) f = minValue;
		Adjust   = 1.0f / maxValue;
                setValue(f * Adjust);
		adj( this );
	}

	void reinit() { init( origValue ); }
	void cancel() { MyValue = TmpValue; }
	void reset () { init( origValue ); }

};

void FloatSlider::adj( puObject *hs )
{
	FloatSlider *slider = (FloatSlider *)hs;
	slider->getValue ( slider->getTmp() );
	slider->setTmp();
	slider->set();
	slider->updateText();
}

FloatSlider::FloatSlider ( int x, int y, int sz, float f, const char *title,
                                                   float max, float min ) :  puSlider( x, y, sz, FALSE )
{
	maxValue = max;
        minValue = min;
	origValue = f;
	init(f);
	setDelta    ( 0.01 );
	setCBMode   ( PUSLIDER_DELTA ) ;
	strcpy      ( _title, title);
	setLabel    ( _title );
	setLabelPlace ( PUPLACE_LEFT );
	setLegend(_text);
	// setLegendPlace( PUPLACE_RIGHT );
}


class FloatDial : public puDial
{

protected:
	float maxValue;
        float minValue;
	float origValue;
	float Adjust;
	float MyValue;
	float TmpValue;
	float SaveValue;
	char buf[8];
	char _title[32];
	char _text[16];
public:
	FloatDial ( int x, int y, int sz, float f, const char *title,
				  float max = 180.0f, float min = -180.0f );

	~FloatDial () {;}
		
	static void adj( puObject *);

	void updateText() {
		sprintf( _text, "%05.2f", MyValue );
	}

	float get() { return( MyValue ); }

        // calc actual "setting" by multiplying the pui fraction by highest value setable
        // in this case we're also turning a 0 to 1 range into a -.05 to +.05 before converting
	void  set() { MyValue = ((2.0*maxValue) * (TmpValue - 0.5f)) - maxValue; }

	float *getTmp() { return( &TmpValue ); }

        // adjust the TmpValue back to 0 to 1 range of pui widget
	void   setTmp() { TmpValue += 0.5f; }
        
        void incTmp() { TmpValue += 0.0001f; setTmp(); }
        void decTmp() { TmpValue -= 0.0001f; setTmp(); }

	// adjust actual "setting" value back to fraction for pui widget
	// double the range from -max <-> max
	void init( float f ) {
                if (f > maxValue) f = maxValue;
                if (f < minValue) f = minValue;
		Adjust   = 0.5f / maxValue;
		setValue((f * Adjust) + 0.5f);
		adj( this );
	}

	void reinit() { init( origValue ); }
	void cancel() { MyValue = TmpValue; }
	void reset () { init( origValue ); }

};

void FloatDial::adj( puObject *hs )
{
	FloatDial *dial = (FloatDial *)hs;
	dial->getValue ( dial->getTmp() );
	dial->setTmp();
	dial->set();
	dial->updateText();
}

FloatDial::FloatDial ( int x, int y, int sz, float f, const char *title,
                                                   float max, float min ) :  puDial( x, y, sz )
{
	maxValue = max;
        minValue = min;
	origValue = f;
	init(f);
	setDelta    ( 0.01 );
	setCBMode   ( PUSLIDER_DELTA ) ;
	strcpy      ( _title, title);
	setLabel    ( _title );
	setLabelPlace ( PUPLACE_LEFT  );
	setLegend(_text);
	// setLegendPlace( PUPLACE_RIGHT );
}


/***********************************************/

class sgVec3Slider : public puDialogBox
{
        static void handle_arrow(puObject *p_obj);
	static void heading_adj(puObject *p_obj);
	static void pitch_adj(puObject *p_obj);
	static void radius_adj(puObject *p_obj);
	static void goAway(puObject *p_obj);
	static void reset(puObject *p_obj);
	static void cancel(puObject *p_obj);

protected:
	char Label[64];
	FloatDial *HS0;
	FloatDial *HS1;
	FloatSlider *HS2;
        puText *TitleText;
	puOneShot *OkButton;
	puOneShot *ResetButton;
	puOneShot *CancelButton;
        puArrowButton *right_arrow;
        puArrowButton *left_arrow;
	sgVec3 Vec, SaveVec;
	
public:

	sgVec3Slider ( int x, int y, sgVec3 vec,
				   const char *title = "Vector Adjuster",
				   const char *Xtitle = "Heading",
				   const char *Ytitle = "Pitch",
				   const char *Ztitle = "Radius   " );

	~sgVec3Slider () {;}


	// calc the property Vec with the actual point on sphere
	void setVec()
	{
		Vec3FromHeadingPitchRadius( Vec,
						HS0->get(),
						HS1->get(),
						HS2->get());
	}

        // return the offset vector according to current widget values
	sgVec3 *getVec()
	{ 
                setVec();
		return &Vec;
	};

        // save the Vector before pushing dialog (for cancel button)
	sgVec3 *getStashVec() { return &SaveVec; }
	void stashVec() { 
		SaveVec[2] = HS0->get();
		SaveVec[1] = HS1->get();
		SaveVec[0] = HS2->get();
	}

        // functions to return pointers to protected pui widget instances
	FloatDial *getHS0() { return HS0; }
	FloatDial *getHS1() { return HS1; }
	FloatSlider *getHS2() { return HS2; }

	static void adjust(puObject *p_obj);
};

// class constructor
sgVec3Slider::sgVec3Slider ( int x, int y, sgVec3 cart, const char *title,
                             const char *Xtitle,
                             const char *Ytitle,
                             const char *Ztitle ): puDialogBox ( x, y )
{
	puFont LegendFont, LabelFont;
	puGetDefaultFonts ( &LegendFont, &LabelFont );
	int fudge = 20;

        HeadingPitchRadiusFromVec3( Vec, cart);
        sgVec3 vec;
        sgSetVec3(vec, Vec[0], Vec[1], Vec[2]);

	int labelW = LabelFont.getStringWidth(Xtitle);
	labelW = SG_MAX2( labelW, LabelFont.getStringWidth(Ytitle));
	labelW = SG_MAX2( labelW, LabelFont.getStringWidth(Ztitle));
		

	sgCopyVec3(SaveVec, vec);
	sgCopyVec3(Vec, vec);
	strcpy( Label, title );

	int nSliders = 3;
	int slider_x = 70+fudge;
	int slider_y = 75;
	int slider_width = 270;
	int dialer_x = 70+fudge;
	int dialer_y = 115;
        int dialer_size = 100;

	int horiz_slider_height = LabelFont.getStringHeight()
            + LabelFont.getStringDescender()
            + PUSTR_TGAP + PUSTR_BGAP + 5;

	int DialogWidth = slider_width + 20 + fudge + labelW;
	int DialogHeight = dialer_size + 115 + nSliders * horiz_slider_height;
	
	// HACKS
	setUserData( this );
	horiz_slider_height += 10;

	new puFrame ( 0, 0, DialogWidth, DialogHeight );

	setLabelPlace( PUPLACE_DEFAULT /*PUPLACE_CENTERED*/ );
	setLabel( Label );

        /* heading */
	HS0 = new FloatDial (  dialer_x, dialer_y, dialer_size, vec[0], Xtitle, 180.0f, -180.0f );
        dialer_x = dialer_x + 170;
        HS0->setUserData (this);
	HS0->setCallback ( heading_adj ) ;

        /* pitch */
	HS1 = new FloatDial (  dialer_x, dialer_y, dialer_size, vec[1], Ytitle, 89.99f, -89.99f );
        HS1->setUserData (this);
	HS1->setCallback ( pitch_adj ) ;

        /* radius */
	HS2 = new FloatSlider (  slider_x+20, slider_y, slider_width-40, vec[2], Ztitle, 100.0f, 0.0f );
        HS2->setUserData (this);
	HS2->setCallback ( radius_adj );

        right_arrow = new puArrowButton ( slider_x + slider_width - 20, slider_y + 1, slider_x + slider_width, slider_y+21, PUARROW_RIGHT ) ;
        right_arrow->setUserData ( HS2 ) ;
        right_arrow->setCallback ( handle_arrow ) ;

        left_arrow = new puArrowButton ( slider_x, slider_y + 1, slider_x+20, slider_y+21, PUARROW_LEFT ) ;
        left_arrow->setUserData ( HS2 ) ;
        left_arrow->setCallback ( handle_arrow ) ;

	TitleText = new puText( 130, DialogHeight - 40);
        TitleText->setLabel("Pilot Offset");

	OkButton = new puOneShot ( 70+fudge, 20, 120+fudge, 50 );
	OkButton-> setUserData( this );
	OkButton-> setLegend ( gui_msg_OK );
	OkButton-> makeReturnDefault ( TRUE );
	OkButton-> setCallback ( goAway );

	CancelButton = new puOneShot ( 130+fudge, 20, 210+fudge, 50 );
	CancelButton-> setUserData( this );
	CancelButton-> setLegend ( gui_msg_CANCEL );
	CancelButton-> setCallback ( cancel );

// renabling this button requires binding in the sim/view[?]/config/?-offset-m values
//	ResetButton = new puOneShot ( 220+fudge, 20, 280+fudge, 50 );
//	ResetButton-> setUserData( this );
//	ResetButton-> setLegend ( gui_msg_RESET );
//	ResetButton-> setCallback ( reset );

	FG_FINALIZE_PUI_DIALOG( this );
}

void sgVec3Slider::handle_arrow(puObject *arrow)
{
  FloatSlider *slider = (FloatSlider *) arrow->getUserData();
  if (((puArrowButton *)arrow)->getArrowType() == PUARROW_RIGHT)
    slider->incTmp();
  else
    slider->decTmp();
  slider->set();
  slider->init( slider->get() );
  sgVec3Slider *me = (sgVec3Slider *)slider->getUserData();
  me->setVec();
};

void sgVec3Slider::heading_adj(puObject *p_obj)
{
  sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
  me->HS0->adj((puObject *)me ->HS0 );
  me->setVec();
};

void sgVec3Slider::pitch_adj(puObject *p_obj)
{
  sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
  me->HS1->adj((puObject *)me ->HS1 );
  me->setVec();
};

void sgVec3Slider::radius_adj(puObject *p_obj)
{
  sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
  me ->HS2-> adj((puObject *)me ->HS2 );
  me->setVec();
};

void sgVec3Slider::goAway(puObject *p_obj)
{
	sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
	FG_POP_PUI_DIALOG( me );
};

void sgVec3Slider::reset(puObject *p_obj)
{
	sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
	me->HS0->reinit();
	me->HS1->reinit();
	me->HS2->reinit();
};

void sgVec3Slider::cancel(puObject *p_obj)
{
	sgVec3 vec;
	sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
	sgVec3 *pvec = me -> getStashVec();
	sgCopyVec3( vec, *pvec );
	me->HS0->init(vec[2]);
	me->HS1->init(vec[1]);
	me->HS2->init(vec[0]);
	me->setVec();
	FG_POP_PUI_DIALOG( me );
};

void sgVec3Slider::adjust(puObject *p_obj)
{
	sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
        heading_adj( me );
        pitch_adj( me );
        radius_adj( me );

	sgVec3 cart;
        sgSetVec3(cart,
               fgGetFloat("/sim/current-view/x-offset-m"),
               fgGetFloat("/sim/current-view/y-offset-m"),
               fgGetFloat("/sim/current-view/z-offset-m")
        );

        HeadingPitchRadiusFromVec3( me->Vec, cart);

        me -> getHS0() -> init(me->Vec[0]);
        me -> getHS1() -> init(me->Vec[1]);
        me -> getHS2() -> init(me->Vec[2]);

	me -> getHS0() -> adj((puObject *)me -> getHS0());
	me -> getHS1() -> adj((puObject *)me -> getHS1());
	me -> getHS2() -> adj((puObject *)me -> getHS2());
	me -> setVec();
};

void sgVec3SliderAdjust( puObject *p_obj )
{
	sgVec3Slider *me = (sgVec3Slider *)p_obj -> getUserData();
	me -> adjust( me );
	FG_PUSH_PUI_DIALOG( me );
}

// These are globals for now
static puObject *PO_vec = 0;

void PilotOffsetInit() {
	sgVec3 cart;
        sgSetVec3(cart,
               fgGetFloat("/sim/current-view/x-offset-m"),
               fgGetFloat("/sim/current-view/y-offset-m"),
               fgGetFloat("/sim/current-view/z-offset-m")
        );
	PilotOffsetInit(cart);
}

void PilotOffsetInit( sgVec3 cart )
{
	// Only one of these things for now
	if( PO_vec == 0 ) {
 		sgVec3Slider *PO = new sgVec3Slider ( 200, 200, cart, "Pilot Offset" );
		PO_vec = PO;
		// Bindings for Pilot Offset
                fgTie("/sim/current-view/x-offset-m", getPilotXOffset, setPilotXOffset);
                fgTie("/sim/current-view/y-offset-m", getPilotYOffset, setPilotYOffset);
                fgTie("/sim/current-view/z-offset-m", getPilotZOffset, setPilotZOffset);
	}
}

void PilotOffsetAdjust( puObject * )
{
	if( PO_vec == 0 ) {
		PilotOffsetInit();
	}
	sgVec3Slider *me = (sgVec3Slider *)PO_vec -> getUserData();
	me -> adjust( me );
	me -> stashVec();
	FG_PUSH_PUI_DIALOG( me );
}

// external to get pilot offset vector for viewer
sgVec3 *PilotOffsetGet()
{
	if( PO_vec == 0 ) {
		PilotOffsetInit();
	}
	sgVec3Slider *me = (sgVec3Slider *)PO_vec -> getUserData();
	return( me -> getVec() );
}

// external function used to tie to FG properties
float PilotOffsetGetSetting(int opt)
{
	float setting = 0.0;
	if( PO_vec == 0 ) {
            PilotOffsetInit();
	}
	sgVec3Slider *me = (sgVec3Slider *)PO_vec -> getUserData();
        sgVec3 vec;
        sgCopyVec3(vec, (float *) me->getVec());
        if( opt == 0 ) setting = vec[0];
        if( opt == 1 ) setting = vec[1];
        if( opt == 2 ) setting = vec[2];
	return( setting );
}

// external function used to tie to FG properties
void PilotOffsetSet(int opt, float setting)
{
	if( PO_vec == 0 ) {
		PilotOffsetInit();
	}
	sgVec3Slider *me = (sgVec3Slider *)PO_vec -> getUserData();

        sgVec3 HPR;
        sgVec3 vec;
        sgCopyVec3(vec, (float *) me->getVec());
        if( opt == 0 ) vec[0] = setting;
        if( opt == 1 ) vec[1] = setting;
        if( opt == 2 ) vec[2] = setting;
        HeadingPitchRadiusFromVec3 ( HPR, vec );
        me -> getHS0() -> init( HPR[0] );
        me -> getHS1() -> init( HPR[1] );
        me -> getHS2() -> init( HPR[2] );
        me -> stashVec();
}


// Calculate vector of point on sphere:
// input Heading == longitude of point on sphere
// input Pitch   == latitude of point on sphere
// input Radius  == radius of sphere

#define MIN_VIEW_OFFSET 0.001
void Vec3FromHeadingPitchRadius ( sgVec3 vec3, float heading, float pitch,
float radius )
{
  double ch, sh, cp, sp;

  if ( heading == SG_ZERO )
  {
    ch = SGD_ONE ;
    sh = SGD_ZERO ;
  }
  else
  {
    sh = sin( (double)( heading * SG_DEGREES_TO_RADIANS )) ;
    ch = cos( (double)( heading * SG_DEGREES_TO_RADIANS )) ;
  }

  if ( pitch == SG_ZERO )
  {
    cp = SGD_ONE ;
    sp = SGD_ZERO ;
  }
  else
  {
    sp = sin( (double)( pitch * SG_DEGREES_TO_RADIANS )) ;
    cp = cos( (double)( pitch * SG_DEGREES_TO_RADIANS )) ;
  }

  if ( radius < MIN_VIEW_OFFSET )
	  radius = MIN_VIEW_OFFSET ;

  vec3[2] = (SGfloat)( ch * cp ) * radius ; // X
  vec3[1] = (SGfloat)( sh * cp ) * radius ; // Y
  vec3[0] = (SGfloat)(  sp ) * radius ; // Z
}

// Convert to speherical coordinates for the dials

void HeadingPitchRadiusFromVec3 ( sgVec3 hpr, sgVec3 vec3 )
{
  double x = vec3[0];
  double y = vec3[1];
  double z = vec3[2];
  double Zx;
  double Pr;

  if (z == 0.0f) {
    hpr[0] = 0;
    hpr[1] = 0;
    hpr[2] = 0;
  } else {
    if (fabs(y) < 0.001f)
      y = 0.001f;

    Zx = sqrt(y*y + z*z);

    hpr[2] = sqrt(x*x + y*y + z*z);

    Pr = Zx / hpr[2];
    if (Pr > 1.0f)
      hpr[1] = 0.0f;
    else 
      hpr[1] = acos(Pr) * SGD_RADIANS_TO_DEGREES;
    if (x < 0.0f) 
      hpr[1] *= -SGD_ONE;

    if (z < 0.0f) {
      hpr[0] = (SGD_PI - asin(y/Zx)) * SGD_RADIANS_TO_DEGREES;
    } else {
      hpr[0] = asin(y/Zx) * SGD_RADIANS_TO_DEGREES;
      if (hpr[0] > 180) {
        hpr[0] = 180 - hpr[0];
      }
    }
  }
}
