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

class FloatSlider : public puSlider
{

        protected:
                float maxValue;
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
                                          float max = 100.0f );

                ~FloatSlider () {;}

                static void adj( puObject *);

                void updateText() {
                        sprintf( _text, "%05.2f", MyValue );
                }

                float get() { return( MyValue ); }
                void  set() { MyValue = ((2.0*maxValue) * (TmpValue - 0.5f)) - maxValue; }

                float *getTmp() { return( &TmpValue ); }
                void   setTmp() { TmpValue += 0.5f; }

                // double the range from -max <-> max
                void init( float f ) {
                        Adjust   = 0.5f / maxValue;
                        setValue((f * Adjust) + 0.5f);
                        adj( this );
                }

                void reinit() { init( origValue ); }

                void cancel() { MyValue = TmpValue; }
                void reset () { init( origValue ); }

};

void FloatSlider::adj( puObject *hs )
{
        FloatSlider *slider = (FloatSlider *)hs->getUserData();
        slider->getValue ( slider->getTmp() );
        slider->setTmp();
        slider->set();
        slider->updateText();
}

FloatSlider::FloatSlider ( int x, int y, int sz, float f, const char *title,
                                                   float max ) :  puSlider( x, y, sz, FALSE )
{
        setUserData( this );
        maxValue = max;
        origValue = f;
        init(f);
        setDelta    ( 0.01 );
        setCBMode   ( PUSLIDER_DELTA ) ;
        setCallback ( adj ) ;
        strcpy      ( _title, title);
        setLabel    ( _title );
        setLabelPlace ( PUPLACE_LEFT  );
        setLegend(_text);
        // setLegendPlace( PUPLACE_RIGHT );
}


/***********************************************/

class sgVec3Slider : public puDialogBox
{
        static void goAway(puObject *p_obj);
        static void reset(puObject *p_obj);
        static void cancel(puObject *p_obj);

        protected:
                char Label[64];
                FloatSlider *HS0;
                FloatSlider *HS1;
                FloatSlider *HS2;
                puOneShot *OkButton;
                puOneShot *ResetButton;
                puOneShot *CancelButton;
                sgVec3 Vec, SaveVec;
        public:

                sgVec3Slider ( int x, int y, sgVec3 vec,
                                           const char *title = "Vector Adjuster",
                                           const char *Xtitle = "X",
                                           const char *Ytitle = "Y",
                                           const char *Ztitle = "Z" );

                ~sgVec3Slider () {;}

                // ???
                void setVec() {
                        Vec[0] = HS2->get();
                        Vec[1] = HS1->get();
                        Vec[2] = HS0->get();
                }

                sgVec3 *getVec() { setVec(); return &Vec; };

                sgVec3 *getStashVec() { return &SaveVec; }
                void stashVec() { sgCopyVec3( SaveVec, Vec ); }

                FloatSlider *getHS0() { return HS0; }
                FloatSlider *getHS1() { return HS1; }
                FloatSlider *getHS2() { return HS2; }

                static void adjust(puObject *p_obj);
};

sgVec3Slider::sgVec3Slider ( int x, int y, sgVec3 vec, const char *title,
                             const char *Xtitle,
                             const char *Ytitle,
                             const char *Ztitle )
    : puDialogBox ( x, y )
{
        int DialogX = x;
        int DialogY = y;
        int fudge = 20;
        static const int DialogWidth = 230 + fudge;

        sgCopyVec3(SaveVec, vec);
        sgCopyVec3(Vec, vec);
        strcpy( Label, title );

        static const int nSliders = 3;
        static const int slider_x = 10+fudge;
        int slider_y = 55;
        static const int slider_width = 210;
        static const int slider_title_x = 15;
        static const int slider_value_x = 160;

        puFont LegendFont, LabelFont;
        puGetDefaultFonts ( &LegendFont, &LabelFont );

        int horiz_slider_height = puGetStringHeight    (&LabelFont) +
                                                          puGetStringDescender (&LabelFont) +
                                                          PUSTR_TGAP + PUSTR_BGAP + 5;

        // HACKS
        setUserData( this );
        horiz_slider_height += 10;

        new puFrame ( 0, 0,
                                  DialogWidth,
                                  85 + nSliders * horiz_slider_height );

        setLabelPlace( PUPLACE_DEFAULT /*PUPLACE_CENTERED*/ );
        setLabel( Label );

        HS2 = new FloatSlider (  slider_x, slider_y, slider_width, vec[2], Ztitle );
        slider_y += horiz_slider_height;

        HS1 = new FloatSlider (  slider_x, slider_y, slider_width, vec[1], Ytitle );
        slider_y += horiz_slider_height;

        HS0 = new FloatSlider (  slider_x, slider_y, slider_width, vec[0], Xtitle );

        OkButton = new puOneShot ( 10+fudge, 10, 60+fudge, 50 );
        OkButton-> setUserData( this );
        OkButton-> setLegend ( gui_msg_OK );
        OkButton-> makeReturnDefault ( TRUE );
        OkButton-> setCallback ( goAway );

        CancelButton = new puOneShot ( 70+fudge, 10, 150+fudge, 50 );
        CancelButton-> setUserData( this );
        CancelButton-> setLegend ( gui_msg_CANCEL );
        CancelButton-> setCallback ( cancel );

        ResetButton = new puOneShot ( 160+fudge, 10, 220+fudge, 50 );
        ResetButton-> setUserData( this );
        ResetButton-> setLegend ( gui_msg_RESET );
        ResetButton-> setCallback ( reset );

        FG_FINALIZE_PUI_DIALOG( this );

}


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
        sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
        sgVec3 vec;
        sgVec3 *pvec = me -> getStashVec();
        sgCopyVec3( vec, *pvec );
        me->HS0->init(vec[0]);
        me->HS1->init(vec[1]);
        me->HS2->init(vec[2]);
        FG_POP_PUI_DIALOG( me );
};

void sgVec3Slider::adjust(puObject *p_obj)
{
        sgVec3Slider *me = (sgVec3Slider *)p_obj->getUserData();
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

void PilotOffsetInit( sgVec3 vec )
{
        // Only one of these things for now
        if( PO_vec == 0 ) {
                sgVec3Slider *PO = new sgVec3Slider ( 200, 200, vec, "Pilot Offset" );
                PO_vec = PO;
        }
}

void PilotOffsetAdjust( puObject * )
{
        if( PO_vec == 0 ) {
                sgVec3 v;
                // we seem to need the ~quite zero terms ???
                sgSetVec3(v,15,0.0001,0.0001);
                PilotOffsetInit(v);
        }
        sgVec3Slider *me = (sgVec3Slider *)PO_vec -> getUserData();
        me -> stashVec();
        me -> adjust( me );
        FG_PUSH_PUI_DIALOG( me );
}

sgVec3 *PilotOffsetGet()
{
        if( PO_vec == 0 ) {
                sgVec3 v;
                // we seem to need the ~quite zero terms ???
                sgSetVec3(v,25,0.0001,0.0001);
                PilotOffsetInit(v);
        }
        sgVec3Slider *me = (sgVec3Slider *)PO_vec -> getUserData();
        return( me -> getVec() );
}
