#include "FGCocoaMenuBar.hxx"

#include <Cocoa/Cocoa.h>

#include <boost/foreach.hpp>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGBinding.hxx>

#include <Main/fg_props.hxx>

#include <iostream>

using std::string;
using std::map;
using std::cout;

typedef std::map<NSMenuItem*, SGBindingList> MenuItemBindings;

@class CocoaMenuDelegate;

class FGCocoaMenuBar::CocoaMenuBarPrivate
{
public:
  CocoaMenuBarPrivate();
  ~CocoaMenuBarPrivate();
  
  bool labelIsSeparator(const std::string& s) const;  
  void menuFromProps(NSMenu* menu, SGPropertyNode* menuNode);
  
  void fireBindingsForItem(NSMenuItem* item);
  
public:
  CocoaMenuDelegate* delegate;
  
  MenuItemBindings itemBindings;
};


@interface CocoaMenuDelegate : NSObject <NSMenuDelegate> {
@private
  FGCocoaMenuBar::CocoaMenuBarPrivate* peer;
}

@property (nonatomic, assign) FGCocoaMenuBar::CocoaMenuBarPrivate* peer;
@end

@implementation CocoaMenuDelegate

@synthesize peer;

- (void) itemAction:(id) sender
{
  peer->fireBindingsForItem((NSMenuItem*) sender);
}

@end

static NSString* stdStringToCocoa(const string& s)
{
  return [NSString stringWithUTF8String:s.c_str()];
}

class EnabledListener : public SGPropertyChangeListener
{
public:
  EnabledListener(NSMenuItem* i) :
    item(i)
  {}
  
  
  virtual void valueChanged(SGPropertyNode *node) 
  {
    BOOL b = node->getBoolValue();
    [item setEnabled:b];
  }
  
private:
  NSMenuItem* item;
};



FGCocoaMenuBar::CocoaMenuBarPrivate::CocoaMenuBarPrivate()
{
  delegate = [[CocoaMenuDelegate alloc] init];
  delegate.peer = this;
}
  
FGCocoaMenuBar::CocoaMenuBarPrivate::~CocoaMenuBarPrivate()
{
  [delegate release];
}
  
bool FGCocoaMenuBar::CocoaMenuBarPrivate::labelIsSeparator(const std::string& s) const
{
  for (unsigned int i=0; i<s.size(); ++i) {
    if (s[i] != '-') {
      return false;
    }
  }
  
  return true;
}
  
void FGCocoaMenuBar::CocoaMenuBarPrivate::menuFromProps(NSMenu* menu, SGPropertyNode* menuNode)
{
  int index = 0;
  BOOST_FOREACH(SGPropertyNode_ptr n, menuNode->getChildren("item")) {
    if (!n->hasValue("enabled")) {
      n->setBoolValue("enabled", true);
    }
    
    string l = n->getStringValue("label");
    string::size_type pos = l.find("(");
    if (pos != string::npos) {
      l = l.substr(0, pos);
    }
    
    NSString* label = stdStringToCocoa(l);
    NSString* shortcut = @"";
    NSMenuItem* item;
    if (index >= [menu numberOfItems]) {
      if (labelIsSeparator(l)) {
        item = [NSMenuItem separatorItem];
        [menu addItem:item];
      } else {        
        item = [menu addItemWithTitle:label action:nil keyEquivalent:shortcut];
        n->getNode("enabled")->addChangeListener(new EnabledListener(item));
        [item setTarget:delegate];
        [item setAction:@selector(itemAction:)];
      }
    } else {
      item = [menu itemAtIndex:index];
      [item setTitle:label]; 
    }
    
    BOOL enabled = n->getBoolValue("enabled");
    [item setEnabled:enabled];
    
    SGBindingList bl;
    BOOST_FOREACH(SGPropertyNode_ptr binding, n->getChildren("binding")) {
    // have to clone the bindings, since SGBinding takes ownership of the
    // passed in node. Seems like something is wrong here, but following the
    // PUI code for the moment.
      SGPropertyNode* cloned(new SGPropertyNode);
      copyProperties(binding, cloned);
      bl.push_back(new SGBinding(cloned, globals->get_props()));
    }
    
    itemBindings[item] = bl;    
    ++index;
  } // of item iteration
}

void FGCocoaMenuBar::CocoaMenuBarPrivate::fireBindingsForItem(NSMenuItem *item)
{
  MenuItemBindings::iterator it = itemBindings.find(item);
  if (it == itemBindings.end()) {
    return;
  }

  BOOST_FOREACH(SGSharedPtr<SGBinding> b, it->second) {
    b->fire();
  }
}

FGCocoaMenuBar::FGCocoaMenuBar() :
  p(new CocoaMenuBarPrivate)
{
  
}

FGCocoaMenuBar::~FGCocoaMenuBar()
{
  
}

void FGCocoaMenuBar::init()
{
  NSMenu* mainBar = [[NSApplication sharedApplication] mainMenu];
  SGPropertyNode_ptr props = fgGetNode("/sim/menubar/default",true);
  
  int index = 0;
  NSMenuItem* previousMenu = [mainBar itemAtIndex:0];
  if (![[previousMenu title] isEqualToString:@"FlightGear"]) {
    [previousMenu setTitle:@"FlightGear"];
  }
  
  BOOST_FOREACH(SGPropertyNode_ptr n, props->getChildren("menu")) {
    NSString* label = stdStringToCocoa(n->getStringValue("label"));
    NSMenuItem* item = [mainBar itemWithTitle:label];
    NSMenu* menu;
    
    if (!item) {
      NSInteger insertIndex = [mainBar indexOfItem:previousMenu] + 1; 
      item = [mainBar insertItemWithTitle:label action:nil keyEquivalent:@"" atIndex:insertIndex];
      item.tag = index + 400;
      
      menu = [[NSMenu alloc] init];
      menu.title = label;
      [menu setAutoenablesItems:NO];
      [mainBar setSubmenu:menu forItem:item];
      [menu autorelease];
    } else {
      menu = item.submenu;
    }
    
  // synchronise menu with properties
    p->menuFromProps(menu, n);
    ++index;
    previousMenu = item;
  }
}

bool FGCocoaMenuBar::isVisible() const
{
  return true;
}

void FGCocoaMenuBar::show()
{
  // no-op
}

void FGCocoaMenuBar::hide()
{
  // no-op
}
