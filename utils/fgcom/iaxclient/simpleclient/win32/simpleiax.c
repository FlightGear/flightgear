/*
 * Miniphone: A simple, command line telephone
 *
 * IAX Support for talking to Asterisk and other Gnophone clients
 *
 * Copyright (C) 1999, Linux Support Services, Inc.
 *
 * Mark Spencer <markster@linux-support.net>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 */

/* #define	PRINTCHUCK /* enable this to indicate chucked incomming packets */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
//#include <conio.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <process.h>
#include <windows.h>
#include <winsock.h>
#include <mmsystem.h>
#include <malloc.h>

// VC++ Memory Leak Detection (Debug malloc Header)
// Comment out malloc.h line before using
//#define _CRTDBG_MAP_ALLOC
//#include <crtdbg.h>

//#include "gsm.h"
#include "iaxclient_lib.h"
//#include "audio_win32.h"
#include "audio_encode.h"
#include "frame.h"
#include "miniphone.h"

static int answered_call;

static void parse_args(FILE *, unsigned char *);
void call(FILE *, char *);
void answer_call(void);
void reject_call(void);
void parse_cmd(FILE *, int, char **);
void issue_prompt(FILE *);
void dump_array(FILE *, char **);

static char *help[] = {
"Welcome to the miniphone telephony client, the commands are as follows:\n",
"Help\t\t-\tDisplays this screen.",
"Call <Number>\t-\tDials the number supplied.",
"Answer\t\t-\tAnswers an Inbound call.",
"Reject\t\t-\tRejects an Inbound call.",
"Dump\t\t-\tDumps (disconnects) the current call.",
"Dtmf <Digit>\t-\tSends specified DTMF digit.",
"Status\t\t-\tLists the current sessions and their current status.",
"Quit\t\t-\tShuts down the client.",
"",
0
};

static struct peer *most_recent_answer;
static struct iax_session *newcall = 0;

/* routine called at exit to shutdown audio I/O and close nicely.
NOTE: If all this isnt done, the system doesnt not handle this
cleanly and has to be rebooted. What a pile of doo doo!! */
void killem(void)
{
	shutdown_client();

	// VC++ Memory Leak Detection
	//_CrtDumpMemoryLeaks();

	return;
}

void parse_args(FILE *f, unsigned char *cmd)
{
	static char *argv[MAXARGS];
	unsigned char *parse = cmd;
	int argc = 0, t = 0;

	// Don't mess with anything that doesn't exist...
	if(!*parse)
		return;

	memset(argv, 0, sizeof(argv));
	while(*parse) {
		if(*parse < 33 || *parse > 128) {
			*parse = 0, t++;
			if(t > MAXARG) {
				fprintf(f, "Warning: Argument exceeds maximum argument size, command ignored!\n");
				return;
			}
		} else if(t || !argc) {
			if(argc == MAXARGS) {
				fprintf(f, "Warning: Command ignored, too many arguments\n");
				return;
			}
			argv[argc++] = parse;
			t = 0;
		}

		parse++;
	}

	if(argc)
		parse_cmd(f, argc, argv);
}

int main(int argc, char *argv[])
{
	FILE *f;
	char rcmd[RBUFSIZE];
//	gsm_frame fo;
//	time_t	t;
	unsigned long lastouttick = 0;

	f = stdout;
	_dup2(fileno(stdout),fileno(stderr));

	/* activate the exit handler */
	atexit(killem);
	
	initialize_client(AUDIO_INTERNAL, f);
	set_encode_format(AST_FORMAT_GSM);

	fprintf(f, "Text Based Telephony Client.\n\n");
	issue_prompt(f);

	/* main tight loop */
	while(1) {

//		process_calls();
		answered_call = was_call_answered();
		/* if key pressed, do command stuff */
		if(_kbhit())
		{
				if ( ( fgets(&*rcmd, 256, stdin))) {
					rcmd[strlen(rcmd)-1] = 0;
					parse_args(f, &*rcmd);
				} else fprintf(f, "Fatal error: failed to read data!\n");

				issue_prompt(f);
		}
		Sleep(10);
	}

	return 0;
}

