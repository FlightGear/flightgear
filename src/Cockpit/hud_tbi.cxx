//
//  Turn Bank Indicator
//

#include <math.h>
#include "hud.hxx"

// FIXME
extern float get_roll(void);
extern float get_sideslip(void);

// x, y, width, height, get_roll, get_sideslip, maxBankAngle, maxSlipAngle, gap_width, working, tsi, rad

// int x, int y, UINT width, UINT height, FLTFNPTR chn1_source, FLTFNPTR chn2_source, float maxBankAngle,
// float maxSlipAngle, UINT gap_width, bool working, bool tsivalue, float radius) :


fgTBI_instr::fgTBI_instr(const SGPropertyNode *node) :
    dual_instr_item(
            node->getIntValue("x"),
            node->getIntValue("y"),
            node->getIntValue("width"),
            node->getIntValue("height"),
            get_roll,					// FIXME
            get_sideslip,
            node->getBoolValue("working", true),
            HUDS_TOP),
    BankLimit(int(node->getFloatValue("maxBankAngle"))),
    SlewLimit(int(node->getFloatValue("maxSlipAngle"))),
    scr_hole(node->getIntValue("gap_width")),
    tsi(node->getBoolValue("tsi")),
    rad(node->getFloatValue("rad"))

{
    SG_LOG(SG_INPUT, SG_BULK, "Done reading TBI instrument"
            << node->getStringValue("name", "[unnamed]"));
}


