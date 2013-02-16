#include "FGCocoaMenuBar.hxx"

#include <Cocoa/Cocoa.h>

#include <boost/foreach.hpp>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGBinding.hxx>
#include <simgear/misc/strutils.hxx>

#include <Main/fg_props.hxx>

#include <iostream>

using std::string;
using std::map;
using std::cout;
using namespace simgear;

typedef std::map<NSMenuItem*, SGBindingList> MenuItemBindings;

@class CocoaMenuDelegate;

class FGCocoaMenuBar::CocoaMenuBarPrivate
{
public:
  CocoaMenuBarPrivate();
  ~CocoaMenuBarPrivate();
  
  void menuFromProps(NSMenu* menu, SGPropertyNode* menuNode);
  
  void fireBindingsForItem(NSMenuItem* item);
  
public:
  CocoaMenuDelegate* delegate;
  
  MenuItemBindings itemBindings;
};

// prior to the 10.6 SDK, NSMenuDelegate was an informal protocol
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1060
@protocol NSMenuDelegate <NSObject>
@end
#endif

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

static void setFunctionKeyShortcut(NSMenuItem* item, unichar shortcut)
{
  unichar ch[1];
  ch[0] = shortcut;
  [item setKeyEquivalentModifierMask:NSFunctionKeyMask];
  [item setKeyEquivalent:[NSString stringWithCharacters:ch length:1]];
  
}

static void setItemShortcutFromString(NSMenuItem* item, const string& s)
{
  const char* shortcut = "";
  
  bool hasCtrl = strutils::starts_with(s, "Ctrl-"); 
  bool hasShift = strutils::starts_with(s, "Shift-");
  bool hasAlt = strutils::starts_with(s, "Alt-");
  
  int offset = 0; // character offset from start of string
  if (hasShift) offset += 6;
  if (hasCtrl) offset += 5;
  if (hasAlt) offset += 4;
  
  shortcut = s.c_str() + offset;
  if (!strcmp(shortcut, "Esc"))
    shortcut = "\e";    
  
  if (!strcmp(shortcut, "F11")) {
    setFunctionKeyShortcut(item, NSF11FunctionKey);
    return;
  }
  
  if (!strcmp(shortcut, "F12")) {
    setFunctionKeyShortcut(item, NSF12FunctionKey);
    return;
  }
  
  [item setKeyEquivalent:[NSString stringWithCString:shortcut encoding:NSUTF8StringEncoding]];
  NSUInteger modifiers = 0;
  if (hasCtrl) modifiers |= NSControlKeyMask;
  if (hasShift) modifiers |= NSShiftKeyMask;
  if (hasAlt) modifiers |= NSAlternateKeyMask;
  
  [item setKeyEquivalentModifierMask:modifiers];
}

namespace {
  class CocoaAutoreleasePool
  {
  public:
    CocoaAutoreleasePool()
    {
      pool = [[NSAutoreleasePool alloc] init];
    }
    
    ~CocoaAutoreleasePool()
    {
      [pool release];
    }
    
  private:
    NSAutoreleasePool* pool;
  };
  
  class CocoaEnabledListener : public SGPropertyChangeListener
  {
  public:
    CocoaEnabledListener(NSMenuItem* i) :
      item(i)
    {}
    
    
    virtual void valueChanged(SGPropertyNode *node) 
    {
      CocoaAutoreleasePool pool;
      BOOL b = node->getBoolValue();
      [item setEnabled:b];
    }
    
  private:
    NSMenuItem* item;
  };
} // of anonymous namespace

FGCocoaMenuBar::CocoaMenuBarPrivate::CocoaMenuBarPrivate()
{
  delegate = [[CocoaMenuDelegate alloc] init];
  delegate.peer = this;
}
  
FGCocoaMenuBar::CocoaMenuBarPrivate::~CocoaMenuBarPrivate()
{
  CocoaAutoreleasePool pool;
  [delegate release];
}
  
static bool labelIsSeparator(NSString* s)
{
  return [s hasPrefix:@"---"];
}
  
void FGCocoaMenuBar::CocoaMenuBarPrivate::menuFromProps(NSMenu* menu, SGPropertyNode* menuNode)
{
  int index = 0;
  BOOST_FOREACH(SGPropertyNode_ptr n, menuNode->getChildren("item")) {
    if (!n->hasValue("enabled")) {
      n->setBoolValue("enabled", true);
    }
    
    string l = getLocalizedLabel(n);
    NSString* label = stdStringToCocoa(strutils::simplify(l));
    string shortcut = n->getStringValue("key");
    
    NSMenuItem* item;
    if (index >= [menu numberOfItems]) {
      if (labelIsSeparator(label)) {
        item = [NSMenuItem separatorItem];
        [menu addItem:item];
      } else {        
        item = [menu addItemWithTitle:label action:nil keyEquivalent:@""];
        if (!shortcut.empty()) {
          setItemShortcutFromString(item, shortcut);
        }
        
        n->getNode("enabled")->addChangeListener(new CocoaEnabledListener(item));
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
  CocoaAutoreleasePool pool;
  
  NSMenu* mainBar = [[NSApplication sharedApplication] mainMenu];
  SGPropertyNode_ptr props = fgGetNode("/sim/menubar/default",true);
  
  int index = 0;
  NSMenuItem* previousMenu = [mainBar itemAtIndex:0];
  if (![[previousMenu title] isEqualToString:@"FlightGear"]) {
    [previousMenu setTitle:@"FlightGear"];
  }
  
  BOOST_FOREACH(SGPropertyNode_ptr n, props->getChildren("menu")) {
    NSString* label = stdStringToCocoa(getLocalizedLabel(n));
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
    
  // track menu enable/disable state
    if (!n->hasValue("enabled")) {
      n->setBoolValue("enabled", true);
    }
    
    n->getNode("enabled")->addChangeListener(new CocoaEnabledListener(item));
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

void cocoaOpenUrl(const std::string& url)
{
  CocoaAutoreleasePool pool;
  NSURL* nsu = [NSURL URLWithString:stdStringToCocoa(url)];
  [[NSWorkspace sharedWorkspace] openURL:nsu];
}