void call(FILE *f, char *num)
{
	client_call(f, num);
	struct peer *peer;

	if(!newcall)
		newcall = iax_session_new();
	else {
		fprintf(f, "Already attempting to call somewhere, please cancel first!\n");
		return;
	}

	if ( !(peer = malloc(sizeof(struct peer)))) {
		fprintf(f, "Warning: Unable to allocate memory!\n");
		return;
	}

	peer->time = time(0);
	peer->session = newcall;
	peer->gsmin = 0;
	peer->gsmout = 0;

	peer->next = peers;
	peers = peer;

	most_recent_answer = peer;

	iax_call(peer->session, "WinIAX", num, NULL, 10);
}

void
answer_call(void)
{
	client_answer_call();
}

void
dump_call(void)
{
	client_dump_call();
}

void
reject_call(void)
{
	client_reject_call();
}

void parse_cmd(FILE *f, int argc, char **argv)
{
	_strupr(argv[0]);
	if(!strcmp(argv[0], "HELP")) {
		if(argc == 1)
			dump_array(f, help);
		else if(argc == 2) {
			if(!strcmp(argv[1], "HELP"))
				fprintf(f, "Help <Command>\t-\tDisplays general help or specific help on command if supplied an arguement\n");
			else if(!strcmp(argv[1], "QUIT"))
				fprintf(f, "Quit\t\t-\tShuts down the miniphone\n");
			else fprintf(f, "No help available on %s\n", argv[1]);
		} else {
			fprintf(f, "Too many arguements for command help.\n");
		}
	} else if(!strcmp(argv[0], "STATUS")) {
		if(argc == 1) {
			int c = 0;
			struct peer *peerptr = peers;

			if(!peerptr)
				fprintf(f, "No session matches found.\n");
			else while(peerptr) {
	 			fprintf(f, "Listing sessions:\n\n");
				fprintf(f, "Session %d\n", ++c);
				fprintf(f, "Session existed for %d seconds\n", (int)time(0)-peerptr->time);
				if(answered_call)
					fprintf(f, "Call answered.\n");
				else fprintf(f, "Call ringing.\n");

				peerptr = peerptr->next;
			}
		} else fprintf(f, "Too many arguments for command status.\n");
	} else if(!strcmp(argv[0], "ANSWER")) {
		if(argc > 1)
			fprintf(f, "Too many arguements for command answer\n");
		else answer_call();
	} else if(!strcmp(argv[0], "REJECT")) {
		if(argc > 1)
			fprintf(f, "Too many arguements for command reject\n");
		else {
			fprintf(f, "Rejecting current phone call.\n");
			reject_call();
		}
	} else if(!strcmp(argv[0], "CALL")) {
		if(argc > 2)
			fprintf(f, "Too many arguements for command call\n");
		else {
			call(f, argv[1]);
		}
	} else if(!strcmp(argv[0], "DUMP")) {
		if(argc > 1)
			fprintf(f, "Too many arguements for command dump\n");
		else {
			dump_call();
		}
	} else if(!strcmp(argv[0], "DTMF")) {
		if(argc > 2)
		{
			fprintf(f, "Too many arguements for command dtmf\n");
			return;
		}
		if (argc < 1)
		{
			fprintf(f, "Too many arguements for command dtmf\n");
			return;
		}
		client_send_dtmf(*argv[1]);
	} else if(!strcmp(argv[0], "QUIT")) {
		if(argc > 1)
			fprintf(f, "Too many arguements for command quit\n");
		else {
			fprintf(f, "Good bye!\n");
			dump_call();
			exit(1);
		}
	} else fprintf(f, "Unknown command of %s\n", argv[0]);
}

void
issue_prompt(FILE *f)
{
	fprintf(f, "TeleClient> ");
	fflush(f);
}

void
dump_array(FILE *f, char **array) {
	while(*array)
		fprintf(f, "%s\n", *array++);
}
