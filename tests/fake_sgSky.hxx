#ifndef FG_TEST_FAKE_SGSKY_HXX
#define FG_TEST_FAKE_SGSKY_HXX

#include <simgear/structure/SGReferenced.hxx>
#include <string>
#include <vector>

class SGCloudLayer : public SGReferenced {
public:

    /**
     * This is the list of available cloud coverages/textures
     */
    enum Coverage {
	SG_CLOUD_OVERCAST = 0,
	SG_CLOUD_BROKEN,
	SG_CLOUD_SCATTERED,
	SG_CLOUD_FEW,
	SG_CLOUD_CIRRUS,
	SG_CLOUD_CLEAR,
	SG_MAX_CLOUD_COVERAGES
    };

    static const std::string SG_CLOUD_OVERCAST_STRING; // "overcast"
    static const std::string SG_CLOUD_BROKEN_STRING; // "broken"
    static const std::string SG_CLOUD_SCATTERED_STRING; // "scattered"
    static const std::string SG_CLOUD_FEW_STRING; // "few"
    static const std::string SG_CLOUD_CIRRUS_STRING; // "cirrus"
    static const std::string SG_CLOUD_CLEAR_STRING; // "clear"

    /**
     * Constructor
     * @param tex_path the path to the set of cloud textures
     */
    SGCloudLayer( const std::string &tex_path ) { }

    /**
     * Destructor
     */
    ~SGCloudLayer( void ) { }

    /** get the cloud span (in meters) */
    float getSpan_m () const
    { return _spanM; }
    /**
     * set the cloud span
     * @param span_m the cloud span in meters
     */
    void setSpan_m (float span_m)
    { _spanM = span_m; }

    /** get the layer elevation (in meters) */
    float getElevation_m () const
    {
      return _elevationM;
    }
    /**
     * set the layer elevation.  Note that this specifies the bottom
     * of the cloud layer.  The elevation of the top of the layer is
     * elevation_m + thickness_m.
     * @param elevation_m the layer elevation in meters
     * @param set_span defines whether it is allowed to adjust the span
     */
    void setElevation_m (float elevation_m, bool set_span = true)
    {
      _elevationM = elevation_m;
    }

    /** get the layer thickness */
    float getThickness_m () const
    {
      return _thicknessM;
    }
    /**
     * set the layer thickness.
     * @param thickness_m the layer thickness in meters.
     */
    void setThickness_m (float thickness_m)
    {
      _thicknessM = thickness_m;
    }

    /** get the layer visibility */
    float getVisibility_m() const
    {
      return _visibilityM;
    }
    /**
     * set the layer visibility
     * @param visibility_m the layer minimum visibility in meters.
     */
    void setVisibility_m(float visibility_m)
    {
      _visibilityM = visibility_m;
    }



    /**
     * get the transition/boundary layer depth in meters.  This
     * allows gradual entry/exit from the cloud layer via adjusting
     * visibility.
     */
    float getTransition_m () const
    {
      return _transitionM;
    }

    /**
     * set the transition layer size in meters
     * @param transition_m the transition layer size in meters
     */
    void setTransition_m (float transition_m)
    {
      _transitionM = transition_m;
    }

    /** get coverage type */
    Coverage getCoverage () const
    {
      return _coverage;
    }

    /**
     * set coverage type
     * @param coverage the coverage type
     */
    void setCoverage (Coverage coverage)
    {
      _coverage = coverage;
    }

    /** get coverage as string */
    const std::string & getCoverageString() const;

    /** get coverage as string */
    static const std::string & getCoverageString( Coverage coverage );

    /** get coverage type from string */
    static Coverage getCoverageType( const std::string & coverage );

    /** set coverage as string */
    void setCoverageString( const std::string & coverage );

    /**
     * set the cloud movement direction
     * @param dir the cloud movement direction
     */
    inline void setDirection(float dir) {
        // cout << "cloud dir = " << dir << endl;
        direction = dir;
    }

    /** get the cloud movement direction */
    inline float getDirection() { return direction; }

    /**
     * set the cloud movement speed
     * @param sp the cloud movement speed
     */
    inline void setSpeed(float sp) {
        // cout << "cloud speed = " << sp << endl;
        speed = sp;
    }

