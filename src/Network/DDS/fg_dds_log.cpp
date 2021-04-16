#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <map>

#include <simgear/io/SGDataDistributionService.hxx>

#include "dds_fwd.hxx"

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
  std::map<std::string, SG_DDS_Topic*> topics;
  SG_DDS participant;

  FG_DDS_GUI gui;
  topics["gui"] = new SG_DDS_Topic(gui, &FG_DDS_GUI_desc);
  participant.add(topics["gui"], SG_IO_IN);

  FG_DDS_FDM fdm;
  topics["fdm"] = new SG_DDS_Topic(fdm, &FG_DDS_FDM_desc);
  participant.add(topics["fdm"], SG_IO_IN);

  FG_DDS_Ctrls ctrls;
  topics["ctrls"] = new SG_DDS_Topic(ctrls, &FG_DDS_Ctrls_desc);
  participant.add(topics["ctrls"], SG_IO_IN);

  FG_DDS_prop prop;
  topics["prop"] = new SG_DDS_Topic(prop, &FG_DDS_prop_desc);
  participant.add(topics["prop"], SG_IO_IN);

  set_mode(1);
  while(participant.wait(0.1f))
  {
    if (topics["gui"]->read()) {
      printf("=== [fg_dds_log] Received : ");
      printf("GUI Message:\n");
      printf(" version: %i\n", gui.version);
      printf(" tuned_freq: %lf\n", gui.longitude);
      printf(" nav_radial: %lf\n", gui.latitude);
      printf("    dist_nm: %lf\n", gui.altitude);
    }

    if (topics["fdm"]->read()) {
      printf("=== [fg_dds_log] Received : ");
      printf("FDM Message:\n");
      printf(" version: %i\n", fdm.version);
      printf("  longitude: %lf\n", fdm.longitude);
      printf("   latitude: %lf\n", fdm.latitude);
      printf("   altitude: %lf\n", fdm.altitude);
    }

    if (topics["ctrls"]->read()) {
      printf("=== [fg_dds_log] Received : ");
      printf("Ctrls Message:\n");
      printf(" version: %i\n", ctrls.version);
      printf("    aileron: %lf\n", ctrls.aileron);
      printf("   elevator: %lf\n", ctrls.elevator);
      printf("     rudder: %lf\n", ctrls.rudder);
    }

    if (topics["prop"]->read() &&
         prop.version == FG_DDS_PROP_VERSION )
    {
      printf("=== [fg_dds_log] Received : ");
      printf("Prop Message:\n");
      printf(" version: %i\n", prop.version);
      printf(" mode: %s\n", prop.mode ? "write" : "read");
      printf("   id: %i\n", prop.id);
      if (prop.id == FG_DDS_PROP_REQUEST) {
        printf(" path: %s\n", prop.val._u.String);
        printf("GUID: ");
        for(int i=0; i<16; ++i)
          printf("%X ", prop.guid[i]);
        printf("\n");
      }
      else
      {
        switch(prop.val._d)
        {
        case FG_DDS_BOOL:
          printf("       type: bool");
          printf("      value: %i\n", prop.val._u.Bool);
          break;
        case FG_DDS_INT:
          printf("       type: int");
          printf("      value: %i\n", prop.val._u.Int32);
          break;
        case FG_DDS_LONG:
          printf("       type: long");
          printf("      value: %li\n", prop.val._u.Int64);
          break;
        case FG_DDS_FLOAT:
          printf("       type: float");
          printf("      value: %f\n", prop.val._u.Float32);
          break;
        case FG_DDS_DOUBLE:
          printf("       type: double");
          printf("      value: %lf\n", prop.val._u.Float64);
          break;
        case FG_DDS_STRING:
          printf("       type: string");
          printf("      value: %s\n", prop.val._u.String);
          break;
        default:
          break;
        }
      }
    }

    fflush(stdout);

    if (get_key()) break;
  }
  set_mode(0);

  return EXIT_SUCCESS;
}
