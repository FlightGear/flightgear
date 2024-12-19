/*
 * SPDX-FileName: AIBase.cxx
 * SPDX-FileComment: abstract base class for AI objects, based on David Luff's FGAIEntity class.
 * SPDX-FileCopyrightText: Written by David Culp, started Nov 2003 - davidculp2@comcast.net
 * SPDX-FileContributor: With additions by Mathias Froehlich & Vivian Meazza 2004-2007
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include <string.h>

#include <simgear/compiler.h>

#include <string>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osgDB/FileUtils>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include <Main/ErrorReporter.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Scripting/NasalModelData.hxx>
#include <Scripting/NasalSys.hxx>
#include <Sound/fg_fx.hxx>

#include "AIFlightPlan.hxx"
#include "AIBase.hxx"
#include "AIManager.hxx"

static std::string default_model = "Models/Geometry/glider.ac";
const double FGAIBase::e = 2.71828183;
const double FGAIBase::lbs_to_slugs = 0.031080950172;   //conversion factor

using std::string;
using namespace simgear;

class FGAIModelData : public simgear::SGModelData {
public:
    explicit FGAIModelData(SGPropertyNode *root = nullptr)
      : _root (root)
    {
    }


    ~FGAIModelData() = default;

    FGAIModelData* clone() const override { return new FGAIModelData(); }

    ErrorContext getErrorContext() const override
    {
        return _errorContext;
    }

    void addErrorContext(const std::string& key, const std::string& value)
    {
        _errorContext[key] = value;
    }

    void captureErrorContext(const std::string& key)
    {
        const auto v = flightgear::ErrorReporter::threadSpecificContextValue(key);
        if (!v.empty()) {
            addErrorContext(key, v);
        }
    }

    /** osg callback, thread-safe */
    void modelLoaded(const std::string& path, SGPropertyNode *prop, osg::Node *n)
    {
        // WARNING: Called in a separate OSG thread! Only use thread-safe stuff here...
        if (_ready && _modelLoaded.count(path) > 0)
            return;

        _modelLoaded[path] = true;

        if(prop->hasChild("interior-path")){
            _interiorPath = prop->getStringValue("interior-path");
            _hasInteriorPath = true;
        }

        // save radius of loaded model for updating LOD
        auto bs = n->getBound();
        if (bs.valid()) {
            _radius = bs.radius();
        }

        _fxpath = prop->getStringValue("sound/path");
        _nasal[path] = std::unique_ptr<FGNasalModelDataProxy>(new FGNasalModelDataProxy(_root));
        _nasal[path]->modelLoaded(path, prop, n);
        _ready = true;
        _initialized = false;
    }

    /** init hook to be called after model is loaded.
     * Not thread-safe. Call from main thread only. */
    void init(void) { _initialized = true; }

    bool needInitialization(void) { return _ready && !_initialized;}
    bool isInitialized(void) { return _initialized;}
    inline std::string& get_sound_path() { return _fxpath;}

    void setInteriorLoaded(const bool state) { _interiorLoaded = state;}
    bool getInteriorLoaded(void) { return _interiorLoaded;}
    bool hasInteriorPath(void) { return _hasInteriorPath;}
    inline std::string& getInteriorPath() { return _interiorPath; }
    inline float getRadius() { return _radius; }
private:
    std::string _fxpath;
    std::string _interiorPath;

    std::map<string, bool> _modelLoaded;
    std::map<string, std::unique_ptr<FGNasalModelDataProxy>> _nasal;
    bool _ready = false;
    bool _initialized = false;
    bool _hasInteriorPath = false;
    bool _interiorLoaded = false;
    float _radius = -1.0;
    SGPropertyNode* _root;

    ErrorContext _errorContext;
};

FGAIBase::FGAIBase(object_type ot, bool enableHot) : replay_time(fgGetNode("sim/replay/time", true)),
                                                     model_removed(fgGetNode("/ai/models/model-removed", true)),
                                                     pos(SGGeod::fromDeg(0.0, 0.0)),
                                                     _impact_lat(0),
                                                     _impact_lon(0),
                                                     _impact_elev(0),
                                                     _impact_hdg(0),
                                                     _impact_pitch(0),
                                                     _impact_roll(0),
                                                     _impact_speed(0),
                                                     _refID(_newAIModelID()),
                                                     _otype(ot)
{
    tgt_heading = hdg = tgt_altitude_ft = tgt_speed = 0.0;
    tgt_roll = roll = tgt_pitch = tgt_yaw = tgt_vs = vs_fps = pitch = 0.0;
    bearing = elevation = range = rdot = 0.0;
    x_shift = y_shift = rotation = 0.0;
    in_range = false;
    invisible = false;
    no_roll = true;
    life = 900;
    delete_me = false;
    _impact_reported = false;
    _collision_reported = false;
    _expiry_reported = false;

    _x_offset = 0;
    _y_offset = 0;
    _z_offset = 0;

    _pitch_offset = 0;
    _roll_offset = 0;
    _yaw_offset = 0;

    speed = 0;
    altitude_ft = 0;
    speed_north_deg_sec = 0;
    speed_east_deg_sec = 0;
    turn_radius_ft = 0;

    ft_per_deg_lon = 0;
    ft_per_deg_lat = 0;

    horiz_offset = 0;
    vert_offset = 0;
    ht_diff = 0;

    serviceable = false;

    // explicitly disable HOT for (most) AI models
    if (!enableHot)
        aip.getSceneGraph()->setNodeMask(~SG_NODEMASK_TERRAIN_BIT);
}

