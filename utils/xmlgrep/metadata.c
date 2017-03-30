
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <xml.h>

#define TS	(4*1024*1024)

#define PRINT(a) \
    tag = (a); if (tag) { strncat(s, "      <tag>", TS-strlen(s)); strncat(s, tag, TS-strlen(s)); strncat(s, "</tag>\n", TS-strlen(s)); }

static unsigned int tags_level = 0;
static unsigned int print_tags = 0;

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

void print_xml(FILE *fd, void *id, char *tags)
{
    static int level = 1;
    void *xid = xmlMarkId(id);
    unsigned int num;

    num = xmlNodeGetNum(xid, "*");
    if (num == 0)
    {
        char *s;
        s = xmlGetString(xid);
        if (s)
        {
            fprintf(fd, "%s", s);
            free(s);
        }
    }
    else
    {
        unsigned int i, q;
        for (i=0; i<num; i++)
        {
            if (xmlNodeGetPos(id, xid, "*", i) != 0)
            {
                unsigned int anum, a;
                char name[256];
                xmlNodeCopyName(xid, (char *)&name, 256);

                if (tags_level == 1 && !strcmp(name, "sim")) tags_level++;

                fprintf(fd, "\n");
                for(q=0; q<level; q++) fprintf(fd, "  ");
                fprintf(fd, "<%s", name);

                anum = xmlAttributeGetNum(xid);
                for (a=0; a<anum; a++)
                {
                    char name[256], attrib[256];
                    if (xmlAttributeCopyName(xid, (char *)&name, 256, a) != 0)
                    {
                        xmlAttributeCopyString(xid, name, (char *)&attrib, 256);
                        fprintf(fd, " %s=\"%s\"", name, attrib);
                    }
                }
                fprintf(fd, ">");

                level++;
                print_xml(fd, xid, tags);

                level--;
                fprintf(fd, "</%s>", name);

                if (tags_level == 2 && !strcmp(name, "aero")) {
                    fprintf(fd, "\n%s\n", tags);
                }
            }
            else printf("error\n");
        }
        fprintf(fd, "\n");
        for(q=1; q<level; q++) fprintf(fd, "  ");
    }
}

void
updateFile(char *fname, char *tags)
{
    char bfname[4096];
    FILE *fd = NULL;
    void *rid;

    snprintf(bfname, 4096, "%s.bak", fname);
    rename(fname, bfname);

    rid = xmlOpen(bfname);
    if (rid)
    {
        fd = fopen(fname, "w");
        if (!fd)
        {
            xmlClose(rid);
            rid = NULL;
            rename(bfname, fname);
        }
    }

    if (rid)
    {
        unsigned int i, num;
        void *xid;

        fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");

        xid = xmlMarkId(rid);
        num = xmlNodeGetNum(xid, "*");
        for (i=0; i<num; i++)
        {
            if (xmlNodeGetPos(rid, xid, "*", i) != 0)
            {
                unsigned int anum, a;
                char name[256];
                xmlNodeCopyName(xid, (char *)&name, 256);

                if (strcmp(name, "PropertyList")) break;

                tags_level++;
                fprintf(fd, "<%s", name);
                anum = xmlAttributeGetNum(xid);
                for (a=0; a<anum; a++)
                {
                    char name[256], attrib[256];
                    if (xmlAttributeCopyName(xid, (char *)&name, 256, a) != 0)
                    {
                        xmlAttributeCopyString(xid, name, (char *)&attrib, 256);
                        fprintf(fd, " %s=\"%s\"", name, attrib);
                    }
                }
                fprintf(fd, ">");
                print_xml(fd, xid, tags);
                fprintf(fd, "\n</%s>\n", name);
            }
        }
        free(xid);

        xmlClose(rid);
        fclose(fd);
    }
}

