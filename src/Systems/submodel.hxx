// submodel.hxx - models a releasable submodel.
// Written by Dave Culp, started Aug 2004
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEMS_SUBMODEL_HXX
#define __SYSTEMS_SUBMODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <AIModel/AIBase.hxx>
#include <vector>
#include <string>
SG_USING_STD(vector);
SG_USING_STD(string);


class SubmodelSystem : public SGSubsystem
{

public:


 typedef struct {
  SGPropertyNode* trigger;
  SGPropertyNode* prop;
  string             name;
  string             model;
  double             speed;
  bool               slaved;
  bool               repeat;
  double             delay;
  double             timer;
  int                count;
  double             x_offset;
  double             y_offset;
  double             z_offset;
  double             yaw_offset;
  double             pitch_offset;
  double             drag_area; 
  double             life;
  double             buoyancy;
  bool               wind;
    
 } submodel; 

 typedef struct {
  double     lat;
  double     lon;
  double     alt;
  double     roll;
  double     azimuth;
  double     elevation;
  double     speed;
  double     wind_from_east;
  double     wind_from_north;
  
 } IC_struct;  

    SubmodelSystem ();
    ~SubmodelSystem ();

    void load ();
    void init ();
    void bind ();
    void unbind ();
    void update (double dt);
    bool release (submodel* sm, double dt);
    void transform (submodel* sm);
    void updatelat( double lat );

private:

    typedef vector <submodel*> submodel_vector_type;
    typedef submodel_vector_type::iterator submodel_vector_iterator;

    submodel_vector_type       submodels;
    submodel_vector_iterator   submodel_iterator;

    float trans[3][3];
    float in[3];
    float out[3];

    double Rx, Ry, Rz;
    double Sx, Sy, Sz;
    double Tx, Ty, Tz;

    float cosRx, sinRx;
    float cosRy, sinRy;
    float cosRz, sinRz;

    double ft_per_deg_longitude;
    double ft_per_deg_latitude;

    double x_offset, y_offset, z_offset;
    double pitch_offset, yaw_offset;

    SGPropertyNode* _serviceable_node;
    SGPropertyNode* _user_lat_node;
    SGPropertyNode* _user_lon_node;
    SGPropertyNode* _user_heading_node;
    SGPropertyNode* _user_alt_node;
    SGPropertyNode* _user_pitch_node;
    SGPropertyNode* _user_roll_node;
    SGPropertyNode* _user_yaw_node;
    SGPropertyNode* _user_speed_node;
    SGPropertyNode* _user_wind_from_east_node;
    SGPropertyNode* _user_wind_from_north_node;
    FGAIManager* ai;
    IC_struct  IC;
};

#endif // __SYSTEMS_SUBMODEL_HXX