FGAIBase::~FGAIBase() {
    // Unregister that one at the scenery manager
    removeModel();

    if (props) {
        SGPropertyNode* parent = props->getParent();

        if (parent)
            model_removed->setStringValue(props->getPath());
    }

    removeSoundFx();
}

/** Cleanly remove the model
 * and let the scenery database pager do the clean-up work.
 */
void
FGAIBase::removeModel()
{
    if (!_model.valid())
        return;

    FGScenery* pSceneryManager = globals->get_scenery();
    if (pSceneryManager && pSceneryManager->get_models_branch())
    {
        osg::ref_ptr<osg::Object> temp = _model.get();
        pSceneryManager->get_models_branch()->removeChild(aip.getSceneGraph());
        // withdraw from SGModelPlacement and drop own reference (unref)
        aip.clear();
        _modeldata = nullptr;
        _model = nullptr;
        _interior = nullptr;
        _high_res = nullptr;
        _low_res = nullptr;

        // pass it on to the pager, to be be deleted in the pager thread
        pSceneryManager->getPager()->queueDeleteRequest(temp);
    }
    else
    {
        aip.clear();
        _model = nullptr;
        _modeldata = nullptr;
    }
}

void FGAIBase::setScenarioPath(const std::string& scenarioPath)
{
    _scenarioPath = scenarioPath;
}

void FGAIBase::readFromScenario(SGPropertyNode* scFileNode)
{
    if (!scFileNode)
        return;

    setPath(scFileNode->getStringValue("model",
            fgGetString("/sim/multiplay/default-model", default_model).c_str()).c_str());

    setFallbackModelIndex(scFileNode->getIntValue("fallback-model-index", 0));

    setHeading(scFileNode->getDoubleValue("heading", 0.0));
    setSpeed(scFileNode->getDoubleValue("speed", 0.0));
    setAltitude(scFileNode->getDoubleValue("altitude", 0.0));
    setLongitude(scFileNode->getDoubleValue("longitude", 0.0));
    setLatitude(scFileNode->getDoubleValue("latitude", 0.0));
    setBank(scFileNode->getDoubleValue("roll", 0.0));
    setPitch(scFileNode->getDoubleValue("pitch", 0.0));
    setCollisionHeight(scFileNode->getDoubleValue("collision-height", 0.0));
    setCollisionLength(scFileNode->getDoubleValue("collision-length", 0.0));

    SGPropertyNode* submodels = scFileNode->getChild("submodels");

    if (submodels) {
        setServiceable(submodels->getBoolValue("serviceable", false));
        setSMPath(submodels->getStringValue("path", ""));
    }

    string searchOrder = scFileNode->getStringValue("search-order", "");
    if (!searchOrder.empty()) {
        if (searchOrder == "DATA_ONLY") {
            _searchOrder = ModelSearchOrder::DATA_ONLY;
        } else if (searchOrder == "PREFER_AI") {
            _searchOrder = ModelSearchOrder::PREFER_AI;
        } else if (searchOrder == "PREFER_DATA") {
            _searchOrder = ModelSearchOrder::PREFER_DATA;
        } else
            SG_LOG(SG_AI, SG_WARN, "invalid model search order " << searchOrder << ". Use either DATA_ONLY, PREFER_AI or PREFER_DATA");
    }

    const string modelLowres = scFileNode->getStringValue("model-lowres", "");
    if (!modelLowres.empty()) {
        setPathLowres(modelLowres);
    }
}

