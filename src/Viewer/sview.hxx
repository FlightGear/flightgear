#pragma once

/*
Support for extra view windows using 'Step' views system.
*/

#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/props/props.hxx>

#include <osgViewer/View>


/* Should be called before the first call to SviewCreate() so that
SviewCreate() can create new simgear::compositor::Compositor instances with the
same parameters as were used for the main window.

options
compositor_path
    Suitable for passing to simgear::compositor::Compositor().
*/
void SViewSetCompositorParams(
        osg::ref_ptr<simgear::SGReaderWriterOptions> options,
        const std::string& compositor_path
        );

/* Pushes current main window view to internal circular list of two items used
by SviewCreate() with 'last_pair' or 'last_pair_double'. */
void SviewPush();

/* Updates camera position/orientation/zoom of all sviews - should be called
each frame. Will also handle closing of Sview windows. */
void SviewUpdate(double dt);

/* Deletes all internal views; e.g. used when restarting. */
void SviewClear();

/* A view, typically an extra view window. The definition of this is not
public. */
struct SviewView;


/*
Creates a new SviewView in a new top-level window. It will be updated as
required by SviewUpdate() and can be dragged, resized
and closed by the user.

As of 2020-12-09, if not specified in *config, the new window will be half
width and height of the main window, and will have top-left corner at (100,
100).
            

Returns:
    Shared ptr to SviewView instance. As of 2020-11-18 there is little that
    the caller can do with this. We handle closing of the SviewView's window
    internally.

As of 2020-11-17, extra views have various limitations including:

    No key event handling, so no zooming with x/X.
    
config:
    width, height:
        Size of new window. Defaults are half main window width and height.
    x, y:
        Position of new window.
    type:
        "sview" or not specified:
            List of sview-step's - see below.
        "current"
            Clones the current view.
        "last_pair"
            Look from first pushed view's eye to second pushed view's
            eye. Returns nullptr if SviewPush hasn't been called at least
            twice.
        "last_pair_double"
            Keep first pushed view's aircraft in foreground and second pushed
            view's aircraft in background. Returns nullptr if SviewPush hasn't
            been called at least twice.
        "legacy":
            view:
                A legacy <view>...</view> tree, e.g. deep copy of /sim/view[]
                or /ai/models/multiplayer[]/set/sim/view[].
                
                Note that agl damping_time is calculated as log(10) /
                view/config/lookat-agl-damping, for backwards compatibility
                with legacy view code.
            callsign:
                "" if user aircraft, else multiplayer aircraft's callsign.
            view-number-raw:
                From /sim/current-view/view-number-raw. Used to examine
                /sim/view[]/config/eye-*-deg-path to decide which of aircraft's
                heading, pitch and roll to preserve in helicopter and chase
                views etc.
            direction-delta: initial modifications to direction.
                heading
                pitch
                roll
            zoom-delta: modification to default zoom.
            
            Chase view seems to be slightly different from the real legacy
            view, possibly because the viewpoint correction config/y-offset-m
            is applied after rotation instead of before.

An sview-step is:
    step:
        type:
            "aircraft"
                callsign:
                    "": The user's aircraft.
                    Otherwise specifies a multiplayer aircraft.
            "move":
                forward
                up
                right
                    Fixed values defining the move relative to current
                    direction.
            "direction-multiply":
                heading, pitch, roll:
                    Values to multiply into direction. This can be used to
                    preserve/not preserve heading, pitch and roll, which allows
                    implementation of Helicopter and Chase views etc.
            "copy-to-target":
                Copy current position into target. This creates an implicit
                final step that rotates the view to point at this target.
            "mouse-drag":
                Modify heading and pitch in response to mouse drags.
                heading-scale:
                pitch-scale:
                    Defaults to 1. For example use zero to disable or -1 to
                    reverse response.
            "nearest-tower":
                Move to position of the nearest tower to the aircraft.
            "rotate":
                heading, pitch, roll:
                    Values used to rotate direction.
                damping-time-heading, damping-time-pitch, damping-time-roll:
                    Damping times for heading, pitch and roll; zero gives no
                    damping. See 'agl' below for details of damping.
            "rotate-current-view":
                Rotate view direction by current /sim/current-view/....
            "double":
                A double view, with eye in foreground and target in background.
                chase-distance:
                    Distance to move from eye.
                angle:
                    Angle to maintain between eye and target.
            "agl":
                Tilt and zoom to keep ground immediately below the aircraft
                visible.
                callsign:
                    Use chase distance of specified aircraft as a measure of
                    its size, to ensure entire aircraft is visible.
                damping-time:
                    Ground level is damped as dx/dt =
                    (x_actual-x)/damping_time, and damping_time is the time in
                    seconds for the damped value to change by a factor of e
                    (2.71...).

Examples:

    Note that as of 2020-12-09 these examples are untested.

    Tower view of user aircraft:

        <window-width>300</window-width>
        <window-height>200</window-height>
        <window-x>100</window-x>
        <window-y>100</window-y>
        <step>  <!-- Move to aircraft. -->
            <type>aircraft</type>
            <callsign></callsign>
        </step>

        <step>  <!-- Move to centre of aircraft. -->
            <type>move</type>
            <right>0</right>
            <up>0.5</up>
            <forward>-3.85</forward>
        </step>

        <step>  <!-- Copy current position to target. -->
            <type>copy-to-target</type>
        </step>

        <step>  <!-- Move to nearest tower. -->
            <type>nearest-tower</type>
        </step>

    Helicopter view:
    
        <step>  <!-- Move to aircraft. -->
            <type>aircraft</type>
            <callsign>foobar</callsign>
        </step>

        <step>  <!-- Move to centre of aircraft. -->
            <type>move</type>
            <right>0</right>
            <up>0.5</up>
            <forward>-3.85</forward>
        </step>

        <step>  <!-- Force some direction values to zero. -->
            <type>direction-multiply</type>
            <heading>1</heading> <!-- Preserve aircraft heading. -->
            <pitch>0</pitch> <!-- Don't follow aircraft pitch. -->
            <roll>0</roll> <!-- Don't follow aircraft roll. -->
        </step>

        <step>  <!-- Add constants to heading, pitch and roll. -->
            <type>rotate</type>
            <heading>123</heading>  <!-- initial view heading -->
            <pitch>-10</pitch> <!-- initial view pitch -->
        </step>

        <step>  <!-- Move back from aircraft. -->
            <type>move</type>
            <forward>-25</forward>
        </step>
*/
std::shared_ptr<SviewView> SviewCreate(const SGPropertyNode* config);


#include <simgear/scene/viewer/Compositor.hxx>

/* If event is for an Sview window, returns the window's Compositor; otherwise
returns nullptr. */
simgear::compositor::Compositor* SviewGetEventViewport(const osgGA::GUIEventAdapter& ea);

/* If event is for an Sview window, handles it and returns true. Otherwise
returns false. */
bool SviewMouseMotion(int x, int y, const osgGA::GUIEventAdapter& ea);

