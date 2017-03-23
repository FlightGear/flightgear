
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <xml.h>

char *
getCommandLineOption(int argc, char **argv, char const *option)
{
    int slen = strlen(option);
    char *rv = 0;
    int i;

    for (i=0; i<argc; i++)
    {
        if (strncmp(argv[i], option, slen) == 0)
        {
            rv = "";
            i++;
            if (i<argc) rv = argv[i];
        }
    }

    return rv;
}

void
show_help()
{
    printf("Usage: metdata [options]\n");
    printf("Updates the metadata of a FlightGear aircraft configuration.\n");
    printf("\nOptions:\n");
    printf("  -i, --input <file>\t\taircraft configuration file to update\n");

    printf("\n");
    exit(-1);
}

char *strlwr(char *str)
{
  unsigned char *p = (unsigned char *)str;

  while (*p) {
     *p = tolower((unsigned char)*p);
      p++;
  }

  return str;
}

/* -- JSBSim ---------------------------------------------------------------- */
const char*
jsb_wing_tag(void *xid)
{
    void *xmid = xmlNodeGet(xid, "/fdm_config/metrics");
    void *xlid = xmlMarkId(xmid);
    double aero_z = 0.0;
    double eye_z = 0.0;
    int i, num;

    num = xmlNodeGetNum(xmid, "location");
    for (i=0; i<num; ++i)
    {
        if (xmlNodeGetPos(xmid, xlid, "location", i) != 0)
        {
            if (!xmlAttributeCompareString(xlid, "name", "AERORP")) {
                aero_z = xmlNodeGetDouble(xlid, "z");
            } else if (!xmlAttributeCompareString(xlid, "name", "EYEPOINT")) {
                eye_z = xmlNodeGetDouble(xlid, "z");
            }
        }
    }
    xmlFree(xlid);
    xmlFree(xmid);

    if (aero_z > eye_z) return "high-wing";
    else return "low-wing";
}

const char*
jsb_gear_tag(void *xid)
{
    void *xgid = xmlNodeGet(xid, "/fdm_config/ground_reactions");
    void *xcid = xmlMarkId(xgid);
    double nose_x = 0.0;
    double main_x = 0.0;
    int bogeys = 0;
    int i, num;

    num = xmlNodeGetNum(xgid, "contact");
    for (i=0; i<num; ++i)
    {
        if (xmlNodeGetPos(xgid, xcid, "contact", i) != 0)
        {
            if (!xmlAttributeCompareString(xcid, "type", "BOGEY"))
            {
                if (xmlNodeGetDouble(xcid, "location/y") == 0.0) {
                    nose_x = xmlNodeGetDouble(xcid, "location/x");
                } else {
                    main_x = xmlNodeGetDouble(xcid, "location/x");
                }
                bogeys++;
            }
        }
    }
    xmlFree(xcid);
    xmlFree(xgid);
 
    if (bogeys < 3) return "skids";
    else if (main_x < nose_x) return "tail-dragger";
    return "tricycle";
}

const char*
jsb_gear_retract_tag(void *xid)
{
    int r = xmlNodeGetInt(xid, "/fdm_config/ground_reactions/contact/retractable");
    if (r) return "retractable-gear";
    else return "fixed-gear";
}

const char*
jsb_gear_steering_tag(void *xid)
{
    void *xgid = xmlNodeGet(xid, "/fdm_config/ground_reactions");
    void *xcid = xmlMarkId(xgid);
    char *rv = "no-steering";
    int found = 0;
    int i, num;

    num = xmlNodeGetNum(xgid, "contact");
    for (i=0; i<num; ++i)
    {
        if (xmlNodeGetPos(xgid, xcid, "contact", i) != 0)
        {
            if (!xmlAttributeCompareString(xcid, "type", "BOGEY"))
            {
                if (xmlNodeGetDouble(xcid, "location/y") == 0.0)
                {
                    double m = xmlNodeGetDouble(xcid, "max_steer");
                    int c = xmlNodeGetInt(xcid, "castered");

                    if (c || (m == 360.0 && !c)) {
                        rv = "castering-wheel";
                        break;
                    } else if (m > 0.0) {
                        rv = "normal-steering";
                        break;
                    }
                    else {
                        rv = "no-steering";
                    }
                }
            }
        }
    }
    xmlFree(xcid);
    xmlFree(xgid);

    return rv;
}

const char*
jsb_engines_tag(void *xid)
{
    void *xpid = xmlNodeGet(xid, "/fdm_config/propulsion");
    void *xeid = xmlMarkId(xpid);
    const char* rv = "multi-engine";
    int engines = 0;
    int i, num;
    char *main;

    num = xmlNodeGetNum(xpid, "engine");
    for (i=0; i<num; ++i)
    {
        if (xmlNodeGetPos(xpid, xeid, "engine", i) != 0)
        {
            if (i == 0)
            {
                main = xmlAttributeGetString(xeid, "file");
                engines++;
            }
            else if (!xmlAttributeCompareString(xeid, "file", main)) {
                engines++;
            }
        }
    }
    xmlFree(main);
    xmlFree(xeid);
    xmlFree(xpid);

    switch (engines)
    {
    case 0:
        rv = "glider";
        break;
    case 1:
        rv = "single-engine";
        break;
    case 2:
        rv = "twin-engine";
        break;
    case 3:
        rv = "three-engine";
        break;
    case 4:
        rv = "four-engine";
        break;
    default:
        break;
    }

    return rv;
}