void FGAIBase::update(double dt) {

    SG_UNUSED(dt);
    if (replay_time->getDoubleValue() > 0)
        return;
    if (_otype == object_type::otStatic)
        return;

    ft_per_deg_lat = 366468.96 - 3717.12 * cos(pos.getLatitudeRad());
    ft_per_deg_lon = 365228.16 * cos(pos.getLatitudeRad());

    if ((_modeldata)&&(_modeldata->needInitialization()))
    {
        // process deferred nasal initialization,
        // which must be done in main thread
        _modeldata->init();

        // update LOD radius from loaded modeldata
        auto radius = _modeldata->getRadius();
        if (radius > 0) {
            _model->setRadius(radius);
            _model->dirtyBound();
        }
        else {
            SG_LOG(SG_AI, SG_WARN, "AIBase: model radius not set.");
        }

        // sound initialization
        if (fgGetBool("/sim/sound/aimodels/enabled",false))
        {
            const string& fxpath = _modeldata->get_sound_path();
            if (fxpath != "")
            {
                removeSoundFx();

                simgear::ErrorReportContext ec("ai-model", _name);
                if (!_scenarioPath.empty()) {
                    ec.add("scenario-path", _scenarioPath);
                }

                props->setStringValue("sim/sound/path", fxpath.c_str());

                // initialize the sound configuration
                std::stringstream name;
                name <<  "aifx:";
                name << _refID;
                _fx = new FGFX(name.str(), props);
                _fx->init();
            }
        }
    }

    if ( _fx )
    {
        // update model's audio sample values
        _fx->set_position_geod( pos );

        SGQuatd orient = SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);
        _fx->set_orientation( orient );

        SGVec3d velocity;
        velocity = SGVec3d( speed_north_deg_sec, speed_east_deg_sec,
                            pitch*speed );
        _fx->set_velocity( velocity );
    }

    updateInterior();
}

void FGAIBase::updateInterior()
{
    if(!_modeldata || !_modeldata->hasInteriorPath())
        return;

    if (!_modeldata->getInteriorLoaded()) { // interior is not yet load
        _interior = SGModelLib::loadPagedModel(_modeldata->getInteriorPath(), props, _modeldata);
        _group->addChild(_interior);
        if (_interior.valid()) {
            bool pixel_mode = !fgGetBool("/sim/rendering/static-lod/aimp-range-mode-distance", false);
            if (pixel_mode) {
                _interior->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
                _interior->setRange(0, _maxRangeInterior, FLT_MAX);
            }
            else {
                _interior->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
                _interior->setRange(0, 0.0, _maxRangeInterior);
            }
            _modeldata->setInteriorLoaded(true);
            SG_LOG(SG_AI, SG_INFO, "AIBase: Loaded interior model " << _interior->getName());
        }
    }
}

/** update LOD properties of the model */
void FGAIBase::updateLOD()
{
    double maxRangeDetail = fgGetDouble("/sim/rendering/static-lod/aimp-detailed", 3000.0);
    double maxRangeBare   = fgGetDouble("/sim/rendering/static-lod/aimp-bare", 10000.0);
    _maxRangeInterior     = fgGetDouble("/sim/rendering/static-lod/aimp-interior", 50.0);

    if (_model.valid())
    {
        bool pixel_mode = !fgGetBool("/sim/rendering/static-lod/aimp-range-mode-distance", false);
        if (pixel_mode) {
            _model->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
        }
        else {
            _model->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        }

        if (maxRangeDetail < 0)
        {
            // High detail model (only)
            // - disables the low detail detail model by setting its visibility from 0 to 0
            if (_high_res.valid()) {
                _model->setRange(modelHighDetailIndex, 0.0, FLT_MAX); // all ranges.
                _model->setRange(modelLowDetailIndex, 0.0, 0.0); // turn it off
            }
            else {
                // only having low-res model
                _model->setRange(modelLowDetailIndex, 0.0, FLT_MAX); // only having low-res model.
                _model->setRange(modelHighDetailIndex, 0.0, 0.0); // only having low-res model.
            }
        }
        else if ((int)maxRangeBare == (int)maxRangeDetail)
        {
            // low detail model  (only)
            if (_low_res.valid()) {
                _model->setRange(modelHighDetailIndex, 0, 0); // turn it off
                _model->setRange(modelLowDetailIndex, 0, FLT_MAX);
            }
            else {
                // Only having high_res model
                _model->setRange(modelHighDetailIndex, 0, FLT_MAX);
                _model->setRange(modelLowDetailIndex, 0, 0.0);
            }
        }
        else
        {
            if(pixel_mode)
            {
                /* In pixel size mode, the range sense is reversed, so we want the
                * detailed model [0] to be displayed when the "range" is really
                * large (i.e. the object is taking up a large number of pixels on screen),
                * and the less detailed model [1] to be displayed if the
                * "range" is between the detailed range and the bare range.
                * When the "range" is less than the bare value, the aircraft
                * represents too few pixels to be worth displaying.
                */

                if (maxRangeBare > maxRangeDetail) {
                  // Sanity check that we have sensible values.
                  maxRangeBare = maxRangeDetail;
                  SG_LOG(SG_AI,
                    SG_WARN,
                    "/sim/rendering/static-lod/aimp-bare greater " <<
                    "than /sim/rendering/static-lod/aimp-detailed when using " <<
                    "/sim/rendering/static-lod/aimp-range-mode-distance=false.  Ignoring ai-bare."
                  );
                }

                if (_low_res.valid() && _high_res.valid()) {
                    /*if (_model->getRadius() < 0)
                    {
                        osg::BoundingSphere bs = _model->computeBound();
                        if (bs.radius() > 0) {
                            _model->setRadius(bs.radius());
                            _model->setCenterMode(osg::LOD::CenterMode::USER_DEFINED_CENTER);
                        }
                    }*/
                  _model->setRange(modelHighDetailIndex , maxRangeDetail, FLT_MAX); // most detailed
                  _model->setRange(modelLowDetailIndex , maxRangeBare, maxRangeDetail); // least detailed
                } else if (_low_res.valid() && !_high_res.valid()) {
                  // we have only low_res_model model it obviously will have to be displayed from the smallest value
                  _model->setRange(modelLowDetailIndex, std::min(maxRangeBare, maxRangeDetail), FLT_MAX );
                  _model->setRange(modelHighDetailIndex, 0,0);
                } else if (!_low_res.valid() && _high_res.valid()) {
                    // we have only high_res model it obviously will have to be displayed from the smallest value
                    _model->setRange(modelHighDetailIndex, std::min(maxRangeBare, maxRangeDetail), FLT_MAX );
                    _model->setRange(modelLowDetailIndex, 0,0);
                }
            } else {
                /* In non-pixel range mode we're dealing with straight distance.
                 * We use the detailed model [0] for when we are up to the detailed
                 * range, and the less complex model [1] (if available) for further
                 * away up to the bare range.
                 * - in this case the maxRangeBare is a delta on top of maxRangeDetail.
                 */

                if (maxRangeBare <= 0) {
                    // Sanity check that we have sensible values.
                    maxRangeBare = 1;
                    SG_LOG(SG_AI,
                        SG_ALERT,
                        "/sim/rendering/static-lod/aimp-bare is <= 0. This should be a delta on top of aimp-detailed in meters mode. setting to 1.");
                }


                if (_low_res.valid() && _high_res.valid()) {
                  _model->setRange(modelHighDetailIndex , 0, maxRangeDetail); // most detailed
                  _model->setRange(modelLowDetailIndex , maxRangeDetail, maxRangeDetail+maxRangeBare); // least detailed
                } else if (_low_res.valid() && !_high_res.valid()) {
                  _model->setRange(modelLowDetailIndex, 0, maxRangeBare + maxRangeDetail); // only low_res, so display from 0 to the highest value in meters
                  _model->setRange(modelHighDetailIndex, 0, 0);
                } else if (!_low_res.valid() && _high_res.valid()) {
                    _model->setRange(modelHighDetailIndex, 0, maxRangeBare + maxRangeDetail); // only high_res, so display from 0 to the highest value in meters
                    _model->setRange(modelLowDetailIndex, 0, 0);
                }
            }
        }
        if (_modeldata->getInteriorLoaded() && _interior.valid()) {
            if (pixel_mode) {
                _interior->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
                _interior->setRange(0, _maxRangeInterior, FLT_MAX);
            }
            else {
                _interior->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
                _interior->setRange(0, 0.0, _maxRangeInterior);
            }
        }
    }
}

