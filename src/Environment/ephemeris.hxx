#ifndef FG_ENVIRONMENT_EPHEMERIS_HXX
#define FG_ENVIRONMENT_EPHEMERIS_HXX

#include <simgear/structure/subsystem_mgr.hxx>

class SGEphemeris;
class SGPropertyNode;

/**
 * Wrap SGEphemeris in a susbsytem/property interface
 */
class Ephemeris : public SGSubsystem
{
public:

	Ephemeris();
	~Ephemeris();		
	
	virtual void bind();
	virtual void unbind();
	virtual void update(double dt);
	virtual void init();
  virtual void postinit();
  
private:
  SGEphemeris* _impl;
  SGPropertyNode* _latProp;
};

#endif // of FG_ENVIRONMENT_EPHEMERIS_HXX

