#include "Highlight.hxx"
#include "simgear/misc/strutils.hxx"

#include <Model/acmodel.hxx>
#include <Main/globals.hxx>
#include <FDM/fdm_shell.hxx>
#include <FDM/flight.hxx>

#include <simgear/scene/util/StateAttributeFactory.hxx>

#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/Texture2D>

#include <algorithm>
#include <map>
#include <string>
#include <iterator>


/* Returns translated string for specified menu/menuitem name. */
static std::string s_getMenuName(SGPropertyNode* node)
{
    /* We use 'label' if available, otherwise we translate 'name' using
    sim/intl/locale/... */
    std::string name = node->getStringValue("label", "");
    if (name != "") return name;
    name = node->getStringValue("name");
    return globals->get_props()->getStringValue("sim/intl/locale/strings/menu/" + name, name.c_str());
}

std::string HighlightMenu::description() const
{
    std::string ret = std::to_string(this->menu) + "/" + std::to_string(this->item) + ": ";
    SGPropertyNode* menubar = globals->get_props()->getNode("sim/menubar/default");
    if (menubar)
    {
        SGPropertyNode* menu = menubar->getChild("menu", this->menu);
        if (menu)
        {
            SGPropertyNode* item = menu->getChild("item", this->item);
            if (item)
            {
                ret += s_getMenuName(menu) + '/' + s_getMenuName(item);
            }
        }
    }
    return ret;
}

static bool operator< (const HighlightMenu& a, const HighlightMenu& b)
{
    if (a.menu != b.menu)   return a.menu < b.menu;
    return a.item < b.item;
}

struct NameValue
{
    NameValue(const std::string& name, const std::string& value)
    : name(name), value(value)
    {}
    std::string name;
    std::string value;
    bool operator< (const NameValue& rhs) const
    {
        if (name != rhs.name)   return name < rhs.name;
        return value < rhs.value;
    }
};

static std::map<std::string, HighlightInfo>                     s_property_to_info;

static std::map<osg::ref_ptr<osg::Node>, std::set<std::string>> s_node_to_properties;
static std::map<std::string, std::set<std::string>>             s_dialog_to_properties;
static std::map<std::string, std::set<std::string>>             s_keypress_to_properties;
static std::map<HighlightMenu, std::set<std::string>>           s_menu_to_properties;

static std::map<HighlightMenu, std::string>                     s_menu_to_dialog;
static std::map<std::string, std::set<HighlightMenu>>           s_dialog_to_menus;

static std::map<std::string, std::set<std::string>>             s_property_to_properties;
static std::map<std::string, std::set<std::string>>             s_property_from_properties;

static std::set<NameValue>                                      s_property_tree_items;

static SGPropertyNode_ptr                                       s_prop_enabled = nullptr;
static bool                                                     s_output_stats = false;

static std::unique_ptr<struct NodeHighlighting>                 s_node_highlighting;

std::unique_ptr<struct FdmInitialisedListener>                  s_fdm_initialised_listener;