void FGAIBase::Transform() {

    if (!invisible) {
        aip.setVisible(true);
        aip.setPosition(pos);

        if (no_roll)
            aip.setOrientation(0.0, pitch, hdg);
        else
            aip.setOrientation(roll, pitch, hdg);

        aip.update();
    } else {
        aip.setVisible(false);
        aip.update();
    }

}

/*
 * Find a set of paths to the model, in order of LOD from most detailed to
 * least, and accounting for the user preference of detailed models vs. AI
 * low resolution models.
 *
 * This returns a vector of size 1 or 2.
 */
std::vector<std::string> FGAIBase::resolveModelPath(ModelSearchOrder searchOrder)
{
    string_list path_list;

    if (searchOrder == ModelSearchOrder::DATA_ONLY) {
        SG_LOG(SG_AI, SG_DEBUG, "Resolving model path:  DATA only");
        auto p = simgear::SGModelLib::findDataFile(model_path);
        if (!p.empty()) {
            // We've got a model, use it
            _installed = true;
            SG_LOG(SG_AI, SG_DEBUG, "Found model " << p);
            path_list.push_back(p);

            if (!model_path_lowres.empty()) {
                p = simgear::SGModelLib::findDataFile(model_path_lowres);
                if (!p.empty()) {
                    //lowres model needs to be the first in the list
                    path_list.insert(path_list.begin(),p);
                }
            }
        } else {
            // No model, so fall back to the default
            const SGPath defaultModelPath = SGPath::fromUtf8(fgGetString("/sim/multiplay/default-model", default_model));
            path_list.push_back(defaultModelPath.utf8Str());
        }
    } else {
        SG_LOG(SG_AI, SG_DEBUG, "Resolving model path:  PREFER_AI/PREFER_DATA");
        // We're either PREFER_AI or PREFER_DATA.  Find an AI model first.
        for (SGPath p : globals->get_data_paths("AI")) {
            p.append(model_path);
            if (p.exists()) {
                SG_LOG(SG_AI, SG_DEBUG, "Found AI model: " << p);
                path_list.push_back(p.utf8Str());
                break;
            }
        }

        if (path_list.empty()) {
            // Fall back on the fallback-model-index which is a lookup into
            // /sim/multiplay/fallback-models/model[]
            std::string fallback_path;
            const SGPropertyNode* fallbackNode =
              globals->get_props()->getNode("/sim/multiplay/fallback-models/model", _getFallbackModelIndex(), false);

            if (fallbackNode != nullptr) {
              fallback_path = fallbackNode->getStringValue();
            } else {
              fallback_path = globals->get_props()->getNode("/sim/multiplay/fallback-models/model", 0, true)->getStringValue();
            }

            for (SGPath p : globals->get_data_paths()) {
                p.append(fallback_path);
                if (p.exists()) {
                    SG_LOG(SG_AI, SG_DEBUG, "Found fallback model path for index " << _fallback_model_index << ": " << p);
                    path_list.push_back(p.utf8Str());
                    break;
                }
            }
        }

        if ((searchOrder == ModelSearchOrder::PREFER_AI) && !path_list.empty()) {
            // if we prefer AI, and we've got a valid AI path from above, then use it, we're done
            _installed = true;
            return path_list;
        }

        // At this point we're looking for a regular model to display at closer range.
        // From experimentation it seems to work best if the LODs are in the range list in terms of detail
        // from lowest to highest - so insert this at the end.
        auto p = simgear::SGModelLib::findDataFile(model_path);
        if (!p.empty()) {
            _installed = true;
            SG_LOG(SG_AI, SG_DEBUG, "Found DATA model " << p);
            path_list.insert(path_list.end(), p);
        }
    }

    /*
     * We return either one or two models.  LoD logic elsewhere relies on this,
     * so anything else is a logic error in the above code.
     */
    assert(path_list.size() != 0);
    assert(path_list.size() <  3);

    return path_list;
}

