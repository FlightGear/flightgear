#ifndef FG_FDM_SHELL_HXX
#define FG_FDM_SHELL_HXX

#include <simgear/structure/subsystem_mgr.hxx>

// forward decls
class FGInterface;

/**
 * Wrap an FDM implementation in a subsystem with standard semantics
 * Notably, deal with the various cases in which update() should not
 * be called, such as replay or before scenery has loaded
 *
 * This class also provides the factory method which creates the
 * specific FDM class (createImplementation)
 */
class FDMShell : public SGSubsystem
{
public:
  FDMShell();
  ~FDMShell();
  
  virtual void init();
  virtual void reinit();
  
  virtual void bind();
  virtual void unbind();
  
  virtual void update(double dt);
  
private:

  void createImplementation();
  
  FGInterface* _impl;
  SGPropertyNode* _props; // root property tree for this FDM instance
  bool _dataLogging;
};

#endif // of FG_FDM_SHELL_HXX
