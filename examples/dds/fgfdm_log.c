#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dds/dds.h>

#include <Network/DDS/dds_gui.h>
#include <Network/DDS/dds_fdm.h>
#include <Network/DDS/dds_ctrls.h>


/* An array of one message(aka sample in dds terms) will be used. */
#define MAX_SAMPLES 1

#ifndef _WIN32
# include <termios.h>

void set_mode(int want_key)
{
    static struct termios old, new;
    if (!want_key) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        return;
    }

    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
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
  dds_entity_t participant;
  dds_entity_t gui_topic;
  dds_entity_t fdm_topic;
  dds_entity_t ctrls_topic;
  dds_entity_t gui_reader;
  dds_entity_t fdm_reader;
  dds_entity_t ctrls_reader;
  dds_entity_t rdcond;
  void *samples[MAX_SAMPLES];
  dds_sample_info_t infos[MAX_SAMPLES];
  dds_return_t rc;
  dds_qos_t *qos;
  int status;

  /* Initialize sample buffer, by pointing the void pointer within
   * the buffer array to a valid sample memory location. */
  samples[0] = FG_DDS_FDM__alloc ();

  /* Create a Participant. */
  participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
  if(participant < 0)
    DDS_FATAL("dds_create_participant: %s\n", dds_strretcode(-participant));

  /* Create a wait set */
  dds_entity_t waitset = dds_create_waitset(participant);

  qos = dds_create_qos();
  dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(10));

  /* Create a GUI Topic. */
  gui_topic = dds_create_topic(
    participant, &FG_DDS_GUI_desc, "FG_DDS_GUI", NULL, NULL);
  if(gui_topic < 0)
    DDS_FATAL("dds_create_topic: %s\n", dds_strretcode(-gui_topic));

  gui_reader = dds_create_reader(participant, gui_topic, qos, NULL);
  if(gui_reader < 0)
    DDS_FATAL("dds_create_reader: %s\n", dds_strretcode(-gui_reader));

  rdcond = dds_create_readcondition(gui_reader, DDS_NOT_READ_SAMPLE_STATE);
  status = dds_waitset_attach(waitset, rdcond, gui_reader);
  if (status < 0)
      DDS_FATAL("ds_waitset_attach: %s\n", dds_strretcode(-status));

  /* Create an FDM Topic. */
  fdm_topic = dds_create_topic(
    participant, &FG_DDS_FDM_desc, "FG_DDS_FDM", NULL, NULL);
  if(fdm_topic < 0)
    DDS_FATAL("dds_create_topic: %s\n", dds_strretcode(-fdm_topic));

  fdm_reader = dds_create_reader(participant, fdm_topic, qos, NULL);
  if(fdm_reader < 0)
    DDS_FATAL("dds_create_reader: %s\n", dds_strretcode(-fdm_reader));

  rdcond = dds_create_readcondition(fdm_reader, DDS_NOT_READ_SAMPLE_STATE);
  status = dds_waitset_attach(waitset, rdcond, fdm_reader);
  if (status < 0)
      DDS_FATAL("ds_waitset_attach: %s\n", dds_strretcode(-status));

  /* Create a Ctrls Topic. */
  ctrls_topic = dds_create_topic(
    participant, &FG_DDS_Ctrls_desc, "FG_DDS_Ctrls", NULL, NULL);
  if(ctrls_topic < 0)
    DDS_FATAL("dds_create_topic: %s\n", dds_strretcode(-ctrls_topic));

  ctrls_reader = dds_create_reader(participant, ctrls_topic, qos, NULL);
  if(ctrls_reader < 0)
    DDS_FATAL("dds_create_reader: %s\n", dds_strretcode(-ctrls_reader));

  rdcond = dds_create_readcondition(ctrls_reader, DDS_NOT_READ_SAMPLE_STATE);
  status = dds_waitset_attach(waitset, rdcond, ctrls_reader);
  if (status < 0)
      DDS_FATAL("ds_waitset_attach: %s\n", dds_strretcode(-status));

  dds_delete_qos(qos);

  printf("\n=== [fgfdm_log] Waiting for a sample ...\n");
  fflush(stdout);

  /* Wait for a new packet. */
  set_mode(1);

  size_t num = 1;
  dds_attach_t results[num];
  dds_duration_t timeout = DDS_MSECS(500);
  while(dds_waitset_wait(waitset, results, num, timeout) >= 0)
  {
    /* Do the actual read.
     * The return value contains the number of read samples. */
    rc = dds_take(gui_reader, samples, infos, 1, 1);
    if(rc < 0)
      DDS_FATAL("dds_read: (gui) %s\n", dds_strretcode(-rc));

    if((rc > 0) &&(infos[0].valid_data))
    {
      FG_DDS_GUI *gui = (FG_DDS_GUI*) samples[0];
      printf("=== [fgfdm_log] Received : ");
      printf("GUI Message:\n");
      printf(" version: %i\n", gui->version);
      printf(" tuned_freq: %lf\n", gui->longitude);
      printf(" nav_radial: %lf\n", gui->latitude);
      printf("    dist_nm: %lf\n", gui->altitude);
    }

    rc = dds_take(fdm_reader, samples, infos, 1, 1);
    if(rc < 0)
      DDS_FATAL("dds_read: (fdm) %s\n", dds_strretcode(-rc));

    /* Check if we read some data and it is valid. */
    if((rc > 0) &&(infos[0].valid_data))
    {
      /* Print Message. */
      FG_DDS_FDM *fdm = (FG_DDS_FDM*) samples[0];
      printf("=== [fgfdm_log] Received : ");
      printf("FDM Message:\n");
      printf(" version: %i\n", fdm->version);
      printf("  longitude: %lf\n", fdm->longitude);
      printf("   latitude: %lf\n", fdm->latitude);
      printf("   altitude: %lf\n", fdm->altitude);
    }

    rc = dds_take(ctrls_reader, samples, infos, 1, 1);
    if(rc < 0)
      DDS_FATAL("dds_read: (ctrls) %s\n", dds_strretcode(-rc));

    /* Check if we read some data and it is valid. */
    if((rc > 0) &&(infos[0].valid_data))
    {
      /* Print Message. */
      FG_DDS_Ctrls *ctrls = (FG_DDS_Ctrls*) samples[0];
      printf("=== [fgfdm_log] Received : ");
      printf("Ctrls Message:\n");
      printf(" version: %i\n", ctrls->version);
      printf("    aileron: %lf\n", ctrls->aileron);
      printf("   elevator: %lf\n", ctrls->elevator);
      printf("     rudder: %lf\n", ctrls->rudder);
    }

    fflush(stdout);

    if (get_key()) break;
  }
  set_mode(0);

  /* Free the data location. */
  FG_DDS_FDM_free (samples[0], DDS_FREE_ALL);

  /* Deleting the participant will delete all its children recursively as well. */
  rc = dds_delete(participant);
  if(rc != DDS_RETCODE_OK)
    DDS_FATAL("dds_delete: %s\n", dds_strretcode(-rc));

  return EXIT_SUCCESS;
}