const char*
vendor_tag(char *desc)
{
    char *p = strlwr(desc);
    if (strstr(p, "boeing")) return "boeing";
    if (strstr(p, "airbus")) return "airbus";
    if (strstr(p, "antonov")) return "antonov";
    if (strstr(p, "tupolev")) return "tupolev";
    if (strstr(p, "ilyoushin")) return "ilyoushin";
    if (strstr(p, "bombardier")) return "bombardier";
    if (strstr(p, "fokker")) return "fokker";
    if (strstr(p, "lockheed")) return "lockheed";
    if (strstr(p, "general-dynamics")) return "general-dynamics";
    if (strstr(p, "general dynamics")) return "general-dynamics";
    if (strstr(p, "mig")) return "mikoyan-gurevich";
    if (strstr(p, "mikoyan gurevich")) return "mikoyan-gurevich";
    if (strstr(p, "mikoyan-gurevich")) return "mikoyan-gurevich";
    if (strstr(p, "sukoi")) return "sukoi";
    if (strstr(p, "cessna")) return "cessna";
    if (strstr(p, "fairchild")) return "fairchild";
    if (strstr(p, "dassult")) return "dassult";
    if (strstr(p, "dornier")) return "dornier";
    if (strstr(p, "arado")) return "arado";
    if (strstr(p, "schleicher")) return "schleicher";
    if (strstr(p, "avro")) return "avro";
    if (strstr(p, "saab")) return "saab";
    if (strstr(p, "dassault")) return "dassault";
    if (strstr(p, "aermacchi")) return "aermacchi";
    if (strstr(p, "arsenal")) return "arsenal";
    if (strstr(p, "rockwell")) return "rockwell";
    if (strstr(p, "northrop")) return "northrop";
    if (strstr(p, "grumman")) return "grumman";
    if (strstr(p, "mc donnell") && strstr(p, "douglas")) return "mc-donnell-douglas";
    if (strstr(p, "douglas")) return "douglas";
    if (strstr(p, "mc donnell")) return "mc-donnell";
    if (strstr(p, "mc-donnell")) return "mc-donnell";
//  if (strstr(p, "jaguar")) return "jaguar";
    if (strstr(p, "junkers")) return "junkers";
    if (strstr(p, "kawasaki")) return "kawasaki";
    if (strstr(p, "de havilland")) return "de-havilland";
    if (strstr(p, "de-havilland")) return "de-havilland";
    if (strstr(p, "diamond")) return "diamond";
    if (strstr(p, "bell")) return "bell";
    if (strstr(p, "hughes")) return "hughes";
    if (strstr(p, "kamov")) return "kamov";
    if (strstr(p, "mil")) return "mil";
    if (strstr(p, "eurocopter")) return "eurocopter";
    if (strstr(p, "alouette")) return "alouette";
    if (strstr(p, "aerospatiale")) return "aerospatiale";
    if (strstr(p, "sikorsky")) return "sikorsky";
    if (strstr(p, "bernard")) return "bernard";
    if (strstr(p, "bleriot")) return "bleriot";
    if (strstr(p, "bristol")) return "bristol";
    if (strstr(p, "breguet")) return "breguet";
    if (strstr(p, "wright")) return "wright";
    if (strstr(p, "breda")) return "breda";
    if (strstr(p, "rutan")) return "rutan";
    if (strstr(p, "vought")) return "vought";
    if (strstr(p, "fiat")) return "fiat";
    if (strstr(p, "focke wulf")) return "focke-wulf";
    if (strstr(p, "focke-wulf")) return "focke-wulf";
    if (strstr(p, "gloster")) return "gloster";
    if (strstr(p, "hawker")) return "hawker";
    if (strstr(p, "heinkel")) return "heinkel";
    if (strstr(p, "messerschmitt")) return "messerschmitt";
    if (strstr(p, "north american")) return "north-american";
    if (strstr(p, "north-american")) return "north-american";
    if (strstr(p, "piaggio")) return "piaggio";
    if (strstr(p, "pilatus")) return "pilatus";
    if (strstr(p, "supermarine")) return "supermarine";
    if (strstr(p, "beechcraft")) return "beechcraft";
    if (strstr(p, "beech")) return "beechcraft";
    if (strstr(p, "vickers")) return "vickers";
    if (strstr(p, "westland")) return "westland";
    if (strstr(p, "yakovlev")) return "yakovlev";
    if (strstr(p, "schweizer")) return "schweizer";

    /* educated guess */
    if (strstr(p, "b7")) return "boeing";
    if (strstr(p, "airbus")) return "airbus";
    if (strstr(p, "an-")) return "antonov";
    if (strstr(p, "tu-")) return "tupolev";
    if (strstr(p, "il-")) return "ilyoushin";
    if (strstr(p, "su-")) return "sukoi";
    if (strstr(p, "dc-")) return "douglas";
    if (strstr(p, "md-")) return "mc-donnell-douglas";
    if (strstr(p, "ju-")) return "junkers";
    if (strstr(p, "ec-")) return "eurocopter";
    if (strstr(p, "he-")) return "heinkel";
    if (strstr(p, "as-")) return "aerospatiale";
    if (strstr(p, "me-")) return "messerschmitt";
    if (strstr(p, "fw-")) return "focke-wulf";
    if (strstr(p, "yak-")) return "yakovlev";
    if (strstr(p, "sgs-")) return "schweizer";
    if (strstr(p, "ask-")) return "schleicher";
    if (strstr(p, "mirage")) return "dassault";

    return NULL;
}