void fgTBI_instr::draw(void)
{
    float bank_angle, sideslip_angle;
    float ss_const; // sideslip angle pixels per rad
    float cen_x, cen_y, bank, fspan, tee, hole;

    int span = get_span();
    float zero = 0.0;

    RECT My_box = get_location();
    POINT centroid = get_centroid();
    int tee_height = My_box.bottom;

    bank_angle = current_ch2();  // Roll limit +/- 30 degrees

    if (bank_angle < -SGD_PI_2 / 3)
        bank_angle = -SGD_PI_2 / 3;
    else if (bank_angle > SGD_PI_2 / 3)
        bank_angle = SGD_PI_2 / 3;


    sideslip_angle = current_ch1(); // Sideslip limit +/- 20 degrees

    if (sideslip_angle < -SGD_PI / 9)
        sideslip_angle = -SGD_PI / 9;
    else if ( sideslip_angle > SGD_PI / 9 )
        sideslip_angle = SGD_PI / 9;

    cen_x = centroid.x;
    cen_y = centroid.y;

    bank = bank_angle * SGD_RADIANS_TO_DEGREES;
    tee = -tee_height;
    fspan = span;
    hole = scr_hole;
    ss_const = 2 * sideslip_angle * fspan / (SGD_2PI / 9);  // width represents 40 degrees

//  printf("side_slip: %f   fspan: %f\n", sideslip_angle, fspan);
//  printf("ss_const: %f   hole: %f\n", ss_const, hole);

    glPushMatrix();
    glTranslatef(cen_x, cen_y, zero);
    glRotatef(-bank, zero, zero, 1.0);

    if (!tsi) {
       glBegin(GL_LINES);

       if (!scr_hole) {
           glVertex2f(-fspan, zero);
           glVertex2f(fspan, zero);

       } else {
           glVertex2f(-fspan, zero);
           glVertex2f(-hole, zero);
           glVertex2f(hole, zero);
           glVertex2f(fspan, zero);
       }

       // draw teemarks
       glVertex2f(hole, zero);
       glVertex2f(hole, tee);
       glVertex2f(-hole, zero);
       glVertex2f(-hole, tee);
       glEnd();

       glBegin(GL_LINE_LOOP);
       glVertex2f(ss_const, -hole);
       glVertex2f(ss_const + hole, zero);
       glVertex2f(ss_const, hole);
       glVertex2f(ss_const - hole, zero);
       glEnd();

    } else { //if tsi enabled
        drawOneLine(cen_x-1.0, My_box.top, cen_x + 1.0, My_box.top);
        drawOneLine(cen_x-1.0, My_box.top, cen_x - 1.0, My_box.top + 10.0);
        drawOneLine(cen_x+1.0, My_box.top, cen_x + 1.0, My_box.top + 10.0);
        drawOneLine(cen_x-1.0, My_box.top + 10.0, cen_x + 1.0, My_box.top + 10.0);

        float x1, y1, x2, y2, x3, y3, x4, y4, x5, y5;
        float xc, yc, r = rad, r1 = rad - 10.0, r2 = rad - 5.0;

        xc = My_box.left + My_box.right / 2.0 ;
        yc = My_box.top + r;

        // first n last lines
        x1 = xc + r * cos(255.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(255.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r1 * cos(255.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r1 * sin(255.0 * SGD_DEGREES_TO_RADIANS);
        drawOneLine(x1,y1,x2,y2);

        x1 = xc + r * cos(285.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(285.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r1 * cos(285.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r1 * sin(285.0 * SGD_DEGREES_TO_RADIANS);
        drawOneLine(x1, y1, x2, y2);

        // second n fifth  lines
        x1 = xc + r * cos(260.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(260.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r2 * cos(260.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r2 * sin(260.0 * SGD_DEGREES_TO_RADIANS);
        drawOneLine(x1, y1, x2, y2);

        x1 = xc + r * cos(280.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(280.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r2 * cos(280.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r2 * sin(280.0 * SGD_DEGREES_TO_RADIANS);
        drawOneLine(x1, y1, x2, y2);

        // third n fourth lines
        x1 = xc + r * cos(265.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(265.0 * SGD_DEGREES_TO_RADIANS);


        x2 = xc + r2 * cos(265.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r2 * sin(265.0 * SGD_DEGREES_TO_RADIANS);
        drawOneLine(x1, y1, x2, y2);

        x1 = xc + r * cos(275.0 * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin(275.0 * SGD_DEGREES_TO_RADIANS);

        x2 = xc + r2 * cos(275.0 * SGD_DEGREES_TO_RADIANS);
        y2 = yc + r2 * sin(275.0 * SGD_DEGREES_TO_RADIANS);
        drawOneLine(x1,y1,x2,y2);

        // draw marker
        float  valbank, valsideslip, sideslip;

        r = rad + 5.0;  // add gap

        // upper polygon
        bank_angle = current_ch2();

        bank = bank_angle * SGD_RADIANS_TO_DEGREES;  // Roll limit +/- 30 degrees
        if (bank > BankLimit)
            bank = BankLimit;
        if (bank < -1.0*BankLimit)
            bank = -1.0*BankLimit;

        valbank = bank * 15.0 / BankLimit; // total span of TSI is 30 degrees

        sideslip_angle = current_ch1(); // Sideslip limit +/- 20 degrees
        sideslip = sideslip_angle * SGD_RADIANS_TO_DEGREES;
        if (sideslip > SlewLimit)
            sideslip = SlewLimit;
        if (sideslip < -1.0 * SlewLimit)
            sideslip = -1.0 * SlewLimit;
        valsideslip = sideslip * 15.0 / SlewLimit;

        // values 270, 225 and 315 are angles in degrees...
        x1 = xc + r * cos((valbank + 270.0) * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin((valbank + 270.0) * SGD_DEGREES_TO_RADIANS);

        x2 = x1 + 6.0 * cos(225 * SGD_DEGREES_TO_RADIANS);
        y2 = y1 + 6.0 * sin(225 * SGD_DEGREES_TO_RADIANS);

        x3 = x1 + 6.0 * cos(315 * SGD_DEGREES_TO_RADIANS);
        y3 = y1 + 6.0 * sin(315 * SGD_DEGREES_TO_RADIANS);

        drawOneLine(x1, y1, x2, y2);
        drawOneLine(x2, y2, x3, y3);
        drawOneLine(x3, y3, x1, y1);

        // lower polygon
        x1 = xc + r * cos((valbank + 270.0) * SGD_DEGREES_TO_RADIANS);
        y1 = yc + r * sin((valbank + 270.0) * SGD_DEGREES_TO_RADIANS);

        x2 = x1 + 6.0 * cos(225 * SGD_DEGREES_TO_RADIANS);
        y2 = y1 + 6.0 * sin(225 * SGD_DEGREES_TO_RADIANS);

        x3 = x1 + 6.0 * cos(315 * SGD_DEGREES_TO_RADIANS);
        y3 = y1 + 6.0 * sin(315 * SGD_DEGREES_TO_RADIANS);

        x4 = x1 + 10.0 * cos(225 * SGD_DEGREES_TO_RADIANS);
        y4 = y1 + 10.0 * sin(225 * SGD_DEGREES_TO_RADIANS);

        x5 = x1 + 10.0 * cos(315 * SGD_DEGREES_TO_RADIANS);
        y5 = y1 + 10.0 * sin(315 * SGD_DEGREES_TO_RADIANS);

        x2 = x2 + cos(valsideslip * SGD_DEGREES_TO_RADIANS);
        y2 = y2 + sin(valsideslip * SGD_DEGREES_TO_RADIANS);
        x3 = x3 + cos(valsideslip * SGD_DEGREES_TO_RADIANS);
        y3 = y3 + sin(valsideslip * SGD_DEGREES_TO_RADIANS);
        x4 = x4 + cos(valsideslip * SGD_DEGREES_TO_RADIANS);
        y4 = y4 + sin(valsideslip * SGD_DEGREES_TO_RADIANS);
        x5 = x5 + cos(valsideslip * SGD_DEGREES_TO_RADIANS);
        y5 = y5 + sin(valsideslip * SGD_DEGREES_TO_RADIANS);

        drawOneLine(x2, y2, x3, y3);
        drawOneLine(x3, y3, x5, y5);
        drawOneLine(x5, y5, x4, y4);
        drawOneLine(x4, y4, x2, y2);
    }
    glPopMatrix();
}