    /** get the cloud movement speed */
    float getSpeed() { return speed; }

    void setAlpha( float alpha );

    void setMaxAlpha( float alpha )
    {
      _maxAlpha = alpha;
    }

    float getMaxAlpha() const
    {
      return _maxAlpha;
    }

    /** Enable/disable 3D clouds in this layer */
    void set_enable3dClouds(bool enable);
private:
  float _spanM = 0.0f;
float _elevationM = 0.0f;
float _thicknessM = 0.0f;
float _transitionM = 0.0f;
float _visibilityM = 0.0f;
Coverage _coverage;
float scale = 0.0f;
float speed = 0.0f;
float direction = 0.0f;
float _maxAlpha = 0.0f;
};



class SGSky
{
public:


  void add_cloud_layer (SGCloudLayer * layer);

  const SGCloudLayer * get_cloud_layer (int i) const
  {
    return _cloudLayers.at(i);
  }

  SGCloudLayer * get_cloud_layer (int i);

  int get_cloud_layer_count () const;


/** @return current effective visibility */
float get_visibility() const
{ return _visibility; }

/** Set desired clear air visibility.
* @param v visibility in meters
*/
void set_visibility( float v )
{ _visibility = v; }

/** Get 3D cloud density */
double get_3dCloudDensity() const
{
  return _3dcloudDensity;
}

/** Set 3D cloud density
* @param density 3D cloud density
*/
void set_3dCloudDensity(double density)
{
  _3dcloudDensity = density;
}

/** Get 3D cloud visibility range*/
float get_3dCloudVisRange() const
{
  return _3dcloudVisRange;
}

/** Set 3D cloud visibility range
*
* @param vis 3D cloud visibility range
*/
void set_3dCloudVisRange(float vis)
{
  _3dcloudVisRange = vis;
}

/** Get 3D cloud impostor distance*/
float get_3dCloudImpostorDistance() const
{
  return _3dcloudImpostorDistance;
}

/** Set 3D cloud impostor distance
*
* @param vis 3D cloud impostor distance
*/
void set_3dCloudImpostorDistance(float vis)
{
  _3dcloudImpostorDistance = vis;
}

/** Get 3D cloud LoD1 Range*/
float get_3dCloudLoD1Range() const
{
  return _3dcloudLoDRange1;
}

/** Set 3D cloud LoD1 Range
* @param vis LoD1 Range
*/
void set_3dCloudLoD1Range(float vis)
{
  _3dcloudLoDRange1 = vis;
}

/** Get 3D cloud LoD2 Range*/
float get_3dCloudLoD2Range() const
{
  return _3dcloudLoDRange2;
}

/** Set 3D cloud LoD2 Range
* @param vis LoD2 Range
*/
void set_3dCloudLoD2Range(float vis)
{
  _3dcloudLoDRange2 = vis;
}

/** Get 3D cloud impostor usage */
bool get_3dCloudUseImpostors() const
{
  return _3dcloudUSeImpostors;
}

/** Set 3D cloud impostor usage
*
* @param imp whether use impostors for 3D clouds
*/
void set_3dCloudUseImpostors(bool imp)
{
  _3dcloudUSeImpostors = imp;
}

/** Get 3D cloud wrapping */
bool get_3dCloudWrap() const
{ return _3dcloudWrap; }

/** Set 3D cloud wrapping
* @param wrap whether to wrap 3D clouds
*/
void set_3dCloudWrap(bool wrap)
{ _3dcloudWrap = wrap; }

void set_clouds_enabled(bool enabled);

private:
  float _visibility = 0.0;
  bool _3dcloudsEnabled = false;
  bool _3dcloudWrap =  false;
  bool _3dcloudUSeImpostors = false;
  float _3dcloudImpostorDistance = 0.0;
  float _3dcloudLoDRange1 = 0.0;
  float _3dcloudLoDRange2 = 0.0;
  float _3dcloudVisRange = 0.0;
  float _3dcloudDensity = 0.0;

  std::vector<SGCloudLayer*> _cloudLayers;
};

#endif
