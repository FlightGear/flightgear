// Scenery management routines

/* static void fgSceneryInit_OLD() { */
    /* make scenery */
/*     scenery = fgSceneryCompile_OLD();
    runway = fgRunwayHack_OLD(0.69, 53.07);
} */


/* create the scenery */
/* GLint fgSceneryCompile_OLD() {
    GLint scenery;

    scenery = mesh2GL(mesh_ptr_OLD);

    return(scenery);
}
*/

/* hack in a runway */
/* GLint fgRunwayHack_OLD(double width, double length) {
    static GLfloat concrete[4] = { 0.5, 0.5, 0.5, 1.0 };
    static GLfloat line[4]     = { 0.9, 0.9, 0.9, 1.0 };
    int i;
    int num_lines = 16;
    float line_len, line_width_2, cur_pos;

    runway = xglGenLists(1);
    xglNewList(runway, GL_COMPILE);
    */
    /* draw concrete */
/*    xglBegin(GL_POLYGON);
    xglMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, concrete );
    xglNormal3f(0.0, 0.0, 1.0);

    xglVertex3d( 0.0,   -width/2.0, 0.0);
    xglVertex3d( 0.0,    width/2.0, 0.0);
    xglVertex3d(length,  width/2.0, 0.0);
    xglVertex3d(length, -width/2.0, 0.0);
    xglEnd();
    */
    /* draw center line */
/*    xglMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, line );
    line_len = length / ( 2 * num_lines + 1);
    printf("line_len = %.3f\n", line_len);
    line_width_2 = 0.02;
    cur_pos = line_len;
    for ( i = 0; i < num_lines; i++ ) {
	xglBegin(GL_POLYGON);
	xglVertex3d( cur_pos, -line_width_2, 0.005);
	xglVertex3d( cur_pos,  line_width_2, 0.005);
	cur_pos += line_len;
	xglVertex3d( cur_pos,  line_width_2, 0.005);
	xglVertex3d( cur_pos, -line_width_2, 0.005);
	cur_pos += line_len;
	xglEnd();
    }

    xglEndList();

    return(runway);
}
*/

/* draw the scenery */
/*static void fgSceneryDraw_OLD() {
    static float z = 32.35;

    xglPushMatrix();

    xglCallList(scenery);

    printf("*** Drawing runway at %.2f\n", z);

    xglTranslatef( -398391.28, 120070.41, 32.35);
    xglRotatef(170.0, 0.0, 0.0, 1.0);
    xglCallList(runway);

    xglPopMatrix();
}
*/


