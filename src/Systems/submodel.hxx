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
#include <AIModel/AIManager.hxx>
#include <vector>
#include <string>
SG_USING_STD(vector);
SG_USING_STD(string);


class SubmodelSystem : public SGSubsystem
{

public:

 typedef struct {
  SGPropertyNode_ptr trigger;
  SGPropertyNode_ptr prop;
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
 } submodel; 

 typedef struct {
  double     lat;
  double     lon;
  double     alt;
  double     azimuth;
  double     elevation;
  double     speed;
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

private:

    typedef vector <submodel*> submodel_vector_type;
    typedef submodel_vector_type::iterator submodel_vector_iterator;

    submodel_vector_type       submodels;
    submodel_vector_iterator   submodel_iterator;


    double x_offset, y_offset, z_offset;
    double pitch_offset, yaw_offset;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _user_lat_node;
    SGPropertyNode_ptr _user_lon_node;
    SGPropertyNode_ptr _user_heading_node;
    SGPropertyNode_ptr _user_alt_node;
    SGPropertyNode_ptr _user_pitch_node;
    SGPropertyNode_ptr _user_roll_node;
    SGPropertyNode_ptr _user_yaw_node;
    SGPropertyNode_ptr _user_speed_node;

    FGAIManager* ai;
    IC_struct  IC;
};

#endif // __SYSTEMS_SUBMODEL_HXX
