/*
 * dg2lib.h - header file for the DG2 library libdg2.a
 *
 * copyright 1988-92 VPL Research Inc.
 *
 */



/******** error returns from the library */

extern int DG2_error;  		/* for error information */
extern float DG2_lib_version;  	/* for the library version */
extern int DG2_box_version;  	/* for the firmware version */
extern int DG2_glove_sensors;	/* for the number of sensors in the glove */

/* defines for DG2_error values */

#define DG2_AOK			0 
#define DG2_SETTINGS_FILE	-1
#define DG2_SERIAL_OPEN		-2 
#define DG2_SERIAL_PORT		-4   
#define DG2_RESET		-6  
#define DG2_PARAMETER		-7  
#define DG2_FILE_IO		-8
#define DG2_CALIBRATION_FILE	-9
#define DG2_GESTURE_FILE	-10
#define DG2_CAL_GEST_FILES	-11
/* defines for DG2_response() */

#define DATAGLOVE	1
#define POLHEMUS	2
#define GESTURE		8

#define DG2_60Hz	1
#define DG2_30Hz	2
#define DG2_oneShot	3

/* defines for DG2_DataGlove_select() */

#define THUMB_INNER	0x1
#define THUMB_OUTER	0x2
#define INDEX_INNER	0x4
#define INDEX_OUTER	0x8
#define MIDDLE_INNER	0x10
#define MIDDLE_OUTER	0x20
#define RING_INNER	0x40
#define RING_OUTER	0x80
#define LITTLE_INNER	0x100
#define LITTLE_OUTER	0x200
#define NORMAL_JOINTS	0x3ff
#define FLEX11		0x400
#define FLEX12		0x800
#define FLEX13		0x1000
#define FLEX14		0x2000
#define FLEX15		0x4000
#define FLEX16		0x8000


/* defines for DG2_DataGlove_trans_select() */

#define DG2_TRANSLATED 	5
#define DG2_RAW		6

/* defines for DG2_Polhemus_units() */

#define POL_RAW		0
#define POL_INCHES	1
#define POL_CM		2

/* defines for DG2_user_IRQ() */

#define IRQ_ON	1
#define IRQ_OFF	2


/* defines for DG2_get_data() */

#define DG2_report	1
#define DG2_userport	2


/* dg2 command codes*/
#define LEADINGBYTE 	0x24
#define RPT60     	0x41	/* repeat 60 */
#define RPT30     	0x42	/* repeat 30 */
#define ONESHOT   	0x43	/* one shot */
#define SYSID     	0x44	/* system ID */
#define EPTBUF    	0x45 	/* empty buffer */
#define USRRD     	0x46	/* user read */
#define USRIRQ    	0x47	/* user IRQ */
#define QBRT      	0x48	/* query bright */	
#define CDRST     	0x49	/* cold reset */
#define WMRST     	0x4A	/* warm reset */
#define MEMALLO   	0x4B	/* memory alloc */
#define DLTSND    	0x4C	/* delta send */
#define SETBRT    	0x4D	/* set bright */
#define SETDIM    	0x4E	/* set dim */
#define FILBUF    	0x4F	/* fill buffer */
#define LDTBL     	0x50	/* load table */
#define LDPOL     	0x51	/* send up to 63 bytes to Polhemus  */
#define ANGLE     	0x52	/* angles */
#define NSNSR     	0x53	/* num sensors */
#define SETFB     	0x54	/* set feedback */
#define QCUT      	0X55	/* query cutoff*/
#define SETCUT    	0X56	/* set cutoff */
#define FLXVAL    	0X57	/* raw flex values */
#define USRWR     	0X58	/* user write */
#define JNTMAP    	0X59	/* joint map */
#define ERRMESS		0XFF	/* error in command input */
#define TIMOUT		0XFE	/* timed out during command */

/* response structure */

typedef struct DG2_data {
	char gesture;
	double location[3];  	/* X,Y,Z */
	double orientation[3];	/* yaw, pitch, roll */
	short flex[16];
	char gesture_name[20];
	short reserved[16];
		/* user port data: */
	char user_nibble;
	char user_analog[3];
} DG2_data;


/**************function prototypes*************/
/*NOTE: all DG2_ functions return -1 on error*/

extern int DG2_open(char *portname, int baud);
extern int DG2_close(int filedes);
extern int DG2_direct(int filedes,char *message,int count);
extern int DG2_response(int filedes,int devices,int rate);
extern int DG2_DataGlove_select(int filedes,int flex_sensors);
extern int DG2_DataGlove_translation(int filedes,int flex_sensors,char table[16][256]);
extern int DG2_DataGlove_trans_select(int filedes,int status);
extern int DG2_DataGlove_LED_set(int filedes,int LED);
extern int DG2_DataGlove_LED_read(int filedes);
extern int DG2_Polhemus_units(int filedes,char type);
extern int DG2_Polhemus_direct(int filedes,char *message,int count);
extern int DG2_user_write(int filedes,int nibble);
extern int DG2_user_IRQ(int filedes,int mode);
extern int DG2_user_read(int filedes,DG2_data *data);
extern int DG2_get_data(int filedes,DG2_data *data);
extern int DG2_gesture_load(int filedes,char *calib,char *gest);

/*use this with caution since it does not return until it gets a correct
 *response from the DG2
*/
extern int DG2U_get_reply(int filedes,char *buff,int response,int size);
