/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003-2006, Horizon Wimba, Inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 * Frik Strecker <frik@gatherworks.com>
 * Mihai Balea <mihai AT hates DOT ms>
 * Peter Grayson <jpgrayson@gmail.com>
 * Bill Cholewka <bcholew@gmail.com>
 * Erik Bunce <kde@bunce.us>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */
#ifndef _iaxclient_h
#define _iaxclient_h

#ifdef __cplusplus
extern "C" {
#endif

/*!
  \file iaxclient.h
  \brief The IAXClient API
	
	

  \note This is the include file which declares all external API functions to
  IAXClient.  It should include all functions and declarations needed
  by IAXClient library users, but not include internal structures, or
  require the inclusion of library internals (or sub-libraries) 
*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _MSC_VER
typedef int socklen_t;
#endif
#include <stdio.h>
#if defined(WIN32)  ||  defined(_WIN32_WCE)
#include <windows.h>
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#ifdef BUILDING_DLL
# if defined(WIN32) ||  defined(_WIN32_WCE)
#  ifdef _MSC_VER
#   define EXPORT __declspec(dllexport)
#  else
#   define EXPORT  __stdcall __declspec(dllexport)
#  endif
# else
#  define EXPORT
#endif
#else
# define EXPORT
#endif
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#if defined(WIN32)  ||  defined(_WIN32_WCE)
#if defined(_MSC_VER)
	typedef int (__stdcall *iaxc_sendto_t)(SOCKET, const void *, size_t, int,
			const struct sockaddr *, socklen_t);
	typedef int (__stdcall *iaxc_recvfrom_t)(SOCKET, void *, size_t, int,
			struct sockaddr *, socklen_t *);
#else
	typedef int PASCAL (*iaxc_sendto_t)(SOCKET, const char *, int, int,
			const struct sockaddr *, int);
	typedef int PASCAL (*iaxc_recvfrom_t)(SOCKET, char *, int, int,
			struct sockaddr *, int *);
#endif
#else
	/*!
		Defines the portotype for an application provided sendto implementation.
	*/
	typedef int (*iaxc_sendto_t)(int, const void *, size_t, int,
			const struct sockaddr *, socklen_t);
	/*!
		Defines the portotype for an application provided recvfrom implementation.
	*/
	typedef int (*iaxc_recvfrom_t)(int, void *, size_t, int,
			struct sockaddr *, socklen_t *);
#endif

/*!
	Mask containing all potentially valid audio formats
*/
#define IAXC_AUDIO_FORMAT_MASK  ((1<<16)-1)

/*!
	Mask containing all potentially valid video formats
*/
#define IAXC_VIDEO_FORMAT_MASK  (((1<<25)-1) & ~IAXC_AUDIO_FORMAT_MASK)

/* payload formats : WARNING: must match libiax values!!! */
/* Data formats for capabilities and frames alike */
#define IAXC_FORMAT_G723_1       (1 << 0)   /*!< G.723.1 compression */
#define IAXC_FORMAT_GSM          (1 << 1)   /*!< GSM compression */
#define IAXC_FORMAT_ULAW         (1 << 2)   /*!< Raw mu-law data (G.711) */
#define IAXC_FORMAT_ALAW         (1 << 3)   /*!< Raw A-law data (G.711) */
#define IAXC_FORMAT_G726         (1 << 4)   /*!< ADPCM, 32kbps  */
#define IAXC_FORMAT_ADPCM        (1 << 5)   /*!< ADPCM IMA */
#define IAXC_FORMAT_SLINEAR      (1 << 6)   /*!< Raw 16-bit Signed Linear (8000 Hz) PCM */
#define IAXC_FORMAT_LPC10        (1 << 7)   /*!< LPC10, 180 samples/frame */
#define IAXC_FORMAT_G729A        (1 << 8)   /*!< G.729a Audio */
#define IAXC_FORMAT_SPEEX        (1 << 9)   /*!< Speex Audio */
#define IAXC_FORMAT_ILBC         (1 << 10)  /*!< iLBC Audio */

#define IAXC_FORMAT_MAX_AUDIO    (1 << 15)  /*!< Maximum audio format value */
#define IAXC_FORMAT_JPEG         (1 << 16)  /*!< JPEG Images */
#define IAXC_FORMAT_PNG          (1 << 17)  /*!< PNG Images */
#define IAXC_FORMAT_H261         (1 << 18)  /*!< H.261 Video */
#define IAXC_FORMAT_H263         (1 << 19)  /*!< H.263 Video */
#define IAXC_FORMAT_H263_PLUS    (1 << 20)  /*!< H.263+ Video */
#define IAXC_FORMAT_H264         (1 << 21)  /*!< H264 Video */
#define IAXC_FORMAT_MPEG4        (1 << 22)  /*!< MPEG4 Video */
#define IAXC_FORMAT_THEORA       (1 << 24)  /*!< Theora Video */
#define IAXC_FORMAT_MAX_VIDEO    (1 << 24)  /*!< Maximum Video format value*/

#define IAXC_EVENT_TEXT          1   /*!< Indicates a text event */
#define IAXC_EVENT_LEVELS        2   /*!< Indicates a level event */
#define IAXC_EVENT_STATE         3   /*!< Indicates a call state change event */
#define IAXC_EVENT_NETSTAT       4   /*!< Indicates a network statistics update event */
#define IAXC_EVENT_URL           5   /*!< Indicates a URL push via IAX(2) */
#define IAXC_EVENT_VIDEO         6   /*!< Indicates a video event */
#define IAXC_EVENT_REGISTRATION  8   /*!< Indicates a registration event */
#define IAXC_EVENT_DTMF          9   /*!< Indicates a DTMF event */
#define IAXC_EVENT_AUDIO         10  /*!< Indicates an audio event */
#define IAXC_EVENT_VIDEOSTATS    11  /*!< Indicates a video statistics update event */
#define IAXC_EVENT_VIDCAP_ERROR  12  /*!< Indicates a video capture error occurred */
#define IAXC_EVENT_VIDCAP_DEVICE 13  /*!< Indicates a possible video capture device insertion/removal */

#define IAXC_CALL_STATE_FREE     0       /*!< Indicates a call slot is free */
#define IAXC_CALL_STATE_ACTIVE   (1<<1)  /*!< Indicates a call is active */
#define IAXC_CALL_STATE_OUTGOING (1<<2)  /*!< Indicates a call is outgoing */
#define IAXC_CALL_STATE_RINGING  (1<<3)  /*!< Indicates a call is ringing */
#define IAXC_CALL_STATE_COMPLETE (1<<4)  /*!< Indicates a completed call */
#define IAXC_CALL_STATE_SELECTED (1<<5)  /*!< Indicates the call is selected */
#define IAXC_CALL_STATE_BUSY     (1<<6)  /*!< Indicates a call is busy */
#define IAXC_CALL_STATE_TRANSFER (1<<7)  /*!< Indicates the call transfer has been released */

/*! Indicates that text is for an IAXClient status change */
#define IAXC_TEXT_TYPE_STATUS     1   
/*!  Indicates that text is an IAXClient warning message */
#define IAXC_TEXT_TYPE_NOTICE     2   
/*!  Represents that text is for an IAXClient error message */
#define IAXC_TEXT_TYPE_ERROR      3   
/*!  
	Represents that text is for an IAXClient fatal error message.
	
	The User Agent should probably display error message text, then die 
*/
#define IAXC_TEXT_TYPE_FATALERROR 4   
/*!  Represents a message sent from the server across the IAX stream*/
#define IAXC_TEXT_TYPE_IAX        5   

/* registration replys, corresponding to IAX_EVENTs*/
#define IAXC_REGISTRATION_REPLY_ACK     18   /*!< Indicates the registration was accepted (See IAX_EVENT_REGACC)  */
#define IAXC_REGISTRATION_REPLY_REJ     30   /*!< Indicates the registration was rejected (See IAX_EVENT_REGREJ)  */
#define IAXC_REGISTRATION_REPLY_TIMEOUT 6    /*!< Indicates the registration timed out (See IAX_EVENT_TIMEOUT) */

#define IAXC_URL_URL              1  /*!< URL received */
#define IAXC_URL_LDCOMPLETE       2  /*!< URL loading complete */
#define IAXC_URL_LINKURL          3  /*!< URL link request */
#define IAXC_URL_LINKREJECT       4  /*!< URL link reject */
#define IAXC_URL_UNLINK           5  /*!< URL unlink */

/* The source of the video or audio data triggering the event. */
#define IAXC_SOURCE_LOCAL  1 /*!<  Indicates that the event data source is local */
#define IAXC_SOURCE_REMOTE 2 /*!<  Indicates that the event data source is remote */

/*!
	The maximum size of a string contained within an event
 */
#define IAXC_EVENT_BUFSIZ 256

/*!
	A structure containing information about an audio level event.
*/
struct iaxc_ev_levels {
	/*!
		The input level in dB.
	*/
	float input;

	/*!
		The output level in dB.
	*/
	float output;
};

/*!
	A structure containing information about a text event.
*/
struct iaxc_ev_text {
	/*!
		The type of text event. 

		Valid values are from the IAXC_TEXT_TYPE_{} family of defines.
		\see IAXC_TEXT_TYPE_STATUS, IAXC_TEXT_TYPE_NOTICE, IAXC_TEXT_TYPE_ERROR, 
		IAXC_TEXT_TYPE_FATALERROR, IAXC_TEXT_TYPE_IAX
	*/
	int type;

	/*!
		The call the text is associated with or -1 if general text.
	*/
	int callNo; 

	/*!
		The UTF8 encoded text of the message.
	*/
	char message[IAXC_EVENT_BUFSIZ];
};

/*!
	A structure containing information about a call state change event.
*/
struct iaxc_ev_call_state {
	/*!
		The call number whose state this is
	*/
	int callNo;

	/*!
		The call state represented using the IAXC_CALL_STATE_{} defines.

		\see IAXC_CALL_STATE_FREE, IAXC_CALL_STATE_ACTIVE, IAXC_CALL_STATE_OUTGOING,
		IAXC_CALL_STATE_RINGING, IAXC_CALL_STATE_COMPLETE, IAXC_CALL_STATE_SELECTED,
		IAXC_CALL_STATE_BUSY, IAXC_CALL_STATE_TRANSFER
	*/
	int state;
	
	/*!
		The audio format of the call.

		\see IAXC_FORMAT_G723_1, IAXC_FORMAT_GSM, IAXC_FORMAT_ULAW, IAXC_FORMAT_ALAW,
		IAXC_FORMAT_G726, IAXC_FORMAT_ADPCM, IAXC_FORMAT_SLINEAR, IAXC_FORMAT_LPC10,
		IAXC_FORMAT_G729A, IAXC_FORMAT_SPEEX, IAXC_FORMAT_ILBC, IAXC_FORMAT_MAX_AUDIO
	*/
	int format;
	
	/*!
		The audio format of the call.

		\see IAXC_FORMAT_JPEG, IAXC_FORMAT_PNG, IAXC_FORMAT_H261, IAXC_FORMAT_H263,
		IAXC_FORMAT_H263_PLUS, IAXC_FORMAT_H264, IAXC_FORMAT_MPEG4, 
		IAXC_FORMAT_THEORA, IAXC_FORMAT_MAX_VIDEO
	*/
	int vformat;

	/*!
		The remote number.
	*/
	char remote[IAXC_EVENT_BUFSIZ];

	/*!
		The remote name.
	*/
	char remote_name[IAXC_EVENT_BUFSIZ];
	
	/*!
		The local number.
	*/
	char local[IAXC_EVENT_BUFSIZ];

	/*!
		The local calling context.
	*/
	char local_context[IAXC_EVENT_BUFSIZ];
};

/*!
	A structure containing information about a set of network statistics.
*/
struct iaxc_netstat {
	/*!
		The amount of observed jitter.
	*/
	int jitter;

	/*!
		The lost frame percentage.
	*/
	int losspct;

	/*!
		The number of missing frames.
	*/
	int losscnt;

	/*!
		The number of frames received.
	*/
	int packets;

	/*!
		The observed delay.
	*/
	int delay;
	
	/*!
		The number of frames dropped.
	*/
	int dropped;

	/*!
		The number of frames received out of order.
	*/
	int ooo;
};

/*!
	A structure containing information about a network statistics event.
*/
struct iaxc_ev_netstats {
	/*!
		The call whose statistics these are.
	*/
	int callNo;
	
	/*!
		The Round Trip Time
	*/
	int rtt;

	/*!
		The locally observed network statistics.
	*/
	struct iaxc_netstat local;

	/*!
		The remotely (peer) observed network statistics.
	*/
	struct iaxc_netstat remote;
};

/*!
	A structure containing video statistics data.
*/
struct iaxc_video_stats
{
	unsigned long received_slices;  /*!< Number of received slices. */
	unsigned long acc_recv_size;    /*!< Accumulated size of inbound slices. */
	unsigned long sent_slices;      /*!< Number of sent slices. */
	unsigned long acc_sent_size;    /*!< Accumulated size of outbound slices. */

	unsigned long dropped_frames;   /*!< Number of frames dropped by the codec (incomplete frames). */
	unsigned long inbound_frames;   /*!< Number of frames decoded by the codec (complete frames). */
	unsigned long outbound_frames;  /*!< Number of frames sent to the encoder. */

	float         avg_inbound_fps;  /*!< Average fps of inbound complete frames. */
	unsigned long avg_inbound_bps;  /*!< Average inbound bitrate. */
	float         avg_outbound_fps; /*!< Average fps of outbound frames. */
	unsigned long avg_outbound_bps; /*!< Average outbound bitrate. */

	struct timeval start_time;      /*!< Timestamp of the moment we started measuring. */
};

/*!
	A structure containing information about a video statistics event.
*/
struct iaxc_ev_video_stats {
	/*!
		The call whose statistics these are.
	*/
	int callNo;

	/*!
		The video statistics for the call.
	*/
	struct iaxc_video_stats stats;
};

/*!
	A structure containing information about an URL event.
*/
struct iaxc_ev_url {
	/*!
		The call this is for.
	*/
	int callNo;

	/*!
		The type of URL received. See the IAXC_URL_{} defines.

		\see IAXC_URL_URL, IAXC_URL_LINKURL, IAXC_URL_LDCOMPLETE, IAXC_URL_UNLINK,
		IAXC_URL_LINKREJECT
	*/
	int type;

	/*!
		The received URL.
	*/
	char url[IAXC_EVENT_BUFSIZ];
};

/*!
	A structure containing data for a video event.
*/
struct iaxc_ev_video {
	/*!
		The call this video data is for.

		Will be -1 for local video.
	*/
	int callNo;

	/*!
		Timestamp of the video
	*/
	unsigned int ts;

	/*!
		The format of the video data.

	\see IAXC_FORMAT_JPEG, IAXC_FORMAT_PNG, IAXC_FORMAT_H261, IAXC_FORMAT_H263,
	IAXC_FORMAT_H263_PLUS, IAXC_FORMAT_H264, IAXC_FORMAT_MPEG4, 
	IAXC_FORMAT_THEORA, IAXC_FORMAT_MAX_VIDEO
	*/
	int format;
	
	/*!
		The width of the video.
	*/
	int width;
	
	/*!
		The height of the video.
	*/
	int height;

	/*!
		Is the data encoded.

		1 for encoded data, 0 for raw.
	*/
	int encoded;

	/*!
		The source of the data.

		\see IAXC_SOURCE_LOCAL, IAXC_SOURCE_REMOTE
	*/
	int source;

	/*!
		The size of the video data in bytes.
	*/
	int size;

	/*!
		The buffer containing the video data.
	*/
	char *data;
};

/*!
	A structure containing data for an audio event.
*/
struct iaxc_ev_audio
{
	/*!
		The call this audio data is for.
	*/
	int callNo;

	/*!
		Timestamp of the video
	*/
	unsigned int ts;

	/*!
		The format of the data.

	\see IAXC_FORMAT_G723_1, IAXC_FORMAT_GSM, IAXC_FORMAT_ULAW, IAXC_FORMAT_ALAW,
	IAXC_FORMAT_G726, IAXC_FORMAT_ADPCM, IAXC_FORMAT_SLINEAR, IAXC_FORMAT_LPC10,
	IAXC_FORMAT_G729A, IAXC_FORMAT_SPEEX, IAXC_FORMAT_ILBC, IAXC_FORMAT_MAX_AUDIO
	*/
	int format;

	/*!
		Is the data encoded.

		1 for encoded data, 0 for raw.
	*/
	int encoded;

	/*!
		The source of the data.

		\see IAXC_SOURCE_LOCAL, IAXC_SOURCE_REMOTE
	*/
	int source;

	/*!
		The size of the audio data in bytes.
	*/ 
	int size;

	/*!
		The buffer containing the audio data.
	*/
	unsigned char *data;
};

/*!
	A structure containing information about a registration event 
*/
struct iaxc_ev_registration {
	/*!
		Indicates the registration id this event corresponds to.

		\see iaxc_register
	*/
	int id;

	/*!
		The registration reply.

		The values are from the IAXC_REGISTRATION_REPLY_{} family of macros.
		\see IAX_EVENT_REGACC, IAX_EVENT_REGREJ, IAX_EVENT_TIMEOUT
	*/
	int reply;

	/*!
		The number of 'voicemail' messages.
	*/
	int msgcount;
};

/*!
	A structure containing information about a DTMF event
 */
struct iaxc_ev_dtmf {
	/*!
		The call this DTMF event is for.
	 */
	int  callNo;

	/*!
		The digit represented by this DTMF tone
	 */
	char digit;
};

/*!
	A structure describing a single IAXClient event.
*/
typedef struct iaxc_event_struct {
	/*!
		Points to the next entry in the event queue
		\internal
	*/
	struct iaxc_event_struct *next;

	/*!
		The type uses one of the IAXC_EVENT_{} macros to describe which type of
		event is being presented
	*/
	int type; 
	
	/*!
		Contains the data specific to the type of event.
	*/
	union {
		/*! Contains level data if type = IAXC_EVENT_LEVELS */
		struct iaxc_ev_levels           levels;
		/*! Contains text data if type = IAXC_EVENT_TEXT */
		struct iaxc_ev_text             text;       
		/*! Contains call state data if type = IAXC_EVENT_STATE */
		struct iaxc_ev_call_state       call;       
		/*! Contains network statistics if type = IAXC_EVENT_NETSTAT */
		struct iaxc_ev_netstats         netstats;   
		/*! Contains video statistics if type = IAXC_EVENT_VIDEOSTATS */
		struct iaxc_ev_video_stats      videostats; 
		/*! Contains url data if type = IAXC_EVENT_URL */
		struct iaxc_ev_url              url;        
		/*! Contains video data if type = IAXC_EVENT_VIDEO */
		struct iaxc_ev_video            video;      
		/*! Contains audio data if type = IAXC_EVENT_AUDIO */
		struct iaxc_ev_audio            audio;      
		/*! Contains registration data if type = IAXC_EVENT_REGISTRATION */
		struct iaxc_ev_registration     reg;
		/*! Contains DTMF data if type = IAXC_EVENT_DTMF */
		struct iaxc_ev_dtmf             dtmf;
	} ev;
} iaxc_event;

/*!
	Defines the prototype for event callback handlers
	\param e The event structure being passed to the callback
	
	\return The result of processing the event; > 0 if successfully handled the event, 0 if not handled, < 0 to indicate an error occurred processing the event.
*/
typedef int (*iaxc_event_callback_t)(iaxc_event e);

/*!
	Sets the callback to call with IAXClient events
	\param func The callback function to call with events
*/
EXPORT void iaxc_set_event_callback(iaxc_event_callback_t func);

/*!
	Sets iaxclient to post a pointer to a copy of event using o/s specific Post method 
	\param handle
	\param id
*/
EXPORT int iaxc_set_event_callpost(void *handle, int id);

/*!
	frees event delivered via o/s specific Post method 
	\param e The event to free
*/
EXPORT void iaxc_free_event(iaxc_event *e);


/* Event Accessors */
/*!
	Returns the levels data associated with event \a e.
	\param e The event to retrieve the levels from.
*/
EXPORT struct iaxc_ev_levels *iaxc_get_event_levels(iaxc_event *e);

/*!
	Returns the text data associated with event \a e.
	\param e The event to retrieve text from.
*/
EXPORT struct iaxc_ev_text *iaxc_get_event_text(iaxc_event *e);

/*!
	Returns the event state data associated with event \a e.
	\param e The event to retrieve call state from.
*/
EXPORT struct iaxc_ev_call_state *iaxc_get_event_state(iaxc_event *e);

/*!
	Set Preferred UDP Port:
	\param sourceUdpPort The source UDP port to prefer
	0 Use the default port (4569), <0 Uses a dynamically assigned port, and
	>0 tries to bind to the specified port

	\note must be called before iaxc_initialize()
*/
EXPORT void iaxc_set_preferred_source_udp_port(int sourceUdpPort);

/*!
	Returns the UDP port that has been bound to.

	\return The UDP port bound to; -1 if no port or
*/
EXPORT int iaxc_get_bind_port();

/*!
	Initializes the IAXClient library
	\param num_calls The maximum number of simultaneous calls to handle.

	This initializes the IAXClient 
*/
EXPORT int iaxc_initialize(int num_calls);

/*!
	Shutsdown the IAXClient library.
	
	This should be called by applications utilizing iaxclient before they exit.
	It dumps all calls, and releases any audio/video drivers being used.

	\note It is unsafe to call most IAXClient API's after calling this!
*/
EXPORT void iaxc_shutdown();

/*!
	Sets the formats to be used
	\param preferred The single preferred audio format
	\param allowed A mask containing all audio formats to allow

	\see IAXC_FORMAT_G723_1, IAXC_FORMAT_GSM, IAXC_FORMAT_ULAW, IAXC_FORMAT_ALAW,
	IAXC_FORMAT_G726, IAXC_FORMAT_ADPCM, IAXC_FORMAT_SLINEAR, IAXC_FORMAT_LPC10,
	IAXC_FORMAT_G729A, IAXC_FORMAT_SPEEX, IAXC_FORMAT_ILBC, IAXC_FORMAT_MAX_AUDIO
*/
EXPORT void iaxc_set_formats(int preferred, int allowed);

/*!
	Sets the minimum outgoing frame size.
	\param samples The minimum number of samples to include in an outgoing frame.
*/
EXPORT void iaxc_set_min_outgoing_framesize(int samples);

/*!
	Sets the caller id \a name and \a number.
	\param name The caller id name.
	\param number The caller id number.
*/
EXPORT void iaxc_set_callerid(const char * name, const char * number);

/*!
	Starts all the internal processing thread(s).

	\note Should be called after iaxc_initialize, but before any call processing
	related functions.
*/
EXPORT int iaxc_start_processing_thread();

/*!
	Stops all the internal processing thread(s).

	\note Should be called before iaxc_shutdown.
*/
EXPORT int iaxc_stop_processing_thread();

/*!
	Initiates a call to an end point
	\param num The entity to call in the format of [user[:password]]@@peer[:portno][/exten[@@context]]

	\return The call number upon sucess; -1 otherwise.

	\note This is the same as calling iaxc_call_ex(num, NULL, NULL, 1).
*/
EXPORT int iaxc_call(const char * num);

/*!
	Initiates a call to an end point
	\param num The entity to call in the format of [user[:password]]@@peer[:portno][/exten[@@context]]
	\param callerid_name The local caller id name to use
	\param callerid_number The local caller id number to use
	\param video 0 indicates no-video. Any non-zero value indicates video is requested

	\return The call number upon sucess; -1 otherwise.
*/
EXPORT int iaxc_call_ex(const char* num, const char* callerid_name, const char* callerid_number, int video);

/*!
	Unregisters IAXClient from a server
	\param id The registration number returned by iaxc_register.
*/
EXPORT int iaxc_unregister( int id );

/*!
	Registers the IAXClient instance with an IAX server
	\param user The username to register as
	\param pass The password to register with
	\param host The address of the host/peer to register with

	\return The registration id number upon success; -1 otherwise.
*/
EXPORT int iaxc_register(const char * user, const char * pass, const char * host);

/*!
	Registers the IAXClient instance with an IAX server
	\param user The username to register as
	\param pass The password to register with
	\param host The address of the host/peer to register with
	\param refresh The registration refresh period

	\return The registration id number upon success; -1 otherwise.
*/
EXPORT int iaxc_register_ex(const char * user, const char * pass, const char * host, int refresh);

/*!
	Respond to incoming call \a callNo as busy.
*/
EXPORT void iaxc_send_busy_on_incoming_call(int callNo);

/*!
	Answers the incoming call \a callNo.
	\param callNo The number of the call to answer.
*/
EXPORT void iaxc_answer_call(int callNo);

/*!
	Initiate a blind call transfer of \a callNo to \a number.
	\param callNo The active call to transfer.
	\param number The number to transfer the call to. See draft-guy-iax-03 section 8.4.1 for further details.
*/
EXPORT void iaxc_blind_transfer_call(int callNo, const char * number);

/*!
	Setup a transfer of \a sourceCallNo to \a targetCallNo.
	\param sourceCallNo The number of the active call session to transfer.
	\param targetCallNo The active call session to be transferred to.

	This is used in performing as the final step in an attended call transfer.
*/
EXPORT void iaxc_setup_call_transfer(int sourceCallNo, int targetCallNo);

/*!
	Hangs up and frees all non-free calls.
*/
EXPORT void iaxc_dump_all_calls(void);

/*!
	Hangs up and frees call \a callNo
	\param callNo The call number to reject.
*/
EXPORT void iaxc_dump_call_number( int callNo );

/*!
	Hangs up and frees the currently selected call.
*/
EXPORT void iaxc_dump_call(void);

/*!
	Rejects the currently selected call.

	\note This is pretty much a useless API, since the act of selecting a call
	will answer it.
*/
EXPORT void iaxc_reject_call(void);

/*!
	Rejects the incoming call \a callNo.
	\param callNo The call number to reject.
*/
EXPORT void iaxc_reject_call_number(int callNo);

/*!
	Sends a DTMF digit to the currently selected call.
	\param digit The DTMF digit to send (0-9, A-D, *, #).
*/
EXPORT void iaxc_send_dtmf(char digit);

/*!
	Sends text to the currently selected call.
*/
EXPORT void iaxc_send_text(const char * text);

/*!
	Sends \a text to call \a callNo
*/
EXPORT void iaxc_send_text_call(int callNo, const char * text);

/*!
	Sends a URL across the currently selected call
	\param url The URL to send across.
	\param link If non-zero the URL is a link
*/
EXPORT void iaxc_send_url(const char *url, int link); /* link == 1 ? AST_HTML_LINKURL : AST_HTML_URL */

/*!
	Suspends thread execution for an interval measured in milliseconds
	\param ms The number of milliseconds to sleep
*/
EXPORT void iaxc_millisleep(long ms);

/*!
	Sets the silence threshold to \a thr.
	\param thr The threshold value in dB. A value of 0.0f effectively mutes audio input.
*/
EXPORT void iaxc_set_silence_threshold(float thr);

/*!
	Sets the audio output to \a mode.
	\param mode The audio mode 0 indicates remote audio should be played; non-zero prevents remote audio from being played.
*/
EXPORT void iaxc_set_audio_output(int mode);

/*!
	Sets \a callNo as the currently selected call
	\param callNo The call to select or < 0 to indicate no selected call.

	\note Will answer an incoming ringing call as a side effect. Personally I
	believe this behavior is undesirable and feel it renders iaxc_reject_call
	pretty much useless.
*/
EXPORT int iaxc_select_call(int callNo);

/*!
	Returns the first free call number.
*/
EXPORT int iaxc_first_free_call();

/*!
	Returns the number of the currently selected call.
*/
EXPORT int iaxc_selected_call();

/*!
	Causes the audio channel for \a callNo to QUELCH (be squelched).
	\param callNo The number of the active, accepted call to quelch.
	\param MOH If non-zero Music On Hold should be played on the QUELCH'd call.
*/
EXPORT int iaxc_quelch(int callNo, int MOH);

/*!
	Causes the audio channel for \a callNo to be UNQUELCH (unsquelched).
*/
EXPORT int iaxc_unquelch(int callNo);

/*!
	Returns the current mic boost setting.

	\return 0 if mic boost is disabled; otherwise non-zero.
*/
EXPORT int iaxc_mic_boost_get( void ) ;

/*!
	Sets the mic boost setting.
	\param enable If non-zero enable the mic boost; otherwise disable.
*/
EXPORT int iaxc_mic_boost_set( int enable ) ;

/*!
	Returns a copy of IAXClient library version
	\param ver A buffer to store the version string in. It MUST be at least 
	IAXC_EVENT_BUFSIZ bytes in size.

	\return the version string (as stored in \a ver).
*/
EXPORT char* iaxc_version(char *ver);

/*!
	Fine tune jitterbuffer control
	\param value
*/
EXPORT void iaxc_set_jb_target_extra( long value );

/*!
	Application-defined networking; give substitute sendto and recvfrom 
	functions.
	\param st The send function to use.
	\param rf The receive function to use.

	\note Must be called before iaxc_initialize! 
*/
EXPORT void iaxc_set_networking(iaxc_sendto_t st, iaxc_recvfrom_t rf) ;

/*!
	wrapper for libiax2 get_netstats 
	\param call
	\param rtt
	\param local
	\param remote
*/
EXPORT int iaxc_get_netstats(int call, int *rtt, struct iaxc_netstat *local, struct iaxc_netstat *remote);

/*!
	A structure containing information about a video capture device.
*/
struct iaxc_video_device {
	/*!
		The "human readable" name of the device
	*/
	const char *name;

	/*!
		unique id of the device
	*/
	const char *id_string;

	/*!
		iaxclient id of the device
	*/
	int id;
};

#define IAXC_AD_INPUT           (1<<0) /*!< Device is usable for input*/
#define IAXC_AD_OUTPUT          (1<<1) /*!< Device is usable for output */
#define IAXC_AD_RING            (1<<2) /*!< Device is usable for ring */
#define IAXC_AD_INPUT_DEFAULT   (1<<3) /*!< Indicates the default input device */
#define IAXC_AD_OUTPUT_DEFAULT  (1<<4) /*!< Indicates the default output device */
#define IAXC_AD_RING_DEFAULT    (1<<5) /*!< Indicates the default ring device */

/*!
	A structure containing information about an audio device.
*/
struct iaxc_audio_device {
	/*!
		The "human readable" name of the device
	*/
	const char * name;

	/*!
		Capability flags, defined using the IAXC_AD_{} macros.
	*/
	long capabilities;      

	/*!
		The device driver specific ID.
	*/
	int devID;
};

/*! Get audio device information:
	 \param devs Returns an array of iaxc_audio_device structures.
	        The array will will be valid as long as iaxc is initialized.
	 \param nDevs Returns the number of devices in the devs array
	 \param input Returns the currently selected input device
	 \param output Returns the currently selected output device
	 \param ring Returns the currently selected ring device
 */
EXPORT int iaxc_audio_devices_get(struct iaxc_audio_device **devs, int *nDevs, int *input, int *output, int *ring);

/*!
	Sets the current audio devices
	\param input The device to use for audio input
	\param output The device to use for audio output
	\param ring The device to use to present ring sounds
 */
EXPORT int iaxc_audio_devices_set(int input, int output, int ring);

/*!
	Get the audio device input level.
	
	\return the input level in the range of 0.0f minimum to 1.0f maximum.
 */
EXPORT float iaxc_input_level_get();

/*!
	Get the audio device output level.
	
	\return the input level in the range of 0.0f minimum to 1.0f maximum.
 */
EXPORT float iaxc_output_level_get();

/*!
	Sets the audio input level to \a level.
	\param level The level in the range from 0.0f (min) to 1.0f (max).
*/
EXPORT int iaxc_input_level_set(float level);

/*!
	Sets the audio output level to \a level.
	\param level The level in the range from 0.0f (min) to 1.0f (max).
 */
EXPORT int iaxc_output_level_set(float level);

/*!
	A structure describing a sound to IAXClient
*/
struct iaxc_sound {
	short   *data;           /*!< Sound sample data in 8KHz 16-bit signed format. */
	long    len;             /*!< Length of sample in frames. */
	int     malloced;        /*!< Should the library free() the data after it is played? */
	int     channel;         /*!< The channel used: 0 for output, 1 for ring. */
	int     repeat;          /*!< Number of times to repeat (-1 = infinite). */
	long    pos;             /*!< \internal use: current play position. */
	int     id;              /*!< \internal use: sound ID. */
	struct iaxc_sound *next; /*!< \internal use: next in list. */
};

/*!
	Play a sound.
	\param sound An iaxc_sound structure.
	\param ring 0 to play through output device or 1 to play through the "ring" device.

	\return The id number of the sound being played
*/
EXPORT int iaxc_play_sound(struct iaxc_sound *sound, int ring);

/*! 
	Stop sound \a id from being played.
	\param id The id of a sound to stop as returned from iaxc_play_sound.
*/
EXPORT int iaxc_stop_sound(int id);

#define IAXC_FILTER_DENOISE     (1<<0) /*!< Noise reduction filter */
#define IAXC_FILTER_AGC         (1<<1) /*!< Automatic Gain Control */
#define IAXC_FILTER_ECHO        (1<<2) /*!< Echo cancellation filter */
#define IAXC_FILTER_AAGC        (1<<3) /*!< Analog (mixer-based) Automatic Gain Control */
#define IAXC_FILTER_CN          (1<<4) /*!< Send Comfort Noise (CN) frames when silence is detected */

/*!
	Returns the set of audio filters being applied.

	The IAXC_FILTER_{} defines are used to specify the filters.
	\see IAXC_FILTER_DENOISE, IAXC_FILTER_AGC, IAXC_FILTER_ECHO, IAXC_FILTER_AAGC,
	IAXC_FILTER_CN
 */
EXPORT int iaxc_get_filters(void);

/*!
	Sets the current audio filters to apply.
	\param filters The combination of all the audio filters to use (IAXC_FILTER_{} defines).

	The IAXC_FILTER_{} defines are used to specify the filters.
	\see IAXC_FILTER_DENOISE, IAXC_FILTER_AGC, IAXC_FILTER_ECHO, IAXC_FILTER_AAGC,
	IAXC_FILTER_CN
 */
EXPORT void iaxc_set_filters(int filters);

/*! 
	Sets speex specific codec settings 
	\param decode_enhance 1/0  perceptual enhancement for decoder
	\param quality: Generally, set either quality (0-9) or bitrate. -1 for "default"
	\param bitrate in kbps.  Applies to CBR only; -1 for default.
      (overrides "quality" for CBR mode)
	\param vbr Variable bitrate mode:  0/1
	\param abr mode/rate: 0 for not ABR, bitrate for ABR mode
	\param complexity Algorithmic complexity.  Think -N for gzip.
      Higher numbers take more CPU for better quality.  3 is
      default and good choice.

	A good choice is (1,-1,-1,0,8000,3): 8kbps ABR 
 */
EXPORT void iaxc_set_speex_settings(int decode_enhance, float quality, int bitrate, int vbr, int abr, int complexity);

/*
 Functions and flags for setting and getting audio callback preferences
  The application can request to receive local/remote, raw/encoded audio
  through the callback mechanism. Please note that changing callback
  settings will overwrite all previous settings.
*/
/*! 
	Indicates the preference that local audio should be passed to the registered callback in a raw 8KHz 16-bit signed format. 
*/
#define IAXC_AUDIO_PREF_RECV_LOCAL_RAW      (1 << 0) 

/*!
	Indicates the preference that local audio should be passed to the registered callback in the current encoded format.
*/
#define IAXC_AUDIO_PREF_RECV_LOCAL_ENCODED  (1 << 1) 

/*!
	Indicates the preference that remote audio should be passed to the registered callback in a raw 8KHz 16-bit signed format.
*/
#define IAXC_AUDIO_PREF_RECV_REMOTE_RAW     (1 << 2) 

/*!
	Indicates the preference that remote audio should be passed to the registered callback in the current encoded format.
*/
#define IAXC_AUDIO_PREF_RECV_REMOTE_ENCODED (1 << 3) 

/*!
	Indicates the preference that sending of audio should be disabled.
*/
#define IAXC_AUDIO_PREF_SEND_DISABLE        (1 << 4) 

/*! 
	 Returns the various audio delivery preferences.

	 The preferences are represented using the AIXC_AUDIO_PREF_{} family of defines.
*/
EXPORT unsigned int iaxc_get_audio_prefs(void);

/*!
	Set the various audio delivery preferences
	\param prefs The desired preferences to use. They are represented using the AIXC_AUDIO_PREF_{} family of defines.
	 
	 \return 0 on success and -1 on error.

	 \see IAXC_AUDIO_PREF_RECV_LOCAL_RAW, IAXC_AUDIO_PREF_RECV_LOCAL_ENCODED,
	 IAXC_AUDIO_PREF_RECV_REMOTE_RAW, IAXC_AUDIO_PREF_RECV_REMOTE_ENCODED,
	 IAXC_AUDIO_PREF_SEND_DISABLE
 */
EXPORT int iaxc_set_audio_prefs(unsigned int prefs);

/*!
	Get video capture device information.
	WARNING: the array pointed to by parameter 'devs' below is owned
	         by iaxclient, and may be freed on subsequent calls to
	         this function.
	 \param devs Returns an array of iaxc_video_device structures.
	        The array will only be valid until this function is
	        called again (if the device list changes), or until
	        iaxc is shutdown.
	 \param nDevs Returns the number of devices in the devs array
	 \param devId Returns the id of the currently selected video capture device

	 \return -1 on error, 0 if no change to the device list, 1 if it's been updated
 */
EXPORT int iaxc_video_devices_get(struct iaxc_video_device **devs, int *nDevs, int *devId);

/*!
	Sets the current video capture device
	\param devId The id of the device to use for video capture
 */
EXPORT int iaxc_video_device_set(int devId);

/*
 * Acceptable range for video rezolution
 */
#define IAXC_VIDEO_MAX_WIDTH    704 /*!< Maximum video width */
#define IAXC_VIDEO_MAX_HEIGHT   576 /*!< Maximum video height */
#define IAXC_VIDEO_MIN_WIDTH    80  /*!< Minimum video width */
#define IAXC_VIDEO_MIN_HEIGHT   60  /*!< Minimum video height */

/*
 Video callback preferences
 The client application can obtain any combination of
 remote/local, encoded/raw video through the event callback
 mechanism
 Use these flags to specify what kind of video do you want to receive
 */

/*! 
	Indicates the preference that local video should be passed to the registered callback in a raw format (typically YUV420). 
*/
#define IAXC_VIDEO_PREF_RECV_LOCAL_RAW      (1 << 0) 

/*!
	Indicates the preference that local video should be passed to the registered callback in the current encoded format.
*/
#define IAXC_VIDEO_PREF_RECV_LOCAL_ENCODED  (1 << 1) 

/*!
	Indicates the preference that remote video should be passed to the registered callback in a raw format (typically YUV420).
*/
#define IAXC_VIDEO_PREF_RECV_REMOTE_RAW     (1 << 2) 

/*!
	Indicates the preference that remote video should be passed to the registered callback in the current encoded format.
*/
#define IAXC_VIDEO_PREF_RECV_REMOTE_ENCODED (1 << 3) 

/*!
	Indicates the preference that sending of video should be disabled.
*/
#define IAXC_VIDEO_PREF_SEND_DISABLE        (1 << 4) 

/*!
	This flag specifies that you want raw video in RGB32 format

	RGB32: FFRRGGBB aligned 4 bytes per pixel
  When this flag is set, iaxclient will convert YUV420 raw video into
  RGB32 before passing it to the main app.
 */
#define IAXC_VIDEO_PREF_RECV_RGB32          (1 << 5)

/*!
  Use this flag to disable/enable camera hardware
 */
#define IAXC_VIDEO_PREF_CAPTURE_DISABLE     (1 << 6)

/*!
	Returns the current video preferences.

	 The preferences are represented using the AIXC_VIDEO_PREF_{} family of macros.

	\see IAXC_VIDEO_PREF_RECV_LOCAL_RAW, IAXC_VIDEO_PREF_RECV_LOCAL_ENCODED,
	IAXC_VIDEO_PREF_RECV_REMOTE_RAW, IAXC_VIDEO_PREF_RECV_REMOTE_ENCODED,
	IAXC_VIDEO_PREF_SEND_DISABLE, IAXC_VIDEO_PREF_RECV_RGB32, 
	IAXC_VIDEO_PREF_CAPTURE_DISABLE
*/
EXPORT unsigned int iaxc_get_video_prefs(void);

/*!
	Sets the current video preferences.
	\param prefs The desired preferences to use. They are represented using the IAXC_VIDEO_PREF_{} family of defines.
	
  Please note that this overwrites all previous preferences. In other
  words, a read-modify-write must be done to change a single preference.

	\return 0 on success and -1 on error.

	\see IAXC_VIDEO_PREF_RECV_LOCAL_RAW, IAXC_VIDEO_PREF_RECV_LOCAL_ENCODED,
	IAXC_VIDEO_PREF_RECV_REMOTE_RAW, IAXC_VIDEO_PREF_RECV_REMOTE_ENCODED,
	IAXC_VIDEO_PREF_SEND_DISABLE, IAXC_VIDEO_PREF_RECV_RGB32, 
	IAXC_VIDEO_PREF_CAPTURE_DISABLE
 */
EXPORT int iaxc_set_video_prefs(unsigned int prefs);

/*!
	Returns the video format settings
	\param preferred Receives the single preferred video format
	\param allowed Receives the mask of the allowed video formats

	\see IAXC_FORMAT_JPEG, IAXC_FORMAT_PNG, IAXC_FORMAT_H261, IAXC_FORMAT_H263,
	IAXC_FORMAT_H263_PLUS, IAXC_FORMAT_H264, IAXC_FORMAT_MPEG4, 
	IAXC_FORMAT_THEORA, IAXC_FORMAT_MAX_VIDEO
*/
EXPORT void iaxc_video_format_get_cap(int *preferred, int *allowed);

/*!
	Sets the video format capabilities
	\param preferred The single preferred video format
	\param allowed The mask of the allowed video formats

	\see IAXC_FORMAT_JPEG, IAXC_FORMAT_PNG, IAXC_FORMAT_H261, IAXC_FORMAT_H263,
	IAXC_FORMAT_H263_PLUS, IAXC_FORMAT_H264, IAXC_FORMAT_MPEG4, 
	IAXC_FORMAT_THEORA, IAXC_FORMAT_MAX_VIDEO
*/
EXPORT void iaxc_video_format_set_cap(int preferred, int allowed);

/*!
	Sets the allowed/preferred video formats
	\param preferred The single preferred video format
	\param allowed The mask of the allowed video formats
	\param framerate The video frame rate in fps.
	\param bitrate The video bitrate in bps.
	\param width The width of the video.
	\param height The height of the video.
	\param fs The video fragment size.

	\see IAXC_FORMAT_JPEG, IAXC_FORMAT_PNG, IAXC_FORMAT_H261, IAXC_FORMAT_H263,
	IAXC_FORMAT_H263_PLUS, IAXC_FORMAT_H264, IAXC_FORMAT_MPEG4, 
	IAXC_FORMAT_THEORA, IAXC_FORMAT_MAX_VIDEO
*/
EXPORT void iaxc_video_format_set(int preferred, int allowed, int framerate, int bitrate, int width, int height, int fs);

/*!
 Change video params for the current call on the fly
 This will destroy the existing encoder and create a new one
 use negative values for parameters that should not change
 \param framerate The video frame rate in fps.
 \param bitrate The video bitrate in bps.
 \param width The width of the video.
 \param height The height of the video.
 \param fs The video fragment size.
*/
EXPORT void iaxc_video_params_change(int framerate, int bitrate, int width, int height, int fs);

/*! Set holding frame to be used in some kind of video calls */
EXPORT int iaxc_set_holding_frame(char *);

/*!
	Helper function to control use of jitter buffer for video events 

	\todo make this a video pref, perhaps? 
*/

EXPORT int iaxc_video_bypass_jitter(int);

/*!
  Returns 1 if the default camera is working; 0 otherwise
 */
EXPORT int iaxc_is_camera_working();

/*!
	Converts a YUV420 image to RGB32
	\param width The width of the image
	\param height The height of the image
	\param src The buffer containing the source image
	\param dest The buffer to write the converted image to. The buffer should be \a width * height * 4 bytes in size.

	Converts the image based on the forumulas found at
	http://en.wikipedia.org/wiki/YUV
*/
EXPORT void iaxc_YUV420_to_RGB32(int width, int height, const char *src, char *dest);

/*
 * Test mode functionality
 * In test mode, iaxclient will do the following:
 *   - skip audio and video hardware initialization
 *   - wait for outgoing media to be provided by the main application
 *   - return incoming media to the calling application if required, via callbacks
 *   - not generate any meaningful statistics
 * Test mode is designed to be used without a GUI, and with multiple instances of iaxclient
 * running on the same machine. However, some applications might actually benefit from having
 * this level of control.
 * iaxc_set_test_mode() should be called before iaxc_initialize()
 */
EXPORT void iaxc_set_test_mode(int);

/*!
	\brief Sends compressed audio data to the currently selected call.
	\param data compressed audio data
	\param size Size of the compressed audio data in bytes
	\param samples The number of (uncompressed) samples represented by the compressed audio data. We normally use 20ms packets at a sampling rate of 8000Hz, so this would be 160.

	\note Data must be in the audio format that was negotiated for the current call
	otherwise bad magic may occur on the recieving side.
*/
EXPORT int iaxc_push_audio(void *data, unsigned int size, unsigned int samples);

/*!
	\brief Sends compressed video data to the currently selected call.
	\param data compressed video data
	\param size Size of the compressed video data in bytes
	\param fragment If true, split video frames larger than the current fragsize into multiple fragments, otherwise send the data as jumbo frames.

	\note Data must be in the video format that was negotiated for the current call
	otherwise bad magic may occur on the recieving side.
*/
EXPORT int iaxc_push_video(void *data, unsigned int size, int fragment);

/*!
	Sets the IAX debug set to \a enable.
	\param enable If non-zero enable iax protocol debugging
*/
EXPORT void iaxc_debug_iax_set(int enable);

#ifdef __cplusplus
}
#endif

#endif