bool FGAIBase::init(ModelSearchOrder searchOrder)
{
    if (_model.valid())
    {
        SG_LOG(SG_AI, SG_ALERT, "AIBase: Cannot initialize a model multiple times! " << model_path);
        return false;
    }

    props->addChild("type")->setStringValue("AI");
    _modeldata = new FGAIModelData(props);
    _modeldata->addErrorContext("ai", _name);
    _modeldata->captureErrorContext("scenario-path");

    // set by FGAISchedule::createAIAircraft
    _modeldata->captureErrorContext("traffic-aircraft-callsign");

    if (_otype == object_type::otMultiplayer) {
        _modeldata->addErrorContext("multiplayer", getCallSign());
    }

    // Load models
    _model = new osg::LOD();
    std::vector<string> model_list = resolveModelPath(searchOrder);
    if(model_list.size() == 1 && _modeldata && _modeldata->hasInteriorPath()) {
        // Only one model and interior available (expecting this to be a high_res model)
        _low_res = new osg::PagedLOD(); // Dummy node to keep LOD node happy
        _model->addChild(_low_res);
        _high_res = SGModelLib::loadPagedModel(model_list[0], props, _modeldata);
        _group = osg::ref_ptr<osg::Group>(new osg::Group());
        _group->addChild(_high_res);
        _model->addChild(_group);
    } else if (model_list.size() == 1) {
        // low_res model only (as we do not have any interior)
        _low_res = SGModelLib::loadPagedModel(model_list[0], props, _modeldata);
        _model->addChild(_low_res);
        _group = osg::ref_ptr<osg::Group>(new osg::Group()); // Dummy node to keep LOD node happy
        _model->addChild(_group);
    } else {
        // high and low-res model
        assert(model_list.size() == 2);
        _low_res = SGModelLib::loadPagedModel(model_list[0], props, _modeldata);
        _model->addChild(_low_res);
        _high_res = SGModelLib::loadPagedModel(model_list[1], props, _modeldata);
        _group = osg::ref_ptr<osg::Group>(new osg::Group());
        _group->addChild(_high_res);
        _model->addChild(_group);
    }

    // Set PagedLODs to MAX Range. The visibility is controlled with the top-level LOD node
    if(_high_res.valid()) {
        _high_res->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        _high_res->setRange(0, 0, FLT_MAX);
    }
    if(_low_res.valid()) {
        _low_res->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        _low_res->setRange(0, 0, FLT_MAX);
    }

    _model->setName("AI-model range animation node");
    _model->setRadius(getDefaultModelRadius());

    updateLOD();
    initModel();

    if (_model.valid() && _initialized == false) {
        aip.init( _model.get() );
        aip.setVisible(true);
        invisible = false;
        
        auto scenery = globals->get_scenery();
        if (scenery) {
            scenery->get_models_branch()->addChild(aip.getSceneGraph());
        }
        _initialized = true;

        SG_LOG(SG_AI, SG_DEBUG, "AIBase: Loaded model " << model_path);

    } else if (!model_path.empty()) {
        SG_LOG(SG_AI, SG_WARN, "AIBase: Could not load model " << model_path);
        // not properly installed...
        _installed = false;
    }

    setDie(false);
    return true;
}