/* Handles highlighting of individual nodes by changing their StateSet to
our internal StateSet. Also changes our StateSet's material in response to
sim/highlighting/material/. */
struct NodeHighlighting : SGPropertyChangeListener
{
    NodeHighlighting()
    :
    m_node(globals->get_props()->getNode("sim/highlighting/material", true /*create*/))
    {
        /* Create highlight StateSet. Based on
        simgear/simgear/scene/model/SGPickAnimation.cxx:sharedHighlightStateSet().
        */
        m_state_set = new osg::StateSet;

        osg::Texture2D* white = simgear::StateAttributeFactory::instance()->getWhiteTexture();
        m_state_set->setTextureAttributeAndModes(
                0,
                white,
                osg::StateAttribute::ON
                    | osg::StateAttribute::OVERRIDE
                    | osg::StateAttribute::PROTECTED
                );

        osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
        polygonOffset->setFactor(-1);
        polygonOffset->setUnits(-1);
        m_state_set->setAttribute(polygonOffset, osg::StateAttribute::OVERRIDE);
        m_state_set->setMode(
                GL_POLYGON_OFFSET_LINE,
                osg::StateAttribute::ON
                    | osg::StateAttribute::OVERRIDE
                );

        osg::PolygonMode* polygonMode = new osg::PolygonMode;
        polygonMode->setMode(
                osg::PolygonMode::FRONT_AND_BACK,
                osg::PolygonMode::FILL
                );
        m_state_set->setAttribute(polygonMode, osg::StateAttribute::OVERRIDE);

        osg::Material* material = new osg::Material;
        material->setColorMode(osg::Material::OFF);
        material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, 0));
        material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, 0));
        material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, 0));
        material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, 0));
        m_state_set->setAttribute(
                material,
                osg::StateAttribute::OVERRIDE
                    | osg::StateAttribute::PROTECTED
                );
        // The default shader has a colorMode uniform that mimics the
        // behavior of Material color mode.

        osg::Uniform* colorModeUniform = new osg::Uniform(osg::Uniform::INT, "colorMode");
        colorModeUniform->set(0); // MODE_OFF
        colorModeUniform->setDataVariance(osg::Object::STATIC);
        m_state_set->addUniform(
                colorModeUniform,
                osg::StateAttribute::OVERRIDE
                    | osg::StateAttribute::ON
                );
        
        /* Initialise highlight material properties to defaults if not already
        set. */
        // XXX Alpha < 1.0 in the diffuse material value is a signal to the
        // default shader to take the alpha value from the material value
        // and not the glColor. In many cases the pick animation geometry is
        // transparent, so the outline would not be visible without this hack.
        setRgbaDefault(m_node, "ambient", 0, 0, 0, 1);
        setRgbaDefault(m_node, "diffuse", 0.5, 0.5, 0.5, .95);
        setRgbaDefault(m_node, "emission", 0.75, 0.375, 0, 1);
        setRgbaDefault(m_node, "specular", 0.0, 0.0, 0.0, 0);
        
        /* Listen for changes to highlight material properties. */
        m_node->addChangeListener(this, true /*initial*/);
    }
    
    static void setRgbaDefault(SGPropertyNode* root, const std::string& path,
            double red, double green, double blue, double alpha)
    {
        setPropertyDefault(root, path + "/red", red);
        setPropertyDefault(root, path + "/green", green);
        setPropertyDefault(root, path + "/blue", blue);
        setPropertyDefault(root, path + "/alpha", alpha);
    }
    
    // Sets property value if property does not exist.
    static void setPropertyDefault(SGPropertyNode* root, const std::string& path, double default_value)
    {
        SGPropertyNode* property = root->getNode(path, false /*create*/);
        if (!property) root->setDoubleValue(path, default_value);
    }
    
    virtual void valueChanged(SGPropertyNode* node)
    {
        assert(m_state_set);
        osg::StateAttribute* material0 = m_state_set->getAttribute(osg::StateAttribute::MATERIAL);
        osg::Material* material = dynamic_cast<osg::Material*>(material0);
        assert(material);
        
        double red;
        double green;
        double blue;
        double alpha;
        
        red = m_node->getDoubleValue("ambient/red");
        green = m_node->getDoubleValue("ambient/green");
        blue = m_node->getDoubleValue("ambient/blue");
        alpha = m_node->getDoubleValue("ambient/alpha");
        material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(red, green, blue, alpha));
        
        red = m_node->getDoubleValue("diffuse/red");
        green = m_node->getDoubleValue("diffuse/green");
        blue = m_node->getDoubleValue("diffuse/blue");
        alpha = m_node->getDoubleValue("diffuse/alpha");
        material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(red, green, blue, alpha));
        
        red = m_node->getDoubleValue("emission/red");
        green = m_node->getDoubleValue("emission/green");
        blue = m_node->getDoubleValue("emission/blue");
        alpha = m_node->getDoubleValue("emission/alpha");
        material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(red, green, blue, alpha));
        
        red = m_node->getDoubleValue("ambient/red");
        green = m_node->getDoubleValue("ambient/green");
        blue = m_node->getDoubleValue("ambient/blue");
        alpha = m_node->getDoubleValue("ambient/alpha");
        material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(red, green, blue, alpha));
        
        red = m_node->getDoubleValue("specular/red");
        green = m_node->getDoubleValue("specular/green");
        blue = m_node->getDoubleValue("specular/blue");
        alpha = m_node->getDoubleValue("specular/alpha");
        material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(red, green, blue, alpha));
    }
    
    /* Highlight/un-highlight the specified node. */
    int highlight(osg::Node* node, bool highlight)
    {
        int ret = 0;
        osg::Group* group = node->asGroup();
        if (!group) return 0;
        auto it_group_stateset = m_group_to_old_state_set.find(group);
        if (it_group_stateset == m_group_to_old_state_set.end() && highlight)
        {
            m_group_to_old_state_set[group] = group->getStateSet();
            group->setStateSet(m_state_set.get());
            ret += 1;
        }
        if (it_group_stateset != m_group_to_old_state_set.end() && !highlight)
        {
            group->setStateSet(it_group_stateset->second);
            m_group_to_old_state_set.erase(it_group_stateset);
            ret += 1;
        }
        return ret;
    }
    
    /* Un-highlight all nodes. */
    int clearAll()
    {
        int ret = 0;
        for(;;)
        {
            if (m_group_to_old_state_set.empty()) break;
            auto& group_stateset = *m_group_to_old_state_set.begin();
            highlight(group_stateset.first, false);
            ret += 1;
        }
        return ret;
    }
    
    ~NodeHighlighting()
    {
        m_node->removeChangeListener(this);
    }
    
    SGPropertyNode_ptr m_node;
    
    /* We keep track of osg::StateSet's for groups that we have highlighted so
    that we can turn highlighting off by swapping the original stateset back
    in. */
    std::map<osg::ref_ptr<osg::Group>, osg::ref_ptr<osg::StateSet>>  m_group_to_old_state_set;

    /* The stateset that we use to highlight nodes. */
    osg::ref_ptr<osg::StateSet> m_state_set;
};