const char*
jsb_engine_tag(void *xid, char *path)
{
    void *xeid = xmlNodeGet(xid, "/fdm_config/propulsion/engine");
    char *engine = xmlAttributeGetString(xeid, "file");
    char *fname = calloc(1, strlen(path)+strlen("Engines/")+
                            strlen(engine)+strlen(".xml")+1);
    const char *rv = "unknown";

    if (fname)
    {
        void *xfid;

        memcpy(fname, path, strlen(path));
        memcpy(fname+strlen(path), "Engines/", strlen("Engines/"));
        memcpy(fname+strlen(path)+strlen("Engines/"), engine, strlen(engine));
        memcpy(fname+strlen(path)+strlen("Engines/")+strlen(engine), ".xml", strlen(".xml"));

        xfid = xmlOpen(fname);
        if (xfid)
        {
            if (xmlNodeTest(xfid, "turbine_engine")) rv = "turbine";
            else if (xmlNodeTest(xfid, "turboprop_engine")) rv = "turboprop";
            else if (xmlNodeTest(xfid, "piston_engine")) rv = "piston";
            else if (xmlNodeTest(xfid, "electric_engine")) rv = "electric";
            else if (xmlNodeTest(xfid, "rocket_engine")) rv = "rocket";

            xmlClose(xfid);
        }
        else {
            printf("JSBSim engine file not found: %s\n", fname);
        }
        free(fname);
    }
    else {
        printf("Unable to allocate memory\n");
    }
    xmlFree(engine);
    xmlFree(xeid);

    return rv;
}

void
update_metadata_jsb(char *path, char *aero)
{
    char *fname;
    void *xid;

    fname = malloc(strlen(path)+strlen(aero)+strlen(".xml"));
    if (!fname)
    {
        printf("Unable to allocate memory\n");
        return;
    }

    memcpy(fname, path, strlen(path));
    memcpy(fname+strlen(path), aero, strlen(aero));
    memcpy(fname+strlen(path)+strlen(aero), ".xml", strlen(".xml"));

    xid = xmlOpen(fname);
    if (!xid)
    {
        printf("JSBSim aero file not found: '%s'\n", fname);
        return;
    }

    printf("    <tags>\n");
    printf("      <tag>%s</tag>\n", strlwr(aero));
    printf("      <tag>%s</tag>\n", jsb_wing_tag(xid));
    printf("      <tag>%s</tag>\n", jsb_gear_tag(xid));
    printf("      <tag>%s</tag>\n", jsb_gear_retract_tag(xid));
    printf("      <tag>%s</tag>\n", jsb_gear_steering_tag(xid));
    printf("      <tag>%s</tag>\n", jsb_engines_tag(xid));
    printf("      <tag>%s</tag>\n", jsb_engine_tag(xid, path));

    printf("    </tags>\n");

    xmlClose(xid);
    free(fname);
}

/* -- Yasim ----------------------------------------------------------------- */

void
update_metadata_yasim(char *path, char *aero)
{
    char *fname;
    void *xid;
    
    fname = malloc(strlen(path)+strlen(aero)+strlen(".xml"));
    if (!fname)
    {   
        printf("Unable to allocate memory\n");
        return;
    }

    memcpy(fname, path, strlen(path));
    memcpy(fname+strlen(path), aero, strlen(aero)); 
    memcpy(fname+strlen(path)+strlen(aero), ".xml", strlen(".xml"));
    
    xid = xmlOpen(fname);
    if (!xid)
    {   
        printf("Yasim aero file not found: '%s'\n", fname);
        return;
    }
    
    printf("    <tags>\n");
    printf("      <tag>%s</tag>\n", strlwr(aero));

    printf("    </tags>\n");
    
    xmlClose(xid);
    free(fname);
}

/* -------------------------------------------------------------------------- */

void
update_metadata(const char *fname)
{
    void *xsid, *xid = xmlOpen(fname);

    if (!xid)
    {
        printf("File not found: '%s'\n", fname);
        return;
    }

    xsid = xmlNodeGet(xid, "/PropertyList/sim");
    if (!xsid)
    {
        printf("path '/PropertyList/sim' not found in '%s'\n", fname);
        xmlClose(xid);
        return;
    }

//  if (xmlNodeTest(xsid, "tags") == 0)
if (1)
    {
        char *str = NULL;
        char *path, *pend;

        pend = strrchr(fname, '/');
        if (!pend) pend = strrchr(fname, '\\');
        if (!pend) path = strdup("./");
        else
        {
            pend++;
            path = calloc(1, pend-fname+1);
            memcpy(path, fname, pend-fname);
        }

        if (!xmlNodeCompareString(xsid, "flight-model", "jsb"))
        {
            str = xmlNodeGetString(xsid, "aero");
            update_metadata_jsb(path, str);
        }
        else if (!xmlNodeCompareString(xsid, "flight-model", "yasim"))
        {
            str = xmlNodeGetString(xsid, "aero");
            update_metadata_yasim(path, str);
        }
        else
        {
            str = xmlNodeGetString(xsid, "flight-model");
            printf("The '%s' flightmodel is unsupported at this time\n", str);
        }

        xmlFree(xsid);

        xmlFree(str);
        free(path);
    }
    else {
        printf("'%s' already contains metadata tags\n", fname);
    }

    xmlClose(xid);
}

int
main(int argc, char **argv)
{
    const char *setFile;

    if (argc == 1 || getCommandLineOption(argc, argv, "-h") ||
                     getCommandLineOption(argc, argv, "--help")) {
        show_help();
    }

    setFile = getCommandLineOption(argc, argv, "-i");
    if (!setFile) {
        setFile = getCommandLineOption(argc, argv, "-input");
    }
    if (!setFile) { 
        show_help();
    }

    update_metadata(setFile);
    return 0;
}