void FGAIBase::initModel()
{
    if (_model.valid()) {

        if( _path != ""){
            props->setStringValue("submodels/path", _path.c_str());
            SG_LOG(SG_AI, SG_DEBUG, "AIBase: submodels/path " << _path);
        }

        if( _parent!= ""){
            props->setStringValue("parent-name", _parent.c_str());
        }

        fgSetString("/ai/models/model-added", props->getPath().c_str());
    } else if (!model_path.empty()) {
        SG_LOG(SG_AI, SG_WARN, "AIBase: Could not load model " << model_path);
    }

    setDie(false);
}


bool FGAIBase::isa( object_type otype ) {
    return otype == _otype;
}

void FGAIBase::bind() {
    _tiedProperties.setRoot(props);
    tie("id", SGRawValueMethods<FGAIBase,int>(*this,
        &FGAIBase::getID));
    tie("velocities/true-airspeed-kt",  SGRawValuePointer<double>(&speed));
    tie("velocities/vertical-speed-fps",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getVS_fps,
        &FGAIBase::_setVS_fps));

    tie("position/altitude-ft",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getAltitude,
        &FGAIBase::_setAltitude));
    tie("position/latitude-deg",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getLatitude,
        &FGAIBase::_setLatitude));
    tie("position/longitude-deg",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getLongitude,
        &FGAIBase::_setLongitude));

    tie("position/global-x",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getCartPosX,
        nullptr));
    tie("position/global-y",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getCartPosY,
        nullptr));
    tie("position/global-z",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getCartPosZ,
        nullptr));
    tie("callsign",
        SGRawValueMethods<FGAIBase,const char*>(*this,
        &FGAIBase::_getCallsign,
        nullptr));
    // 2018.2 - to ensure consistent properties also tie the callsign to where it would be in a local model.
    tie("sim/multiplay/callsign",
        SGRawValueMethods<FGAIBase, const char*>(*this, &FGAIBase::_getCallsign, nullptr));

    tie("orientation/pitch-deg",   SGRawValuePointer<double>(&pitch));
    tie("orientation/roll-deg",    SGRawValuePointer<double>(&roll));
    tie("orientation/true-heading-deg", SGRawValuePointer<double>(&hdg));

    tie("radar/in-range", SGRawValuePointer<bool>(&in_range));
    tie("radar/bearing-deg",   SGRawValuePointer<double>(&bearing));
    tie("radar/elevation-deg", SGRawValuePointer<double>(&elevation));
    tie("radar/range-nm", SGRawValuePointer<double>(&range));
    tie("radar/h-offset", SGRawValuePointer<double>(&horiz_offset));
    tie("radar/v-offset", SGRawValuePointer<double>(&vert_offset));
    tie("radar/x-shift", SGRawValuePointer<double>(&x_shift));
    tie("radar/y-shift", SGRawValuePointer<double>(&y_shift));
    tie("radar/rotation", SGRawValuePointer<double>(&rotation));
    tie("radar/ht-diff-ft", SGRawValuePointer<double>(&ht_diff));
    tie("subID", SGRawValuePointer<int>(&_subID));

    props->setStringValue("sim/model/path", model_path);

    // note: AIAircraft creates real SGPropertyNodes for these, we don't do
    // that here because it would bloat AIBase slightly
    props->setBoolValue("controls/glide-path", true);

    props->setStringValue("controls/flight/lateral-mode", "roll");
    props->setDoubleValue("controls/flight/target-hdg", hdg);
    props->setDoubleValue("controls/flight/target-roll", roll);

    props->setStringValue("controls/flight/vertical-mode", "alt");

    // The property above was incorrectly labelled 'longitude-mode' up until
    // FG 2018.4, so create an alias in case anyone is relying on the old name
    auto node = props->getNode("controls/flight/longitude-mode", true);
    node->alias(props->getNode("controls/flight/vertical-mode"), false);

    props->setDoubleValue("controls/flight/target-alt", altitude_ft);
    props->setDoubleValue("controls/flight/target-pitch", pitch);

    props->setDoubleValue("controls/flight/target-spd", speed);

    props->setBoolValue("sim/sound/avionics/enabled", false);
    props->setDoubleValue("sim/sound/avionics/volume", 0.0);
    props->setBoolValue("sim/sound/avionics/external-view", false);
    props->setBoolValue("sim/current-view/internal", false);
}

void FGAIBase::unbind() {
    _tiedProperties.Untie();

    props->setBoolValue("/sim/controls/radar", true);

    removeSoundFx();
}

void FGAIBase::removeSoundFx()
{
    if (_fx) {
        _fx->shutdown();
        _fx.clear();
    }
}