Highlight::Highlight()
{
}

Highlight::~Highlight()
{
}

void Highlight::shutdown()
{
    s_property_to_info.clear();

    s_node_to_properties.clear();
    s_dialog_to_properties.clear();
    s_keypress_to_properties.clear();
    s_menu_to_properties.clear();

    s_menu_to_dialog.clear();
    s_dialog_to_menus.clear();

    s_property_to_properties.clear();
    
    s_property_tree_items.clear();
    s_prop_enabled.reset();
}

void Highlight::init()
{
     s_node_highlighting.reset(new NodeHighlighting);
}

/* Waits until sim/fdm-initialized is true and then gets property associations
from the FDM using FGInterface::property_associations(). */
struct FdmInitialisedListener : SGPropertyChangeListener
{
    FdmInitialisedListener()
    :
    m_fdm_initialised(globals->get_props()->getNode("sim/fdm-initialized", true /*create*/))
    {
        m_fdm_initialised->addChangeListener(this, true /*initial*/);
    }
    void valueChanged(SGPropertyNode* node) override
    {
        if (m_fdm_initialised->getBoolValue())
        {
            SG_LOG(SG_GENERAL, SG_DEBUG, "Getting property associations from FDM");
            auto highlight = globals->get_subsystem<Highlight>();
            if (highlight)
            {
                auto fdmshell = globals->get_subsystem<FDMShell>();
                FGInterface* fginterface = fdmshell->getInterface();
                assert(fginterface);
                fginterface->property_associations(
                        [highlight](const std::string& from, const std::string& to)
                        {
                            SG_LOG(SG_GENERAL, SG_DEBUG, "fdm property association: " << from << " => " << to);
                            highlight->addPropertyProperty(from, to);
                        }
                        );
            }
            s_fdm_initialised_listener.reset();
        }
    }
    ~FdmInitialisedListener()
    {
        m_fdm_initialised->removeChangeListener(this);
    }
    SGPropertyNode_ptr m_fdm_initialised;
};

void Highlight::bind()
{
    s_prop_enabled = globals->get_props()->getNode("sim/highlighting/enabled", true);
    globals->get_props()->setStringValue("sim/highlighting/current", "");
    globals->get_props()->setStringValue("sim/highlighting/current-ptr", "/sim/highlighting/current");
    
    /* Get property associations from FDM when sim/fdm-initialized becomes true. */
    s_fdm_initialised_listener.reset(new FdmInitialisedListener);
}

void Highlight::reinit()
{
}

void Highlight::unbind()
{
}

void Highlight::update(double dt)
{
}

/* Avoid ambiguities caused by possible use of "[0]" within property paths. */
static std::string canonical(const std::string property)
{
    return simgear::strutils::replace(property, "[0]", "");
}


