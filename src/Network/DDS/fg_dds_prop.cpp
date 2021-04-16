#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <simgear/io/SGDataDistributionService.hxx>

#include "dds_props.h"

/* An array of one message(aka sample in dds terms) will be used. */
#define MAX_SAMPLES 1

int main()
{
  SG_DDS participant;

  FG_DDS_prop prop;
  SG_DDS_Topic *topic = new SG_DDS_Topic(prop, &FG_DDS_prop_desc);

  participant.add(topic, SG_IO_BI);

  dds_guid_t guid = topic->get_guid();
  memcpy(prop.guid, guid.v, 16);
  printf("GUID: ");
  for(int i=0; i<16; ++i)
    printf("%X ", prop.guid[i]);
  printf("\n");

  char path[256];
  printf("\nType 'q' to quit\n");
  do
  {
    printf("Property path or id: ");
    int len = scanf("%255s", path);
    if (len == EOF) continue;

    if (*path == 'q') break;

    char *end;
    int id = strtol(path, &end, 10);

    prop.id = (end == path) ? FG_DDS_PROP_REQUEST : id;
    prop.version = FG_DDS_PROP_VERSION;
    prop.mode = FG_DDS_MODE_READ;
    prop.val._d = FG_DDS_STRING;
    prop.val._u.String = path;

    topic->write();

    participant.wait();
    if (topic->read() &&
         prop.version == FG_DDS_PROP_VERSION &&
         prop.id != FG_DDS_PROP_REQUEST)
    {
      printf("\nReceived:\n");
      switch(prop.val._d)
      {
      case FG_DDS_NONE:
        printf("       type: none");
        break;
      case FG_DDS_ALIAS:
        printf("       type: alias");
        printf("      value: %s\n", prop.val._u.String);
        break;
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
      case FG_DDS_UNSPECIFIED:
        printf("       type: unspecified");
        printf("      value: %s\n", prop.val._u.String);
        break;
      default:
        break;
      }
    }
  } while(true);

  return EXIT_SUCCESS;
}