/* -- JSBSim ---------------------------------------------------------------- */
const char*
jsb_wing_tag(void *xid, void *path)
{
    void *xlid, *xmid, *xeid;
    double aero_z = 0.0;
    double eye_z = 0.0;
    int i, num;

    xeid = xmlNodeGet(xid, "/fdm_config/propulsion/engine/thruster");
    if (xeid)
    {
        char *rv = NULL;
        char *file = xmlAttributeGetString(xeid, "file");
        char *fname = calloc(1, strlen(path)+strlen("Engines/")+
                                strlen(file)+strlen(".xml")+1);
        if (fname)
        {
            void *xfid;

            memcpy(fname, path, strlen(path));
            memcpy(fname+strlen(path), "Engines/", strlen("Engines/"));
            memcpy(fname+strlen(path)+strlen("Engines/"), file, strlen(file));
            memcpy(fname+strlen(path)+strlen("Engines/")+strlen(file), ".xml", strlen(".xml"));

            xfid = xmlOpen(fname);
            if (xfid)
            {
                if (xmlNodeTest(xfid, "rotor")) rv = "helicopter";
                xmlClose(xfid);
            }
            free(fname);
        }
        xmlFree(file);
        xmlFree(xeid);
        if (rv) return rv;
    }

    xmid = xmlNodeGet(xid, "/fdm_config/metrics");
    if (!xmid) return NULL;

    xlid = xmlMarkId(xmid);
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
    void *xcid, *xgid = xmlNodeGet(xid, "/fdm_config/ground_reactions");
    double nose_x = 0.0;
    double main_x = 0.0;
    int bogeys = 0;
    int i, num;

    if (!xgid) return NULL;

    xcid = xmlMarkId(xgid);
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
 
    if (bogeys == 0) return "skids";
    else if (bogeys < 3 || main_x < nose_x) return "tail-dragger";
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
    void *xcid, *xgid = xmlNodeGet(xid, "/fdm_config/ground_reactions");
    char *rv = NULL;
    int found = 0;
    int i, num;

    if (!xgid) return NULL;

    xcid = xmlMarkId(xgid);
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
                        rv = "castering";
                        break;
                    } else if (m > 0.0) {
//                      rv = "normal-steering";
                        break;
                    }
                    else {
//                      rv = "no-steering";
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
    void *xeid, *xpid = xmlNodeGet(xid, "/fdm_config/propulsion");
    const char* rv = NULL;
    int engines = 0;
    int i, num;
    char *main;

    if (!xpid) return rv;

    xeid = xmlMarkId(xpid);

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
    char *engine, *fname;
    const char *rv = NULL;

    if (!xeid) return rv;

    engine = xmlAttributeGetString(xeid, "file");
    fname = calloc(1, strlen(path)+strlen("Engines/")+
                      strlen(engine)+strlen(".xml")+1);
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
            if (xmlNodeTest(xfid, "turbine_engine")) rv = "jet";
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

const char*
jsb_propulsion_tag(void *xid, char *path)
{
    void *xeid = xmlNodeGet(xid, "fdm_config/propulsion/engine");
    char *file, *fname;
    const char *rv = NULL;

    if (!xeid) return rv;

    file = xmlAttributeGetString(xeid, "file");
    fname = calloc(1, strlen(path)+strlen("Engines/")+
                      strlen(file)+strlen(".xml")+1);
    if (fname)
    {
        void *xfid;

        memcpy(fname, path, strlen(path));
        memcpy(fname+strlen(path), "Engines/", strlen("Engines/"));
        memcpy(fname+strlen(path)+strlen("Engines/"), file, strlen(file));
        memcpy(fname+strlen(path)+strlen("Engines/")+strlen(file), ".xml", strlen(".xml"));

        xfid = xmlOpen(fname);
        if (xfid)
        {
            if (xmlNodeGetInt(xfid, "turbine_engine/augmented")) {
                rv = "afterburner";
            }
            else if (xmlNodeGetInt(xfid, "piston_engine/numboostspeeds")) {
                rv = "supercharged";
            }
            xmlClose(xfid);
        }
        else {
            printf("JSBSim engine file not found: %s\n", fname);
        }
        free(fname);
    }
    xmlFree(xeid);
    xmlFree(file);

    return rv;
}

const char*
jsb_thruster_tag(void *xid, char *path)
{
    void *xeid = xmlNodeGet(xid, "/fdm_config/propulsion/engine/thruster");
    char *file, *fname;
    const char *rv = NULL;

    if (!xeid) return rv;

    file = xmlAttributeGetString(xeid, "file");
    fname = calloc(1, strlen(path)+strlen("Engines/")+
                      strlen(file)+strlen(".xml")+1);
    if (fname)
    {
        void *xfid;

        memcpy(fname, path, strlen(path));
        memcpy(fname+strlen(path), "Engines/", strlen("Engines/"));
        memcpy(fname+strlen(path)+strlen("Engines/"), file, strlen(file));
        memcpy(fname+strlen(path)+strlen("Engines/")+strlen(file), ".xml", strlen(".xml"));

        xfid = xmlOpen(fname);
        if (xfid)
        {
            if (xmlNodeTest(xfid, "propeller"))
            {
                double min = xmlNodeGetDouble(xfid, "propeller/minpitch");
                double max = xmlNodeGetDouble(xfid, "propeller/maxpitch");
                if (min == max) rv = "fixed-pitch";
                else rv = "variable-pitch";
            }
            xmlClose(xfid);
        }
        else {
            printf("JSBSim thruster file not found: %s\n", fname);
        }
        free(fname);
    }
    else {
        printf("Unable to allocate memory\n");
    }
    xmlFree(file);
    xmlFree(xeid);

    return rv;
}

void
update_metadata_jsb(char *sfname, char *path, char *aero, char *desc)
{
    const char *tag;
    char *s, *fname;
    void *xid;

    fname = calloc(1, strlen(path)+strlen(aero)+strlen(".xml")+1);
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

    s = malloc(65*1024);
    if (s)
    {
        sprintf(s, "    <tags>\n");
        PRINT("auto-generated");
        PRINT(vendor_tag(desc ? desc : aero));
        PRINT(strlwr(desc ? desc : aero));
        PRINT(jsb_wing_tag(xid, path));
        PRINT(jsb_gear_tag(xid));
        PRINT(jsb_gear_retract_tag(xid));
        PRINT(jsb_gear_steering_tag(xid));
        PRINT(jsb_engines_tag(xid));
        PRINT(jsb_engine_tag(xid, path));
        PRINT(jsb_propulsion_tag(xid, path));
        PRINT(jsb_thruster_tag(xid, path));
        s = strncat(s, "    </tags>", TS-strlen(s));

        if (print_tags) {
           printf( "%s\n", s);
        } else {
           updateFile(sfname, s);
        }
        free(s);
    }
    xmlClose(xid);
    free(fname);
}

/* -- Yasim ----------------------------------------------------------------- */

const char*
yasim_wing_tag(void *xid)
{
    void *xaid = xmlNodeGet(xid, "/airplane");
    void *xwid, *xcid;
    int n_wings;
    double wing_z = 0.0;
    double eye_z = 0.0;

    if (xmlNodeTest(xaid, "rotor")) {
        return "helicopter";
    }

    xwid = xmlNodeGet(xaid, "wing");
    if (xwid)
    {
        wing_z = xmlAttributeGetDouble(xwid, "z");
        xmlFree(xwid);
    }

    xcid = xmlNodeGet(xaid, "cockpit");
    if (xcid)
    {
       eye_z = xmlAttributeGetDouble(xcid, "z");
        xmlFree(xcid);
    }

    n_wings = xmlNodeGetNum(xaid, "mstab");
    xmlFree(xaid);

//  if (n_wings == 2) return "triplane";
//  if (n_wings == 1) return "biplane";

    if (wing_z > eye_z) return "high-wing";
    return "low-wing";
}

const char*
yasim_gear_tag(void *xid)
{
    void *xaid = xmlNodeGet(xid, "/airplane");
    void *xgid = xmlMarkId(xaid);
    double nose_x = 0.0;
    double main_x = 0.0;
    int gears = 0;
    int i, num;

    num = xmlNodeGetNum(xaid, "gear");
    for (i=0; i<num; ++i)
    {
        if (xmlNodeGetPos(xaid, xgid, "gear", i) != 0)
        {
             if (xmlAttributeGetDouble(xgid, "y") == 0.0) {
                 nose_x = xmlAttributeGetDouble(xgid, "x");
             } else {
                 main_x = xmlAttributeGetDouble(xgid, "x");
             }
             gears++;
        }
    }
    xmlFree(xgid);
    xmlFree(xaid);

    if (gears == 0) return "skids";
    else if (gears < 3 || main_x > nose_x) return "tail-dragger";
    return "tricycle";
}

const char*
yasim_gear_retract_tag(void *xid)
{
    void *xaid = xmlNodeGet(xid, "/airplane");
    void *xgid = xmlMarkId(xaid);
    char *rv = "fixed-gear";
    int found = 0;
    int i, num;

    num = xmlNodeGetNum(xaid, "gear");
    for (i=0; i<num; ++i)
    {
        if (xmlNodeGetPos(xaid, xgid, "gear", i) != 0)
        {
            void *xgcid = xmlMarkId(xgid);
            int j, cnum = xmlNodeGetNum(xgid, "control-input");
            for (j=0; j<cnum; ++j)
            {
                if (xmlNodeGetPos(xgid, xgcid, "control-input", j))
                {
                    if (!xmlAttributeCompareString(xgcid, "control", "EXTEND")) {
                        rv = "retractable-gear";
                        found = 1;
                        break;
                    }
               } 
            }
            xmlFree(xgcid);
        }
    }
    xmlFree(xgid);
    xmlFree(xaid);
    
    return rv;
}

const char*
yasim_gear_steering_tag(void *xid)
{
    void *xaid = xmlNodeGet(xid, "/airplane");
    void *xgid = xmlMarkId(xaid);
    char *rv = NULL;
    int found = 0;
    int i, num;

    num = xmlNodeGetNum(xaid, "gear");
    for (i=0; i<num; ++i)
    {
        if (xmlNodeGetPos(xaid, xgid, "gear", i) != 0)
        {
            void *xgcid = xmlMarkId(xgid);
            int j, cnum = xmlNodeGetNum(xgid, "control-input");
            for (j=0; j<cnum; ++j)
            {
                if (xmlNodeGetPos(xgid, xgcid, "control-input", j))
                {
                    if (!xmlAttributeCompareString(xgcid, "control", "STEER")) {
//                      rv = "normal-steering";
                        found = 1;
                        break;
                    }
                    else if (!xmlAttributeCompareString(xgcid, "control", "CASTERING")) {
                        rv = "castering";
                        found = 1;
                        break;
                   }
               }
            }
	    xmlFree(xgcid);
        }
    }
    xmlFree(xgid);
    xmlFree(xaid);

    return rv;
}

const char*
yasim_engines_tag(void *xid)
{
    void *xeid = xmlNodeGet(xid,  "/airplane");
    const char* rv = "multi-engine";
    int engines = 0;

    if (xmlNodeTest(xeid, "propeller")) {
        engines = xmlNodeGetNum(xeid, "propeller");
    }
    else if (xmlNodeTest(xeid, "jet")) {
        engines = xmlNodeGetNum(xeid, "jet");
    }
    else if (xmlNodeTest(xeid, "rotor")) {
        engines = 1;
    }

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
    xmlFree(xeid);

    return rv;
}

const char*
yasim_engine_tag(void *xid, char *path)
{
    const char* rv = NULL;

    if (xmlNodeTest(xid, "/airplane/propeller/piston-engine")) {
        rv = "piston";
    }
    else if (xmlNodeTest(xid, "/airplane/propeller/turbine-engine")) {
        rv = "turboprop";
    }
    else if (xmlNodeTest(xid, "/airplane/jet")) {
        rv = "jet";
    }

    return rv;
}

const char*
yasim_propulsion_tag(void *xid, char *path)
{
    const char* rv = NULL;
    void *xpid;

    if ((xpid = xmlNodeGet(xid, "/airplane/jet")) != NULL)
    {
        if (xmlAttributeGetDouble(xpid, "afterburner") > 0.0) {
            rv = "afterburner";
        }
        xmlFree(xpid);
    }
    else if ((xpid = xmlNodeGet(xid, "/airplane/propeller/piston-engine"))  != NULL)
    {
        if (xmlAttributeGetInt(xpid, "supercharger") > 0) {
            rv = "supercharger";
        }
        xmlFree(xpid);
    }

    return rv;
}

const char*
yasim_thruster_tag(void *xid, char *path)
{
    const char* rv = NULL;
    void *xtid;

    xtid = xmlNodeGet(xid, "/airplane/propeller/control-input");
    if (xtid)
    {
        if (!xmlAttributeCompareString(xtid, "control", "ADVANCE")) {
            rv = "variable-pitch";
        } else rv = "fixed-pitch";
        xmlFree(xtid);
    }
    
    return rv;
}

void
update_metadata_yasim(char *sfname, char *path, char *aero, char *desc)
{
    const char *tag;
    char *s, *fname;
    void *xid;
    
    fname = calloc(1, strlen(path)+strlen(aero)+strlen(".xml")+1);
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
    
    s = malloc(65*1024);
    if (s)
    {
        sprintf(s, "    <tags>\n");
        PRINT("auto-generated");
        PRINT(vendor_tag(desc ? desc : aero));
        PRINT(strlwr(desc ? desc : aero));
        PRINT(yasim_wing_tag(xid));
        PRINT(yasim_gear_tag(xid));
        PRINT(yasim_gear_retract_tag(xid));
        PRINT(yasim_gear_steering_tag(xid));
        PRINT(yasim_engines_tag(xid));
        PRINT(yasim_engine_tag(xid, path));
        PRINT(yasim_propulsion_tag(xid, path));
        PRINT(yasim_thruster_tag(xid, path));
        s = strncat(s, "    </tags>", TS-strlen(s));

        if (print_tags) {
           printf( "%s\n", s);
        } else {
           updateFile(sfname, s);
        }
        free(s);
    }
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

    if (xmlNodeTest(xsid, "tags") == 0)
    {
        char *desc = NULL, *str = NULL;
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

        desc = xmlNodeGetString(xsid, "description");
        str = xmlNodeGetString(xsid, "aero");
        if (!xmlNodeCompareString(xsid, "flight-model", "jsb")) {
            update_metadata_jsb(fname, path, str, desc);
        }
        else if (!xmlNodeCompareString(xsid, "flight-model", "yasim")) {
            update_metadata_yasim(fname, path, str, desc);
        }
        else
        {
            str = xmlNodeGetString(xsid, "flight-model");
            printf("The '%s' flightmodel is unsupported at this time\n", str);
        }

        xmlFree(xsid);

        xmlFree(desc);
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

    if (getCommandLineOption(argc, argv, "-p") ||
        getCommandLineOption(argc, argv, "--print")) {
        print_tags = 1;
    }

    setFile = getCommandLineOption(argc, argv, "-i");
    if (!setFile) {
        setFile = getCommandLineOption(argc, argv, "-input");
    }
    if (!setFile) { 
        show_help();
    }

    printf("%s\n", setFile);
    update_metadata(setFile);
    printf("\n");
    return 0;
}