int Highlight::highlightNodes(osg::Node* node)
{
    if (!s_output_stats)
    {
        s_output_stats = true;
        SG_LOG(SG_GENERAL, SG_DEBUG, "Sizes of internal maps:"
                << " s_property_to_info=" << s_property_to_info.size()
                << " s_node_to_properties=" << s_node_to_properties.size()
                << " s_dialog_to_properties=" << s_dialog_to_properties.size()
                << " s_keypress_to_properties=" << s_keypress_to_properties.size()
                << " s_menu_to_properties=" << s_menu_to_properties.size()
                << " s_menu_to_dialog=" << s_menu_to_dialog.size()
                << " s_dialog_to_menus=" << s_dialog_to_menus.size()
                << " s_property_to_properties=" << s_property_to_properties.size()
                << " s_property_from_properties=" << s_property_from_properties.size()
                << " s_property_tree_items=" << s_property_tree_items.size()
                );
    }
    if (!s_node_highlighting) return 0;
    
    s_node_highlighting->clearAll();
    
    int num_props = 0;
    
    /* We build up a list of name:value items such as:
    
        'dialog':   joystick-config
        'menu':     0/6: File/Joystick Configuration
        'property': /controls/flight/rudder
        'property': /surface-positions/rudder-pos-norm
    
    These items are written into /sim/highlighting/current/. For items with
    name='property', we highlight nodes which are animated by the specified
    property. */
    std::set<NameValue> items;
    
    if (s_prop_enabled->getBoolValue())
    {
        /* As well as <node> itself, we also highlight other nodes by following
        a limited part of the tree of property dependencies:

            property3
                -> property1
                    -> <node>
                    -> other nodes
                    -> property2
                        -> other nodes
                -> other nodes
                -> property4
                    -> other nodes
        */
        for (auto& property1: Highlight::findNodeProperties(node))
        {
            /* <property1> animates <node>. */
            items.insert(NameValue("property", property1));
            
            for (auto& property2: Highlight::findPropertyToProperties(property1))
            {
                /* <property2> is set by <property1> (which animates <node>). */
                items.insert(NameValue("property", property2));
            }
            
            for (auto& property3: Highlight::findPropertyFromProperties(property1))
            {
                /* <property3> sets <property1> (which animates <node>). */
                items.insert(NameValue("property", property3));
                
                for (auto& property4: Highlight::findPropertyToProperties(property3))
                {
                    /* <property4> is set by <property3> (which also
                    sets <property1>, which animates <node>). */
                    items.insert(NameValue("property", property4));
                }
            }
        }
        
        /* Highlight all nodes animated by properties in <items>, and add
        nodes, dialogs, menus and keypress info to <items>. */
        for (auto& nv: items)
        {
            const std::string& property = nv.value;
            SG_LOG(SG_GENERAL, SG_DEBUG, "Looking at property=" << property);
            const HighlightInfo& info = Highlight::findPropertyInfo(property);
            for (auto& node: info.nodes)
            {
                num_props += s_node_highlighting->highlight(node, true);
            }
            for (auto& dialog: info.dialogs)
            {
                items.insert(NameValue("dialog", dialog));
                for (auto& menu: Highlight::findMenuFromDialog(dialog))
                {
                    items.insert(NameValue("menu", menu.description()));
                }
            }
            for (auto& keypress: info.keypresses)
            {
                items.insert(NameValue("keypress", keypress));
            }
            for (auto& menu: info.menus)
            {
                items.insert(NameValue("menu", menu.description()));
            }
        }
    }
    else
    {
        num_props = -1;
    }
    
    if (items.empty())
    {
        /* Create dummy item to allow property viewer to be opened on
        sim/highlighting/current even if it would otherwise be currently empty.
        */
        items.insert(NameValue("dummy", ""));
    }
    
    /* Update property tree representation of currently-highlighted properties
    and associated dialogs, keypresses and menus, so that it matches
    <items>. We do this by calculating what items need adding or removing from
    the property tree representation */
    std::set<NameValue>   properties_new;
    std::set<NameValue>   properties_removed;
    std::set_difference(
            items.begin(), items.end(),
            s_property_tree_items.begin(), s_property_tree_items.end(),
            std::inserter(properties_new, properties_new.begin())
            );
    std::set_difference(
            s_property_tree_items.begin(), s_property_tree_items.end(),
            items.begin(), items.end(),
            std::inserter(properties_removed, properties_removed.begin())
            );
    SGPropertyNode* current = globals->get_props()->getNode("sim/highlighting/current", true /*create*/);
    for (int pos=0; pos<current->nChildren(); ++pos)
    {
        SGPropertyNode* child = current->getChild(pos);
        auto nv = NameValue(child->getNameString(), child->getStringValue());
        if (properties_removed.find(nv) != properties_removed.end())
        {
            current->removeChild(pos);
            pos -= 1;
        }
    }
    for (auto& name_value: properties_new)
    {
        current->addChild(name_value.name)->setStringValue(name_value.value);
    }
    
    /* Remember the current items for next time. */
    std::swap(items, s_property_tree_items);
    
    return num_props;
}

/* Functions that interrogate our internal data.


We use these to avoid adding multiple empty items to our std::maps.
*/
static const HighlightInfo info_empty;
static const std::set<std::string> set_string_empty;
static const std::set<HighlightMenu> set_menu_empty;

