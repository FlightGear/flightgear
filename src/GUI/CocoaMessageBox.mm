
#include <string>

#include <Cocoa/Cocoa.h>
#include <GUI/CocoaAutoreleasePool.hxx>
#include <GUI/MessageBox.hxx>

static NSString* stdStringToCocoa(const std::string& s)
{
  return [NSString stringWithUTF8String:s.c_str()];
}

flightgear::MessageBoxResult cocoaMessageBox(const std::string& msg,
                                             const std::string& text)
{
    CocoaAutoreleasePool pool;
    NSAlert* alert = [NSAlert alertWithMessageText:stdStringToCocoa(msg)
                                     defaultButton:nil /* localized 'ok' */
                                   alternateButton:nil
                                       otherButton:nil
                         informativeTextWithFormat:@"%@",stdStringToCocoa(text)];
    [[alert retain] autorelease];
    [alert runModal];
    return flightgear::MSG_BOX_OK;
}



flightgear::MessageBoxResult cocoaFatalMessage(const std::string& msg,
                                               const std::string& text)
{
    CocoaAutoreleasePool pool;
    NSAlert* alert = [NSAlert alertWithMessageText:stdStringToCocoa(msg)
                                     defaultButton:@"Quit FlightGear"
                                   alternateButton:nil
                                       otherButton:nil
                         informativeTextWithFormat:@"%@", stdStringToCocoa(text)];
    [[alert retain] autorelease];
    [alert runModal];
    return flightgear::MSG_BOX_OK;
}