double FGAIBase::UpdateRadar(FGAIManager* manager)
{
    if (!manager->isRadarEnabled())
        return 0.0;

    const double radar_range_m = manager->radarRangeM() * 1.1; // + 10%
    bool force_on = manager->enableRadarDebug();
    double d = dist(SGVec3d::fromGeod(pos), globals->get_aircraft_position_cart());
    double dFt = d * SG_METER_TO_FEET;
    in_range = (d < radar_range_m);

    if (!force_on && !in_range) {
        return dFt * dFt;
    }

    // copy values from the AIManager
    double user_heading   = manager->get_user_heading();
    double user_pitch     = manager->get_user_pitch();

    range = d * SG_METER_TO_NM;
    // calculate bearing to target
    bearing = SGGeodesy::courseDeg(globals->get_aircraft_position(), pos);

    // calculate look left/right to target, without yaw correction
    horiz_offset = bearing - user_heading;
    SG_NORMALIZE_RANGE(horiz_offset, -180.0, 180.0);

    // calculate elevation to target
    ht_diff = altitude_ft - globals->get_aircraft_position().getElevationFt();
    elevation = atan2( ht_diff, dFt ) * SG_RADIANS_TO_DEGREES;

    // calculate look up/down to target
    vert_offset = elevation - user_pitch;

    /* this calculation needs to be fixed, but it isn't important anyway
    // calculate range rate
    double recip_bearing = bearing + 180.0;
    if (recip_bearing > 360.0) recip_bearing -= 360.0;
    double my_horiz_offset = recip_bearing - hdg;
    if (my_horiz_offset > 180.0) my_horiz_offset -= 360.0;
    if (my_horiz_offset < -180.0) my_horiz_offset += 360.0;
    rdot = (-user_speed * cos( horiz_offset * SG_DEGREES_TO_RADIANS ))
    +(-speed * 1.686 * cos( my_horiz_offset * SG_DEGREES_TO_RADIANS ));
    */

    // now correct look left/right for yaw
    // horiz_offset += user_yaw; // FIXME: WHY WOULD WE WANT TO ADD IN SIDE-SLIP HERE?

    // calculate values for radar display
    y_shift = range * cos( horiz_offset * SG_DEGREES_TO_RADIANS);
    x_shift = range * sin( horiz_offset * SG_DEGREES_TO_RADIANS);

    rotation = hdg - user_heading;
    SG_NORMALIZE_RANGE(rotation, 0.0, 360.0);

    return dFt * dFt;
}

/*
* Getters and Setters
*/

SGVec3d FGAIBase::getCartPosAt(const SGVec3d& _off) const {
    // Transform that one to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(pos);

    // and postrotate the orientation of the AIModel wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);

    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth fixed coordinates axis
    SGVec3d off = hlTrans.backTransform(_off);

    // Add the position offset of the AIModel to gain the earth centered position
    SGVec3d cartPos = SGVec3d::fromGeod(pos);

    return cartPos + off;
}

SGVec3d FGAIBase::getCartPos() const {
    SGVec3d cartPos = SGVec3d::fromGeod(pos);
    return cartPos;
}

bool FGAIBase::getGroundElevationM(const SGGeod& pos, double& elev,
                                   const simgear::BVHMaterial** material) const {
    return globals->get_scenery()->get_elevation_m(pos, elev, material,
                                                   _model.get());
}

SGPropertyNode* FGAIBase::getPositionFromNode(SGPropertyNode* scFileNode, const std::string &key, SGVec3d &position) {
    SGPropertyNode* positionNode = scFileNode->getChild(key);
    if (positionNode) {
        // Transform to the right coordinate frame, configuration is done in
        // the usual x-back, y-right, z-up coordinates, computations
        // in the simulation usual body x-forward, y-right, z-down coordinates
        position(0) = -positionNode->getDoubleValue("x-offset-m", 0);
        position(1) = positionNode->getDoubleValue("y-offset-m", 0);
        position(2) = -positionNode->getDoubleValue("z-offset-m", 0);
        return positionNode;
    }
    else
        position = SGVec3d::zeros();
    return nullptr;
}

double FGAIBase::_getCartPosX() const {
    SGVec3d cartPos = getCartPos();
    return cartPos.x();
}

double FGAIBase::_getCartPosY() const {
    SGVec3d cartPos = getCartPos();
    return cartPos.y();
}

double FGAIBase::_getCartPosZ() const {
    SGVec3d cartPos = getCartPos();
    return cartPos.z();
}

void FGAIBase::_setLongitude( double longitude ) {
    pos.setLongitudeDeg(longitude);
}

void FGAIBase::_setLatitude ( double latitude )  {
    pos.setLatitudeDeg(latitude);
}

void FGAIBase::_setSubID( int s ) {
    _subID = s;
}