const HighlightInfo& Highlight::findPropertyInfo(const std::string& property)
{
    std::string property2 = canonical(property);
    auto it = s_property_to_info.find(property2);
    if (it == s_property_to_info.end()) return info_empty;
    return it->second;
}

const std::set<std::string>& Highlight::findNodeProperties(osg::Node* node)
{
    auto it = s_node_to_properties.find(node);
    if (it == s_node_to_properties.end()) return set_string_empty;
    return it->second;
}

const std::set<std::string>& Highlight::findDialogProperties(const std::string& dialog)
{
    auto it = s_dialog_to_properties.find(dialog);
    if (it == s_dialog_to_properties.end()) return set_string_empty;
    return it->second;
}

const std::set<std::string>& Highlight::findKeypressProperties(const std::string& keypress)
{
    auto it = s_keypress_to_properties.find(keypress);
    if (it == s_keypress_to_properties.end()) return set_string_empty;
    return it->second;
}

const std::set<std::string>& Highlight::findMenuProperties(const HighlightMenu& menu)
{
    auto it = s_menu_to_properties.find(menu);
    if (it == s_menu_to_properties.end()) return set_string_empty;
    return it->second;
}

const std::set<std::string>& Highlight::findPropertyToProperties(const std::string& property)
{
    std::string property2 = canonical(property);
    auto it = s_property_to_properties.find(property2);
    if (it == s_property_to_properties.end()) return set_string_empty;
    return it->second;
}

const std::set<std::string>& Highlight::findPropertyFromProperties(const std::string& property)
{
    std::string property2 = canonical(property);
    auto it = s_property_from_properties.find(property2);
    if (it == s_property_from_properties.end()) return set_string_empty;
    return it->second;
}

const std::set<HighlightMenu>& Highlight::findMenuFromDialog(const std::string& dialog)
{
    auto it = s_dialog_to_menus.find(dialog);
    if (it == s_dialog_to_menus.end()) return set_menu_empty;
    return it->second;
}


/* Functions that populate our internal data. */

void Highlight::addPropertyNode(const std::string& property, osg::ref_ptr<osg::Node> node)
{
    std::string property2 = canonical(property);
    s_property_to_info[property2].nodes.insert(node);
    if (s_node_to_properties[node].insert(property2).second)
        SG_LOG(SG_GENERAL, SG_DEBUG, "node=" << node.get() << " property=" << property2);
}

void Highlight::addPropertyDialog(const std::string& property, const std::string& dialog)
{
    std::string property2 = canonical(property);
    s_property_to_info[property2].dialogs.insert(dialog);
    if (s_dialog_to_properties[dialog].insert(property2).second)
        SG_LOG(SG_GENERAL, SG_DEBUG, "dialog=" << dialog << " property=" << property2);
}

void Highlight::addPropertyKeypress(const std::string& property, const std::string& keypress)
{
    std::string property2 = canonical(property);
    s_property_to_info[property2].keypresses.insert(keypress);
    if (s_keypress_to_properties[keypress].insert(property2).second)
        SG_LOG(SG_GENERAL, SG_DEBUG, "keypress=" << keypress << " property=" << property2);
}

void Highlight::addPropertyMenu(HighlightMenu menu, const std::string& property)
{
    std::string property2 = canonical(property);
    s_property_to_info[property2].menus.insert(menu);
    if (s_menu_to_properties[menu].insert(property2).second)
        SG_LOG(SG_GENERAL, SG_DEBUG, "menu=(" << menu.menu << " " << menu.item << ") property=" << property2);
}

void Highlight::addMenuDialog(HighlightMenu menu, const std::string& dialog)
{
    s_menu_to_dialog[menu] = dialog;
    if (s_dialog_to_menus[dialog].insert(menu).second)
        SG_LOG(SG_GENERAL, SG_DEBUG, "menu (" << menu.menu << " " << menu.item << ") dialog=" << dialog);
}

void Highlight::addPropertyProperty(const std::string& from0, const std::string& to0)
{
    std::string from = canonical(from0);
    std::string to = canonical(to0);
    if (from == to) return;
    bool is_new = false;
    if (s_property_to_properties[from].insert(to).second) is_new = true;
    if (s_property_from_properties[to].insert(from).second) is_new = true;
    if (is_new)
    {
        // Add transitive associations.
        for (auto& toto: s_property_to_properties[to])
        {
            Highlight::addPropertyProperty(from, toto);
        }
        for (auto& fromfrom: s_property_from_properties[from])
        {
            Highlight::addPropertyProperty(fromfrom, to);
        }
    }
}

// Register the subsystem.
SGSubsystemMgr::Registrant<Highlight> registrantHighlight(
    SGSubsystemMgr::INIT);
