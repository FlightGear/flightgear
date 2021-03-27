#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <map>

#include <simgear/io/SGDataDistributionService.hxx>

#include "dds_gui.h"
#include "dds_fdm.h"
#include "dds_ctrls.h"

/* An array of one message(aka sample in dds terms) will be used. */
#define MAX_SAMPLES 1

#ifndef _WIN32
# include <termios.h>

void set_mode(int want_key)
{
    static struct termios tios_old, tios_new;
    if (!want_key) {
        tcsetattr(STDIN_FILENO, TCSANOW, &tios_old);
        return;
    }

    tcgetattr(STDIN_FILENO, &tios_old);
    tios_new = tios_old;
    tios_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tios_new);
}

int get_key()
{
    int c = 0;
    struct timeval tv;
    fd_set fs;
    tv.tv_usec = tv.tv_sec = 0;

    FD_ZERO(&fs);
    FD_SET(STDIN_FILENO, &fs);
    select(STDIN_FILENO + 1, &fs, 0, 0, &tv);

    if (FD_ISSET(STDIN_FILENO, &fs)) {
        c = getchar();
    }
    return c;
}
#else
# include <conio.h>

int get_key()
{
   if (kbhit()) {
      return getch();
   }
   return 0;
}

void set_mode(int want_key)
{
}
#endif

int main()
{
  SG_DDS participant;
  std::map<std::string, SG_DDS_Topic *> topics;

  topics["gui"] = new SG_DDS_Topic();
  topics["fdm"] = new SG_DDS_Topic();
  topics["ctrls"] = new SG_DDS_Topic();

  topics["gui"]->setup("FG_DDS_GUI", &FG_DDS_GUI_desc, sizeof(FG_DDS_GUI));
  topics["fdm"]->setup("FG_DDS_FDM", &FG_DDS_FDM_desc, sizeof(FG_DDS_FDM));
  topics["ctrls"]->setup("FG_DDS_Ctrls", &FG_DDS_Ctrls_desc, sizeof(FG_DDS_Ctrls));

  participant.add(topics["gui"], SG_IO_IN);
  participant.add(topics["fdm"], SG_IO_IN);
  participant.add(topics["ctrls"], SG_IO_IN);

  set_mode(1);
  while(participant.wait(0.1f))
  {
    FG_DDS_Ctrls ctrls;
    FG_DDS_FDM fdm;
    FG_DDS_GUI gui;

    if (topics["gui"]->read(gui)) {
      printf("=== [fgfdm_log] Received : ");
      printf("GUI Message:\n");
      printf(" version: %i\n", gui.version);
      printf(" tuned_freq: %lf\n", gui.longitude);
      printf(" nav_radial: %lf\n", gui.latitude);
      printf("    dist_nm: %lf\n", gui.altitude);
    }

    if (topics["fdm"]->read(fdm)) {
      printf("=== [fgfdm_log] Received : ");
      printf("FDM Message:\n");
      printf(" version: %i\n", fdm.version);
      printf("  longitude: %lf\n", fdm.longitude);
      printf("   latitude: %lf\n", fdm.latitude);
      printf("   altitude: %lf\n", fdm.altitude);
    }

    if (topics["ctrls"]->read(ctrls)) {
      printf("=== [fgfdm_log] Received : ");
      printf("Ctrls Message:\n");
      printf(" version: %i\n", ctrls.version);
      printf("    aileron: %lf\n", ctrls.aileron);
      printf("   elevator: %lf\n", ctrls.elevator);
      printf("     rudder: %lf\n", ctrls.rudder);
    }

    fflush(stdout);

    if (get_key()) break;
  }
  set_mode(0);

  return EXIT_SUCCESS;
}
