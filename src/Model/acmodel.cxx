// acmodel.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstring>		// for strcmp()

#include <simgear/compiler.h>
#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/debug/logstream.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/model/modellib.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/view.hxx>
#include <Scenery/scenery.hxx>
#include <Sound/fg_fx.hxx>
#include <GUI/Highlight.hxx>

#include "acmodel.hxx"


static osg::Node *
fgLoad3DModelPanel(const SGPath &path, SGPropertyNode *prop_root)
{
    bool loadPanels = true;
    bool autoTooltipsMaster = fgGetBool("/sim/rendering/automatic-animation-tooltips/enabled");
    int autoTooltipsMasterMax = fgGetInt("/sim/rendering/automatic-animation-tooltips/max-count");
    SG_LOG(SG_INPUT, SG_DEBUG, ""
            << " autoTooltipsMaster=" << autoTooltipsMaster
            << " autoTooltipsMasterMax=" << autoTooltipsMasterMax
            );
    osg::Node* node = simgear::SGModelLib::loadModel(path.utf8Str(), prop_root, NULL, loadPanels, autoTooltipsMaster, autoTooltipsMasterMax);
    if (node)
        node->setNodeMask(~SG_NODEMASK_TERRAIN_BIT);
    return node;
}

////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel
////////////////////////////////////////////////////////////////////////

FGAircraftModel::FGAircraftModel()
    : _velocity(SGVec3d::zeros())
{
}

FGAircraftModel::~FGAircraftModel ()
{
    shutdown();
}


// Gathers information about all animated nodes and the properties that they
// depend on, and registers with Highlight::add_property_node().
//
struct VisitorHighlight : osg::NodeVisitor
{
    VisitorHighlight()
    {
        m_highlight = globals->get_subsystem<Highlight>();
        setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
    }
    std::string spaces()
    {
        return std::string(m_level*4, ' ');
    }
    virtual void apply(osg::Node& node)
    {
        SG_LOG(SG_GENERAL, SG_DEBUG, spaces() << "node: " << node.libraryName() << "::" << node.className());
        m_level += 1;
        traverse(node);
        m_level -= 1;
    }
    virtual void apply(osg::Group& group)
    {
        // Parent nodes of <group> are animated by properties in
        // <m_highlight_names>, so register the association between <group> and
        // these properties.
        SG_LOG(SG_GENERAL, SG_DEBUG, spaces() << "group: " << group.libraryName() << "::" << group.className());
        for (auto name: m_highlight_names)
        {
            if (m_highlight) {
                m_highlight->addPropertyNode(name, &group);
            }
        }
        m_level += 1;
        traverse(group);
        m_level -= 1;
    }
    virtual void apply( osg::Transform& node )
    {
        SG_LOG(SG_GENERAL, SG_DEBUG, spaces() << "transform: "
                << node.libraryName() << "::" << node.className()
                << ": " << typeid(node).name()
                );

        SGSharedPtr<SGExpressiond const> expression = TransformExpression(&node);
        int num_added = 0;
        if (expression) {
            // All <nodes>'s child nodes will be affected by <expression> so
            // add all of <expression>'s properties to <m_highlight_names> so
            // that we can call Highlight::add_property_node() for each child
            // node.
            std::set<const SGPropertyNode*> properties;
            expression->collectDependentProperties(properties);
            SG_LOG(SG_GENERAL, SG_DEBUG, spaces() << typeid(node).name() << ":");
            for (auto p: properties) {
                SG_LOG(SG_GENERAL, SG_DEBUG, spaces() << "        " << p->getPath(true));
                num_added += 1;
                m_highlight_names.push_back(p->getPath(true /*simplify*/));
            }
        }

        m_level += 1;
        traverse(node);
        m_level -= 1;

        // Remove the names we appended to <m_highlight_names> earlier.
        assert((int) m_highlight_names.size() >= num_added);
        m_highlight_names.resize(m_highlight_names.size() - num_added);
    }
    unsigned int m_level = 0;   // Only used to indent diagnostics.
    std::vector<std::string> m_highlight_names;
    Highlight* m_highlight = nullptr;
};