bool FGAIBase::setParentNode() {
    if (_parent == ""){
        SG_LOG(SG_AI, SG_ALERT, "AIBase: " << _name << " parent not set ");
       return false;
    }

    const SGPropertyNode_ptr ai = fgGetNode("/ai/models", true);

    for (int i = ai->nChildren() - 1; i >= -1; i--) {
        SGPropertyNode_ptr model;

        if (i < 0) { // last iteration: selected model
            model = _selected_ac;
        } else {
            model = ai->getChild(i);
            const string name = model->getStringValue("name");

            if (!model->nChildren()){
                continue;
            }

            if (name == _parent) {
                _selected_ac = model;  // save selected model for last iteration
                break;
            }
        }

        if (!model)
            continue;
    }// end for loop

    if (_selected_ac != 0){
        // DEADCODE: const string name = _selected_ac->getStringValue("name");
        return true;
    } else {
        SG_LOG(SG_AI, SG_ALERT, "AIBase: " << _name << " parent not found: dying ");
        setDie(true);
        return false;
    }
}

double FGAIBase::_getLongitude() const {
    return pos.getLongitudeDeg();
}

double FGAIBase::_getLatitude() const {
    return pos.getLatitudeDeg();
}

double FGAIBase::_getElevationFt() const {
    return pos.getElevationFt();
}

double FGAIBase::_getRdot() const {
    return rdot;
}

double FGAIBase::_getVS_fps() const {
    return vs_fps;
}

double FGAIBase::_get_speed_east_fps() const {
    return speed_east_deg_sec * ft_per_deg_lon;
}

double FGAIBase::_get_speed_north_fps() const {
    return speed_north_deg_sec * ft_per_deg_lat;
}

void FGAIBase::_setVS_fps( double _vs ) {
    vs_fps = _vs;
}

double FGAIBase::_getAltitude() const {
    return altitude_ft;
}

double FGAIBase::_getAltitudeAGL(SGGeod inpos, double start){
    getGroundElevationM(SGGeod::fromGeodM(inpos, start),
        _elevation_m, NULL);
    return inpos.getElevationFt() - _elevation_m * SG_METER_TO_FEET;
}

bool FGAIBase::_getServiceable() const {
    return serviceable;
}

SGPropertyNode* FGAIBase::_getProps() const {
    return props;
}

void FGAIBase::_setAltitude( double _alt ) {
    setAltitude( _alt );
}

bool FGAIBase::_isNight() {
    return (fgGetFloat("/sim/time/sun-angle-rad") > 1.57);
}

bool FGAIBase::_getCollisionData() {
    return _collision_reported;
}

bool FGAIBase::_getExpiryData() {
    return _expiry_reported;
}

bool FGAIBase::_getImpactData() {
    return _impact_reported;
}

double FGAIBase::_getImpactLat() const {
    return _impact_lat;
}

double FGAIBase::_getImpactLon() const {
    return _impact_lon;
}

double FGAIBase::_getImpactElevFt() const {
    return _impact_elev * SG_METER_TO_FEET;
}

double FGAIBase::_getImpactPitch() const {
    return _impact_pitch;
}

double FGAIBase::_getImpactRoll() const {
    return _impact_roll;
}

double FGAIBase::_getImpactHdg() const {
    return _impact_hdg;
}

double FGAIBase::_getImpactSpeed() const {
    return _impact_speed;
}

int FGAIBase::getID() const {
    return  _refID;
}

int FGAIBase::_getSubID() const {
    return  _subID;
}

double FGAIBase::_getSpeed() const {
    return speed;
}

double FGAIBase::_getRoll() const {
    return roll;
}

double FGAIBase::_getPitch() const {
    return pitch;
}

double FGAIBase::_getHeading() const {
    return hdg;
}

double  FGAIBase::_getXOffset() const {
    return _x_offset;
}

double  FGAIBase::_getYOffset() const {
    return _y_offset;
}

double  FGAIBase::_getZOffset() const {
    return _z_offset;
}

const char* FGAIBase::_getPath() const {
    return model_path.c_str();
}

const char* FGAIBase::_getSMPath() const {
    return _path.c_str();
}

const char* FGAIBase::_getName() const {
    return _name.c_str();
}

const char* FGAIBase::_getCallsign() const {
    return _callsign.c_str();
}

const char* FGAIBase::_getSubmodel() const {
    return _submodel.c_str();
}

int FGAIBase::_getFallbackModelIndex() const {
    return _fallback_model_index;
}

int FGAIBase::_newAIModelID() {
    static int id = 0;

    if (!++id)
        id++;	// id = 0 is not allowed.

    return id;
}

void FGAIBase::setFlightPlan(std::unique_ptr<FGAIFlightPlan> f)
{
    fp = std::move(f);
}

bool FGAIBase::isValid() const
{
	//Either no flightplan or it is valid
	return !fp || fp->isValidPlan();
}

osg::LOD* FGAIBase::getSceneBranch() const
{
    return _model;
}

bool FGAIBase::modelLoaded() const
{
    if(_low_res.valid())
        return _low_res->getNumChildren() >= 1;
    else if (_high_res.valid())
        return _high_res->getNumChildren() >= 1;
    return false;
}

SGGeod FGAIBase::getGeodPos() const
{
    return pos;
}

void FGAIBase::setGeodPos(const SGGeod& geod)
{
    pos = geod;
}