void
FGAircraftModel::init ()
{
    if (_aircraft.get()) {
        SG_LOG(SG_AIRCRAFT, SG_ALERT, "FGAircraftModel::init: already inited");
        return;
    }

    simgear::ErrorReportContext ec("primary-aircraft", "yes");
    _fx = new FGFX("fx");
    _fx->init();

    SGPropertyNode_ptr sim = fgGetNode("/sim", true);
    for (auto model : sim->getChildren("model")) {
        std::string path = model->getStringValue("path", "Models/Geometry/glider.ac");
        std::string usage = model->getStringValue("usage", "external");

        simgear::ErrorReportContext ec("aircraft-model", path);

        SGPath resolvedPath = globals->resolve_aircraft_path(path);
        if (resolvedPath.isNull()) {
            simgear::reportFailure(simgear::LoadFailure::NotFound,
                                   simgear::ErrorCode::XMLModelLoad,
                                   "Failed to find aircraft model", SGPath::fromUtf8(path));
            SG_LOG(SG_AIRCRAFT, SG_ALERT, "Failed to find aircraft model: " << path);
            continue;
        }

        osg::Node* node = NULL;
        try {
            node = fgLoad3DModelPanel( resolvedPath, globals->get_props());
        } catch (const sg_exception &ex) {
            simgear::reportFailure(simgear::LoadFailure::BadData,
                                   simgear::ErrorCode::XMLModelLoad,
                                   "Failed to load aircraft model:" + ex.getFormattedMessage(),
                                   ex.getLocation());
            SG_LOG(SG_AIRCRAFT, SG_ALERT, "Failed to load aircraft from " << path << ':');
            SG_LOG(SG_AIRCRAFT, SG_ALERT, "  " << ex.getFormattedMessage());
        }

        if (usage == "interior") {
            // interior model
            if (!_interior.get()) {
                _interior.reset(new SGModelPlacement);
                _interior->init(node);
            } else {
                _interior->add(node);
            }
        } else {
            // normal / exterior model
            if (!_aircraft.get()) {
                _aircraft.reset(new SGModelPlacement);
                _aircraft->init(node);
            } else {
                _aircraft->add(node);
            }
        } // of model usage switch
    } // of models iteration

    // no models loaded, load the glider instead
    if (!_aircraft.get()) {
        SG_LOG(SG_AIRCRAFT, SG_ALERT, "(Falling back to glider.ac.)");
        osg::Node* model = fgLoad3DModelPanel( SGPath::fromUtf8("Models/Geometry/glider.ac"),
                                   globals->get_props());
        _aircraft.reset(new SGModelPlacement);
        _aircraft->init(model);

    }

  osg::Node* node = _aircraft->getSceneGraph();
  globals->get_scenery()->get_aircraft_branch()->addChild(node);

    if (_interior.get()) {
        node = _interior->getSceneGraph();
        globals->get_scenery()->get_interior_branch()->addChild(node);
    }

    // Register animated nodes and associated properties with Highlight.
    VisitorHighlight visitor_highlight;
    visitor_highlight.traverse(*node);
}

void
FGAircraftModel::reinit()
{
    shutdown();
    init();
    // TODO globally create signals for all subsystems (re)initialized
    fgSetBool("/sim/signals/model-reinit", true);
}

void
FGAircraftModel::shutdown()
{
    FGScenery* scenery = globals->get_scenery();

    if (_aircraft.get()) {
        if (scenery && scenery->get_aircraft_branch()) {
            scenery->get_aircraft_branch()->removeChild(_aircraft->getSceneGraph());
        }
    }

    if (_interior.get()) {
        if (scenery && scenery->get_interior_branch()) {
            scenery->get_interior_branch()->removeChild(_interior->getSceneGraph());
        }
    }

    _aircraft.reset();
    _interior.reset();

    if (_fx) {
        // becuase the sound-manager keeps a ref to our FX itself, we need to manually call
        // shutdown() to unregister from the Sound-manager, otherwise the ref will persist,
        // and prevent us re-registering a new FGFX on reset/reinit
        _fx->shutdown();
        _fx.reset();
    }
}

void
FGAircraftModel::bind ()
{
   _speed_n = fgGetNode("velocities/speed-north-fps", true);
   _speed_e = fgGetNode("velocities/speed-east-fps", true);
   _speed_d = fgGetNode("velocities/speed-down-fps", true);
}

void
FGAircraftModel::unbind ()
{
    _speed_n.reset();
    _speed_e.reset();
    _speed_d.reset();
}

void
FGAircraftModel::update (double dt)
{
    int view_number = globals->get_viewmgr()->getCurrentViewIndex();
    int is_internal = fgGetBool("/sim/current-view/internal");

    if (view_number == 0 && !is_internal) {
        _aircraft->setVisible(false);
  } else {
    _aircraft->setVisible(true);
  }

    double heading, pitch, roll;
    globals->get_aircraft_orientation(heading, pitch, roll);
    SGQuatd orient = SGQuatd::fromYawPitchRollDeg(heading, pitch, roll);

    SGGeod pos = globals->get_aircraft_position();

    _aircraft->setPosition(pos);
    _aircraft->setOrientation(orient);
    _aircraft->update();

    if (_interior.get()) {
        _interior->setPosition(pos);
        _interior->setOrientation(orient);
        _interior->update();
    }

  // update model's audio sample values
  _fx->set_position_geod( pos );
  _fx->set_orientation( orient );

  _velocity = SGVec3d( _speed_n->getDoubleValue(),
                       _speed_e->getDoubleValue(),
                       _speed_d->getDoubleValue() );
  _fx->set_velocity( _velocity );

  float temp_c = fgGetFloat("/environment/temperature-degc");
  float humidity = fgGetFloat("/environment/relative-humidity");
  float pressure = fgGetFloat("/environment/pressure-inhg")*SG_INHG_TO_PA/1000.0f;
  _fx->set_atmosphere( temp_c, humidity, pressure );

  // _fx->update() is run via SGSoundMgr, don't need to call it here
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGAircraftModel> registrantFGAircraftModel(
    SGSubsystemMgr::DISPLAY);

// end of model.cxx
