// FGHIDEventInput.cxx -- handle event driven input devices via HIDAPI
//
// Written by James Turner
//
// Copyright (C) 2017, James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "config.h"

#include "FGHIDEventInput.hxx"

#include <cstdlib>
#include <cassert>
#include <algorithm>

#include <Main/fg_props.hxx>
#include <hidapi/hidapi.h>
#include <hidapi/hidparse.h>

#include <simgear/structure/exception.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/misc/strutils.hxx>
#include <simgear/io/lowlevel.hxx>

const char* hexTable = "0123456789ABCDEF";

namespace HID
{
    enum class UsagePage
    {
        Undefined = 0x00,
        GenericDesktop = 0x01,
        Simulation = 0x02,
        VR = 0x03,
        Sport =0x04,
        Game =0x05,
        GenericDevice = 0x06,
        Keyboard = 0x07,
        LEDs = 0x08,
        Button = 0x09,
        Ordinal = 0x0A,
        Telephony = 0x0B,
        Consumer = 0x0C,
        Digitizer = 0x0D,
        // reserved 0x0E
        PID = 0x0F,
        Unicode = 0x10,
        AlphanumericDisplay = 0x14,
        MedicalInstruments = 0x40,
        BarCodeScanner = 0x8C,
        MagneticStripeReadingDevice = 0x8E,
        CameraControl = 0x90,
        Arcade = 0x91,
        VendorDefinedStart = 0xFF00
    };

    enum SimulationUsage
    {
        SC_FlightSimulationDevice = 0x01,
        SC_AutomobileSimulationDevice = 0x02,
        SC_TankSimulationDevice = 0x03,
        SC_SpaceShipSimulationDevice = 0x04,
        SC_SubmarineSimulationDevice = 0x05,
        SC_SailingSimulationDevice = 0x06,
        SC_MotorcycleSimulationDevice = 0x07,
        SC_SportsSimulationDevice = 0x08,
        SC_AirplaneSimulationDevice = 0x09,
        SC_HelicopterSimulationDevice = 0x0A,
        SC_MagicCarpetSimulationDevice = 0x0B,
        SC_BycicleSimulationDevice = 0x0C,
        SC_FlightControlStick = 0x20,
        SC_FlightStick = 0x21,
        SC_CyclicControl = 0x22,
        SC_CyclicTrim = 0x23,
        SC_FlightYoke = 0x24,
        SC_TrackControl = 0x25,
        SC_Aileron = 0xB0,
        SC_AileronTrim = 0xB1,
        SC_AntiTorqueControl = 0xB2,
        SC_AutopilotEnable = 0xB3,
        SC_ChaffRelease = 0xB4,
        SC_CollectiveControl = 0xB5,
        SC_DiveBrake = 0xB6,
        SC_ElectronicCountermeasures = 0xB7,
        SC_Elevator = 0xB8,
        SC_ElevatorTrim = 0xB9,
        SC_Rudder = 0xBA,
        SC_Throttle = 0xBB,
        SC_FlightCommunications = 0xBC,
        SC_FlareRelease = 0xBD,
        SC_LandingGear = 0xBE,
        SC_ToeBrake = 0xBF,
        SC_Trigger = 0xC0,
        SC_WeaponsArm = 0xC1,
        SC_WeaponsSelect = 0xC2,
        SC_WingFlaps = 0xC3,
        SC_Accelerator = 0xC4,
        SC_Brake = 0xC5,
        SC_Clutch = 0xC6,
        SC_Shifter = 0xC7,
        SC_Steering = 0xC8,
        SC_TurretDirection = 0xC9,
        SC_BarrelElevation = 0xCA,
        SC_DivePlane = 0xCB,
        SC_Ballast = 0xCC,
        SC_BicycleCrank = 0xCD,
        SC_HandleBars= 0xCE,
        SC_FrontBrake = 0xCF,
        SC_RearBrake = 0xD0

    };


    enum GenericDesktopUsage
    {
        // generic desktop section
        GD_Undefined= 0x00,
        GD_Pointer = 0x01,
        GD_Mouse = 0x02,
        GD_Reserved03 = 0x03,
        GD_Joystick = 0x04,
        GD_GamePad = 0x05,
        GD_Keyboard = 0x06,
        GD_Keypad = 0x07,
        GD_MultiAxisController = 0x08,
        GD_TabletPCSysCtrls = 0x09,
        GD_WaterCoolingDevice = 0x0A,
        GD_ComputerChassisDevice = 0x0B,
        GD_WirelessRadioControls = 0x0C,
        GD_PortableDeviceControl = 0x0D,
        GD_SystemMultiAxisController = 0x0E,
        GD_SpatialController = 0x0F,
        GD_AssistiveControl = 0x10,
        GD_DeviceDock = 0x11,
        GD_DockableDevice = 0x12,
        GD_CallStateManagementControl = 0x13,
        GD_Reserved14 = 0x14,
        GD_Reserved15 = 0x15,
        GD_Reserved16 = 0x16,
        GD_Reserved17 = 0x17,
        GD_Reserved18 = 0x18,
        GD_Reserved19 = 0x19,
        GD_Reserved1A = 0x1A,
        GD_Reserved1B = 0x1B,
        GD_Reserved1C = 0x1C,
        GD_Reserved1D = 0x1D,
        GD_Reserved1E = 0x1E,
        GD_Reserved1F = 0x1F,
        GD_Reserved20 = 0x20,
        GD_Reserved21 = 0x21,
        GD_Reserved22 = 0x22,
        GD_Reserved23 = 0x23,
        GD_Reserved24 = 0x24,
        GD_Reserved25 = 0x25,
        GD_Reserved26 = 0x26,
        GD_Reserved27 = 0x27,
        GD_Reserved28 = 0x28,
        GD_Reserved29 = 0x29,
        GD_Reserved2A = 0x2A,
        GD_Reserved2B = 0x2B,
        GD_Reserved2C = 0x2C,
        GD_Reserved2D = 0x2D,
        GD_Reserved2E = 0x2E,
        GD_Reserved2F = 0x2F,
        GD_X = 0x30,
        GD_Y = 0x31,
        GD_Z = 0x32,
        GD_Rx = 0x33,
        GD_Ry = 0x34,
        GD_Rz = 0x35,
        GD_Slider = 0x36,
        GD_Dial = 0x37,
        GD_Wheel = 0x38,
        GD_Hatswitch = 0x39,
        GD_CountedBuffer= 0x3A,
        GD_ByteCount = 0x3B,
        GD_MotionWakeUp = 0x3C,
        GD_Start = 0x3D,
        GD_Select = 0x3E,
        GD_Reserved3F = 0x3F,
        GD_Vx = 0x40,
        GD_Vy = 0x41,
        GD_Vz = 0x42,
        GD_Vbrx = 0x43,
        GD_Vbry = 0x44,
        GD_Vbrz = 0x45,
        GD_Vno = 0x46,
        GD_FeatureNotification = 0x47,
        GD_ResolutionMultiplier = 0x48,
        GD_Qx = 0x49,
        GD_Qy = 0x4A,
        GD_Qz = 0x4B,
        GD_Qw = 0x4C,
        GD_SystemControl = 0x80,
        GD_SystemPowerDown = 0x81,
        GD_SystemSleep = 0x82,
        GD_SystemWakeUp = 0x83,
        GD_SystemContextMenu = 0x84,
        GD_SystemMainMenu = 0x85,
        GD_SystemAppMenu = 0x86,
        GD_SystemMenuHelp = 0x87,
        GD_SystemMenuExit = 0x88,
        GD_SystemMenuSelect = 0x89,
        GD_SystemMenuRight = 0x8A,
        GD_SystemMenuLeft = 0x8B,
        GD_SystemMenuUp= 0x8C,
        GD_SystemMenuDown = 0x8D,
        GD_SystemColdRestart = 0x8E,
        GD_SystemWarmRestart = 0x8F,
        GD_DpadUp = 0x90,
        GD_DpadDown = 0x91,
        GD_DpadRight = 0x92,
        GD_DpadLeft = 0x93,
        GD_IndexTrigger = 0x94,
        GD_PalmTrigger = 0x95,
        GD_Thumbstick = 0x96,
        GD_SystemFunctionShift = 0x97,
        GD_SystemFunctionShiftLock = 0x98,
        GD_SystemFunctionShiftLockIndicator = 0x99,
        GD_SystemDismissNotification = 0x9A,
        GD_SystemDoNotDisturb = 0x9B,
        GD_SystemDock = 0xA0,
        GD_SystemUndock = 0xA1,
        GD_SystemSetup  = 0xA2,
        GD_SystemBreak = 0xA3,
        GD_SystemDebuggerBreak = 0xA4,
        GD_ApplicationBreak = 0xA5,
        GD_ApplicationDebuggerBreak = 0xA6,
        GD_SystemSpeakerMute = 0xA7,
        GD_SystemHibernate = 0xA8,
        GD_SystemMicrophoneMute = 0xA9,
        GD_SystemDisplayInvert = 0xB0,
        GD_SystemDisplayInternal = 0xB1,
        GD_SystemDisplayExternal = 0xB2,
        GD_SystemDisplayBoth = 0xB3,
        GD_SystemDisplayDual = 0xB4,
        GD_SystemDisplayToggleIntExtMode = 0xB5,
        GD_SystemDisplaySwapPrimarySecondary = 0xB6,
        GD_SystemDisplayToggleLCDAutoscale = 0xB7,
        GD_SensorZone = 0xC0,
        GD_RPM= 0xC1,
        GD_CoolantLevel = 0xC2,
        GD_CoolantCriticalLevel = 0xC3,
        GD_CoolantPump = 0xC4,
        GD_ChassisEnclosure = 0xC5,
        GD_WirelessRadioButton = 0xC6,
        GD_WirelessRadioLED = 0xC7,
        GD_WirelessRadioSliderSwitch = 0xC8,
        GD_SystemDisplayRotationLockButton = 0xC9,
        GD_SystemDisplayRotationLockSliderSwitch = 0xCA,
        GD_ControlEnable = 0xCB,
        GD_DockableDeviceUniqueID = 0xD0,
        GD_DockableDeviceVendorID = 0xD1,
        GD_DockableDevicePrimaryUsagePage = 0xD2,
        GD_DockableDevicePrimaryUsageID = 0xD3,
        GD_DockableDeviceDockingState = 0xD4,
        GD_DockableDeviceDisplayOcclusion = 0xD5,
        GD_DockableDeviceObjectType = 0xD6,
        GD_CallActiveLED = 0xE0,
        GD_CallMuteToggle = 0xE1,
        GD_CallMuteLED = 0xE2
    };

    enum LEDUsage
    {
        LED_Undefined = 0,
        LED_NumLock = 0x01,
        LED_CapsLock = 0x02,
        LED_ScrollLock = 0x03,
        LED_Compose = 0x04,
        LED_Kana = 0x05,
        LED_Power = 0x06,
        LED_Shift = 0x07,
        LED_DoNotDisturb = 0x08,
        LED_Mute = 0x09,
        LED_ToneEnable = 0x0A,
        LED_HighCutFilter = 0x0B,
        LED_LowCutFilter = 0x0C,
        LED_EqualizerEnable = 0x0D,
        LED_SoundFieldOn = 0x0E,
        LED_SurroundOn = 0x0F,
        LED_Repeat = 0x10,
        LED_Stereo = 0x11,
        LED_SampligRateDetect = 0x12,
        LED_Spinning = 0x013,
        LED_CAV = 0x14,
        LED_CLV = 0x15,
        LED_RecordingFormatDetect = 0x16,
        LED_OffHook = 0x17,
        LED_Ring = 0x18,
        LED_MessageWaiting= 0x19,
        LED_DataMode = 0x1A,
        LED_BatteryOperation = 0x1B,
        LED_BatteryOk = 0x1C,
        LED_BatteryLow = 0x1D,
        LED_Speaker = 0x1E,
        LED_HeadSet = 0x1F,
        LED_Hold = 0x20,
        LED_Microphone = 0x21,
        LED_Coverage = 0x22,
        LED_NightMode = 0x23,
        LED_SendCalls = 0x24,
        LED_CallPickup = 0x25,
        LED_Conference = 0x26,
        LED_StandBy = 0x27,
        LED_CameraOn = 0x28,
        LED_CameraOff = 0x29,
        LED_OnLine = 0x2A,
        LED_OffLine = 0x2B,
        LED_Busy = 0x2C,
        LED_Ready = 0x2D,
        LED_PaperOut = 0x2E,
        LED_PaperJam = 0x2F,
        LED_Remote = 0x30,
        LED_Forward = 0x31,
        LED_Reverse = 0x32,
        LED_Stop = 0x33,
        LED_Rewind = 0x34,
        LED_FastForward = 0x35,
        LED_Play = 0x36,
        LED_Pause = 0x37,
        LED_Record = 0x38,
        LED_Error = 0x39,
        LED_UsageSelectedIndicator = 0x3A,
        LED_UsageInUseIndicator = 0x3B,
        LED_UsageMultiModeIndicator = 0x3C,
        LED_IndicatorOn = 0x3D,
        LED_IndicatorFlash= 0x3E,
        LED_IndicatorSlowBlink = 0x3F,
        LED_IndicatorFastBlink = 0x40,
        LED_IndicatorOff = 0x41,
        LED_FlashOnTime = 0x42,
        LED_SlowBlinkOnTime = 0x43,
        LED_SlowBlinkOffTime = 0x44,
        LED_FastBlinkOnTime = 0x45,
        LED_FastBlinkOfftime = 0x46,
        LED_UsageIndicatorColor = 0x47,
        LED_IndicatorRed = 0x48,
        LED_IndicatorGreen = 0x49,
        LED_IndicatorAmber = 0x4A,
        LED_GenericIndicator = 0x4B,
        LED_SystemSuspend = 0x4C,
        LED_ExternalPowerConnected = 0x4D
    };

    enum AlphanumericDisplayUsage
    {
        AD_AlphanumericDisplay = 0x01,
        AD_BitmappedDisplay = 0x02,
        AD_DisplayControlReport = 0x24,
        AD_ClearDisplay = 0x25,
        AD_CharacterReport = 0x2B,
        AD_DisplayData = 0x2C,
        AD_DisplayStatus = 0x2D,
        AD_Rows = 0x35,
        AD_Columns = 0x36,
        AD_7SegmentDirectMap = 0x43,
        AD_14SegmentDirectMap = 0x45,
        AD_DisplayBrightness = 0x46,
        AD_DisplayContrast = 0x47
    };

    enum VRUsage
    {
        VR_Undefined = 0x00,
        VR_Belt = 0x01,
        VR_BodySuit = 0x02,
        VR_Flexor = 0x03,
        VR_Glove = 0x04,
        VR_HeadTracker = 0x05,
        VR_HeadMountedDisplay = 0x06,
        VR_HandTracker = 0x07,
        VR_Oculometer = 0x08,
        VR_Vest = 0x09,
        VR_AnimatronicDevice = 0x0A,
        VR_StereoEnable = 0x20,
        VR_DisplayEnable = 0x21
    };

    enum class ReportType
    {
        Invalid = 0,
        In = 0x08,
        Out = 0x09,
        Feature = 0x0B
    };

    std::string nameForUsage(uint32_t usagePage, uint32_t usage)
    {
        const auto enumUsage = static_cast<UsagePage>(usagePage);
        if (enumUsage == UsagePage::Undefined) {
          std::stringstream os;
          os << "undefined-" << usage;
          return os.str();
        }

        if (enumUsage == UsagePage::GenericDesktop) {
            switch (usage) {
                case GD_Undefined:          return "undefined";
                case GD_Pointer:            return "pointer";
                case GD_Mouse:              return "mouse";
                case GD_Reserved03:           return "reserved03";
                case GD_GamePad:             return "gamepad";
                case GD_Keyboard:            return "keyboard";
                case GD_Keypad:              return "keypad";
                case GD_Joystick:            return "joystick";
                case GD_Wheel:               return "wheel";
                case GD_Dial:                return "dial";
                case GD_Hatswitch:           return "hat";
                case GD_Slider:              return "slider";
                case GD_Rx:                  return "x-rotate";
                case GD_Ry:                  return "y-rotate";
                case GD_Rz:                  return "z-rotate";
                case GD_X:                   return "x-translate";
                case GD_Y:                   return "y-translate";
                case GD_Z:                   return "z-translate";
                case GD_WaterCoolingDevice:  return "watercoolingdevice";
                case GD_MultiAxisController: return "multiaxiscontroller";
                case GD_TabletPCSysCtrls:    return "tabletpcsysctrls";
                case GD_CountedBuffer:       return "countedbuffer";
                case GD_ByteCount:           return "bytecount";
                case GD_MotionWakeUp:        return "motionwakeup";
                case GD_Start:               return "start";
                case GD_Select:              return "select";
                case GD_Vx:                  return "x-vector";
                case GD_Vy:                  return "y-vector";
                case GD_Vz:                  return "z-vector";
                case GD_Vbrx:                return "relative-x-vector";
                case GD_Vbry:                return "relative-y-vector";
                case GD_Vbrz:                return "relative-z-vector";
                case GD_Vno:                 return "non-oriented-vector";
                case GD_DpadUp:              return "direction-pad-up";
                case GD_DpadDown:            return "direction-pad-down";
                case GD_DpadRight:           return "direction-pad-right";
                case GD_DpadLeft:            return "direction-pad-left";
                case GD_ComputerChassisDevice: return "computerchassisdevice";
                case GD_WirelessRadioControls: return "wirelessradiocontrols";
                case GD_PortableDeviceControl: return "portabledevicecontrol";
                case GD_SystemMultiAxisController: return "systemmultiaxiscontroller";
                case GD_SpatialController:     return "spatialcontroller";
                case GD_AssistiveControl:    return "assistivecontrol";
                case GD_DeviceDock:          return "devicedock";
                case GD_DockableDevice:      return "dockabledevice";
                case GD_CallStateManagementControl: return "callstatemanagementcontrol";
                case GD_FeatureNotification: return "featurenotification";
                case GD_ResolutionMultiplier: return "resolutionmultiplier";
                case GD_Qx:                   return "qx";
                case GD_Qy:                   return "qy";
                case GD_Qz:                   return "qz";
                case GD_Qw:                   return "qw";
                case GD_SystemControl:        return "systemcontrol";
                case GD_SystemPowerDown:      return "systempowerdown";
                case GD_SystemSleep:          return "systemsleep";
                case GD_SystemWakeUp:         return "systemwakeup";
                case GD_SystemContextMenu:    return "systemcontextmenu";
                case GD_SystemMainMenu:       return "systemmainmenu";
                case GD_SystemAppMenu:        return "systemappmenu";
                case GD_SystemMenuHelp:       return "systemmenuhelp";
                case GD_SystemMenuExit:       return "systemmenuexit";
                case GD_SystemMenuSelect:     return "systemmenuselect";
                case GD_SystemMenuRight:      return "systemmenuright";
                case GD_SystemMenuLeft:       return "systemmenuleft";
                case GD_SystemMenuUp:         return "systemmenuup";
                case GD_SystemMenuDown:       return "systemmenudown";
                case GD_SystemColdRestart:    return "systemcoldrestart";
                case GD_SystemWarmRestart:    return "systemwarmrestart";
                case GD_IndexTrigger:         return "indextrigger";
                case GD_PalmTrigger:          return "palmtrigger";
                case GD_Thumbstick:           return "thumbstick";
                case GD_SystemFunctionShift:  return "systemfunctionshift";
                case GD_SystemFunctionShiftLock:  return "systemfunctinshiftlock";
                case GD_SystemFunctionShiftLockIndicator: return "systemfunctionshiftlockindicator";
                case GD_SystemDismissNotification: return "systemdismissnotification";
                case GD_SystemDoNotDisturb:    return "systemdonotdisturb";
                case GD_SystemDock:            return "systemdock";
                case GD_SystemUndock:          return "systemundock";
                case GD_SystemSetup:           return "systemsetup";
                case GD_SystemBreak:           return "systembreak";
                case GD_SystemDebuggerBreak:   return "systemdebuggerbreak";
                case GD_ApplicationBreak:      return "applicationbreak";
                case GD_ApplicationDebuggerBreak: return "applicationdebuggerbreak";
                case GD_SystemSpeakerMute:     return "systemspeakermute";
                case GD_SystemHibernate:       return "systemhibernate";
                case GD_SystemMicrophoneMute:  return "systemmicrophonemute";
                case GD_SystemDisplayInvert:   return "systemdisplayinvert";
                case GD_SystemDisplayInternal: return "systemdisplayinternal";
                case GD_SystemDisplayExternal: return "systemdisplayexternal";
                case GD_SystemDisplayBoth:     return "systemdisplayboth";
                case GD_SystemDisplayDual:     return "systemdisplaydual";
                case GD_SystemDisplayToggleIntExtMode: return "systemdisplaytoggleintextmode";
                case GD_SystemDisplaySwapPrimarySecondary: return "systemdisplayswapprimarysecondary";
                case GD_SystemDisplayToggleLCDAutoscale: return "systemdisplaytogglelcdautoscale";
                case GD_SensorZone:            return "SENSORZONE";
                case GD_RPM:                   return "rpm";
                case GD_CoolantLevel:          return "coolantlevel";
                case GD_CoolantCriticalLevel:  return "coolantcriticallevel";
                case GD_CoolantPump:           return "coolant";
                case GD_ChassisEnclosure:      return "chassisenclosure";
                case GD_WirelessRadioButton:   return "wirelessradiobutton";
                case GD_WirelessRadioLED:      return "wirelessradioled";
                case GD_WirelessRadioSliderSwitch: return "wirelessradiosliderswitch";
                case GD_SystemDisplayRotationLockButton: return "systemdisplayrotationlockbutton";
                case GD_SystemDisplayRotationLockSliderSwitch: return "systemdisplayrotationlocksliderswitch";
                case GD_ControlEnable:         return "controlenable";
                case GD_DockableDeviceUniqueID: return "dockabledeviceuniqueid";
                case GD_DockableDeviceVendorID: return "dockabledevicevendorid";
                case GD_DockableDevicePrimaryUsagePage: return "dockabledeviceprimaryusagepage";
                case GD_DockableDevicePrimaryUsageID: return "dockabledeviceprimaryusageid";
                case GD_DockableDeviceDockingState: return "dockabledevicedockingstate";
                case GD_DockableDeviceDisplayOcclusion: return "dockabledevicedisplayocclusion";
                case GD_DockableDeviceObjectType: return "dockabledeviceobjecttype";
                case GD_CallActiveLED: return "callactiveled";
                case GD_CallMuteToggle: return "callmutetoggle";
                case GD_CallMuteLED: return "callmuteled";
                case GD_Reserved14: return "reserved14";
                case GD_Reserved15: return "reserved15";
                case GD_Reserved16: return "reserved16";
                case GD_Reserved17: return "reserved17";
                case GD_Reserved18: return "reserved18";
                case GD_Reserved19: return "reserved19";
                case GD_Reserved1A: return "reserved1a";
                case GD_Reserved1B: return "reserved1b";
                case GD_Reserved1C: return "reserved1c";
                case GD_Reserved1D: return "reserved1d";
                case GD_Reserved1E: return "reserved1e";
                case GD_Reserved1F: return "reserved1f";
                case GD_Reserved20: return "reserved20";
                case GD_Reserved21: return "reserved21";
                case GD_Reserved22: return "reserved22";
                case GD_Reserved23: return "reserved23";
                case GD_Reserved24: return "reserved24";
                case GD_Reserved25: return "reserved25";
                case GD_Reserved26: return "reserved26";
                case GD_Reserved27: return "reserved27";
                case GD_Reserved28: return "reserved28";
                case GD_Reserved29: return "reserved29";
                case GD_Reserved2A: return "reserved2a";
                case GD_Reserved2B: return "reserved2b";
                case GD_Reserved2C: return "reserved2c";
                case GD_Reserved2D: return "reserved2d";
                case GD_Reserved2E: return "reserved2e";
                case GD_Reserved2F: return "reserved2f";
                case GD_Reserved3F: return "reserved3f";

                default:
                    SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID generic desktop usage:" << usage);
            }
        } else if (enumUsage == UsagePage::Simulation) {
            switch (usage) {
                case SC_FlightSimulationDevice:                 return "flightsimulationdevice";
                case SC_AutomobileSimulationDevice:             return "AutomobileSimulationDevice";
                case SC_TankSimulationDevice:                   return "tanksimulationdevice";
                case SC_SpaceShipSimulationDevice:              return "spaceshipsimulationdevice";
                case SC_SubmarineSimulationDevice:              return "submarinesimulationdevice";
                case SC_SailingSimulationDevice:                return "sailingsimulationdevice";
                case SC_MotorcycleSimulationDevice:             return "motorcyclesimulationdevice";
                case SC_SportsSimulationDevice:                 return "sportssimulationdevice";
                case SC_AirplaneSimulationDevice:               return "airplanesimulationdevice";
                case SC_HelicopterSimulationDevice:             return "helicoptersimulationdevice";
                case SC_MagicCarpetSimulationDevice:            return "magiccarpetsimulationdevice";
                case SC_BycicleSimulationDevice:                return "byciclesimulationdevice";
                case SC_FlightControlStick:                     return "flightcontrolstick";
                case SC_FlightStick:                            return "flightstick";
                case SC_CyclicControl:                          return "cycliccontrol";
                case SC_CyclicTrim:                             return "cyclictrim";
                case SC_FlightYoke:                             return "flightyoke";
                case SC_TrackControl:                           return "trackcontrol";
                case SC_Aileron:                                return "aileron";
                case SC_AileronTrim:                            return "ailerontrim";
                case SC_AntiTorqueControl:                      return "antitorquecontrol";
                case SC_AutopilotEnable:                        return "autopilotenable";
                case SC_ChaffRelease:                           return "chaffrelease";
                case SC_CollectiveControl:                      return "collectivecontrol";
                case SC_DiveBrake:                              return "divebrake";
                case SC_ElectronicCountermeasures:              return "electroniccountermeasures";
                case SC_Elevator:                               return "elevator";
                case SC_ElevatorTrim:                           return "elevatortrim";
                case SC_Rudder:                                 return "rudder";
                case SC_Throttle:                               return "throttle";
                case SC_FlightCommunications:                   return "flightcommunications";
                case SC_FlareRelease:                           return "flarerelease";
                case SC_LandingGear:                            return "landinggear";
                case SC_ToeBrake:                               return "toebrake";
                case SC_Trigger:                                return "trigger";
                case SC_WeaponsArm:                             return "weaponsarm";
                case SC_WeaponsSelect:                          return "weaponsselect";
                case SC_WingFlaps:                              return "wingsflap";
                case SC_Accelerator:                            return "accelerator";
                case SC_Brake:                                  return "brake";
                case SC_Clutch:                                 return "clutch";
                case SC_Shifter:                                return "shifter";
                case SC_Steering:                               return "steering";
                case SC_TurretDirection:                        return "turretdirection";
                case SC_BarrelElevation:                        return "barrelelevation";
                case SC_DivePlane:                              return "diveplane";
                case SC_Ballast:                                return "balast";
                case SC_BicycleCrank:                           return "bicyclehandle";
                case SC_HandleBars:                              return "handlebars";
                case SC_FrontBrake:                             return "frontbrake";
                case SC_RearBrake:                              return "rearbrake";
                default:
                    SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID simulation usage:" << usage);
            }
        } else if (enumUsage == UsagePage::Consumer) {
            switch (usage) {
                default:
                    SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID consumer usage:" << usage);
            }
        } else if (enumUsage == UsagePage::AlphanumericDisplay) {
            switch (usage) {
                case AD_AlphanumericDisplay:       return "alphanumeric";
                case AD_CharacterReport:           return "character-report";
                case AD_DisplayData:               return "display-data";
                case AD_DisplayBrightness:         return "display-brightness";
                case AD_7SegmentDirectMap:          return "seven-segment-direct";
                case AD_14SegmentDirectMap:         return "fourteen-segment-direct";

                default:
                    SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID alphanumeric usage:" << usage);
            }
        } else if (enumUsage == UsagePage::AlphanumericDisplay) {
            switch (usage) {
                case VR_Undefined:          return "undefined-vr";
                case VR_Belt:               return "belt-vr";
                case VR_BodySuit:           return "bodysuit-vr";
                case VR_Flexor:             return "flexor-vr";
                case VR_Glove:              return "glove-vr";
                case VR_HeadTracker:        return "headtracker-vr";
                case VR_HeadMountedDisplay: return "headmounteddisplay-vr";
                case VR_HandTracker:        return "handtracker-vr";
                case VR_Oculometer:         return "oculometer-vr";
                case VR_Vest:               return "vest-vr";
                case VR_AnimatronicDevice:  return "animatronicdevice-vr";
                case VR_StereoEnable:       return "stereoenable-vr";
                case VR_DisplayEnable:      return "displayenable-vr";
            default:
                SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID VR usage:" << usage);
            }
        } else if (enumUsage == UsagePage::LEDs) {
            switch (usage) {
                case LED_Undefined:                 return "undefined-led";
                case LED_NumLock:                   return "numlock-led";
                case LED_CapsLock:                  return "capslock-led";
                case LED_ScrollLock:                return "scrolllock-led";
                case LED_Compose:                   return "compose-led";
                case LED_Kana:                      return "kana-led";
                case LED_Power:                     return "power-led";
                case LED_Shift:                     return "shift-led";
                case LED_DoNotDisturb:              return "donotdisturb-led";
                case LED_Mute:                      return "mute-led";
                case LED_ToneEnable:                return "toneenable-led";
                case LED_HighCutFilter:             return "highcutfilter-led";
                case LED_LowCutFilter:              return "lowcutfilter-led";
                case LED_EqualizerEnable:           return "equalizerenable-led";
                case LED_SoundFieldOn:              return "soundfieldon-led";
                case LED_SurroundOn:                return "surroundon-led";
                case LED_Repeat:                    return "repeat-led";
                case LED_Stereo:                    return "stereo-led";
                case LED_SampligRateDetect:         return "samplingratedetect-led";
                case LED_Spinning:                  return "spinning-led";
                case LED_CAV:                       return "cav-led";
                case LED_CLV:                       return "clv-led";
                case LED_RecordingFormatDetect:     return "recordingformatdetect-led";
                case LED_OffHook:                   return "offhook-led";
                case LED_Ring:                      return "ring-led";
                case LED_MessageWaiting:            return "messagewaiting-led";
                case LED_DataMode:                  return "datamode-led";
                case LED_BatteryOperation:          return "batteryoperation-led";
                case LED_BatteryOk:                 return "batteryok-led";
                case LED_BatteryLow:                return "batterylow-led";
                case LED_Speaker:                   return "speaker-led";
                case LED_HeadSet:                   return "headset-led";
                case LED_Hold:                      return "hold-led";
                case LED_Microphone:                return "microphone-led";
                case LED_Coverage:                  return "coverage-led";
                case LED_NightMode:                 return "nightmode-led";
                case LED_SendCalls:                 return "sendcalls-led";
                case LED_CallPickup:                return "callpickup-led";
                case LED_Conference:                return "conference-led";
                case LED_StandBy:                   return "standby-led";
                case LED_CameraOn:                  return "cameraon-led";
                case LED_CameraOff:                 return "cameraoff-led";
                case LED_OnLine:                    return "online-led";
                case LED_OffLine:                   return "offline-led";
                case LED_Busy:                      return "busy-led";
                case LED_Ready:                     return "ready-led";
                case LED_PaperOut:                  return "paperout-led";
                case LED_PaperJam:                  return "paperjam-led";
                case LED_Remote:                    return "remote-led";
                case LED_Forward:                   return "forward-led";
                case LED_Reverse:                   return "reverse-led";
                case LED_Stop:                      return "stop=led";
                case LED_Rewind:                    return "rewind-led";
                case LED_FastForward:               return "fastforward-led";
                case LED_Play:                      return "play-led";
                case LED_Pause:                     return "pause-led";
                case LED_Record:                    return "record-led";
                case LED_Error:                     return "error-led";
                case LED_UsageSelectedIndicator:    return "usageselectedindicator-led";
                case LED_UsageInUseIndicator:       return "usageinuseindicator-led";
                case LED_UsageMultiModeIndicator:   return "usagemultimodeindicator-led";
                case LED_IndicatorOn:               return "indicatoron-led";
                case LED_IndicatorFlash:            return "idicatorflash-led";
                case LED_IndicatorSlowBlink:        return "indicatorslowblink-led";
                case LED_IndicatorFastBlink:        return "indicatorfastblink-led";
                case LED_IndicatorOff:              return "indicatoroff-led";
                case LED_FlashOnTime:               return "flashontime-led";
                case LED_SlowBlinkOnTime:           return "slowblinkontime-led";
                case LED_SlowBlinkOffTime:          return "slowblinkofftime-led";
                case LED_FastBlinkOnTime:           return "fastblinkontime-led";
                case LED_FastBlinkOfftime:          return "fastblinkofftime-led";
                case LED_UsageIndicatorColor:       return "usageindicatorcolor-led";
                case LED_IndicatorRed:              return "usageindicatorred-led";
                case LED_IndicatorGreen:            return "usageindicatorgreen-led";
                case LED_IndicatorAmber:            return "usageindicatoramber-led";
                case LED_GenericIndicator:          return "usagegenericindicator-led";
                case LED_SystemSuspend:             return "usagesystemsuspend-led";
                case LED_ExternalPowerConnected:    return "externalpowerconnected-led";
                default:
                    SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID LED usage:" << usage);

            }
        } else if (enumUsage == UsagePage::Button) {
            std::stringstream os;
            os << "button-" << usage;
            return os.str();
        } else if (enumUsage >= UsagePage::VendorDefinedStart) {
            return "vendor";
        } else {
            SG_LOG(SG_INPUT, SG_WARN, "Unhandled HID usage page:" << std::hex << usagePage
                   << " with usage " << std::hex << usage);
        }

        return "unknown";
    }

    bool shouldPrefixWithAbs(uint32_t usagePage, uint32_t usage)
    {
        const auto enumUsage = static_cast<UsagePage>(usagePage);
        if (enumUsage == UsagePage::GenericDesktop) {
            switch (usage) {
            case GD_Wheel:
            case GD_Dial:
            case GD_Hatswitch:
            case GD_Slider:
            case GD_Rx:
            case GD_Ry:
            case GD_Rz:
            case GD_X:
            case GD_Y:
            case GD_Z:
                return true;
            default:
                break;
            }
        }

        return false;
    }

    ReportType reportTypeFromString(const std::string& s)
    {
        if (s == "input") return ReportType::In;
        if (s == "output") return ReportType::Out;
        if (s == "feature") return ReportType::Feature;
        return ReportType::Invalid;
    }
} // of namespace

class FGHIDEventInput::FGHIDEventInputPrivate
{
public:
    FGHIDEventInput* p = nullptr;

    void evaluateDevice(hid_device_info* deviceInfo);
};

// anonymous namespace to define our device subclass
namespace
{

class FGHIDDevice : public FGInputDevice {
public:
    FGHIDDevice(hid_device_info* devInfo,
        FGHIDEventInput* subsys);

    virtual ~FGHIDDevice();

    bool Open() override;
    void Close() override;
    void Configure(SGPropertyNode_ptr node) override;

    void update(double dt) override;
    const char *TranslateEventName(FGEventData &eventData) override;
    void Send( const char * eventName, double value ) override;
    void SendFeatureReport(unsigned int reportId, const std::string& data) override;

    class Item
    {
    public:
        Item(const std::string& n, uint32_t offset, uint8_t size) :
            name(n),
            bitOffset(offset),
            bitSize(size)
        {}

        std::string name;
        uint32_t bitOffset = 0; // form the start of the report
        uint8_t bitSize = 1;
        bool isRelative = false;
        bool doSignExtend = false;
        int lastValue = 0;
        // int defaultValue = 0;
        // range, units, etc not needed for now
        // hopefully this doesn't need to be a list
        FGInputEvent_ptr event;
    };
private:
    class Report
    {
    public:
        Report(HID::ReportType ty, uint8_t n = 0) : type(ty), number(n) {}

        HID::ReportType type;
        uint8_t number = 0;
        std::vector<Item*> items;

        uint32_t currentBitSize() const
        {
            uint32_t size = 0;
            for (auto i : items) {
                size += i->bitSize;
            }
            return size;
        }
    };

    bool parseUSBHIDDescriptor();
    void parseCollection(hid_item* collection);
    void parseItem(hid_item* item);

    Report* getReport(HID::ReportType ty, uint8_t number, bool doCreate = false);

    void sendReport(Report* report) const;

    uint8_t countWithName(const std::string& name) const;
    std::pair<Report*, Item*> itemWithName(const std::string& name) const;

    void processInputReport(Report* report, unsigned char* data, size_t length,
                            double dt, int keyModifiers);

    int maybeSignExtend(Item* item, int inValue);

    void defineReport(SGPropertyNode_ptr reportNode);

    std::vector<Report*> _reports;
    std::string _hidPath;
    hid_device* _device = nullptr;
    bool _haveNumberedReports = false;
    bool _debugRaw = false;

    /// set if we parsed the device description our XML
    /// instead of from the USB data. Useful on Windows where the data
    /// is inaccessible, or devices with broken descriptors
    bool _haveLocalDescriptor = false;

    /// allow specifying the descriptor as hex bytes in XML
    std::vector<uint8_t>_rawXMLDescriptor;

    // all sets which will be send on the next update() call.
    std::set<Report*> _dirtyReports;
};

class HIDEventData : public FGEventData
{
public:
    // item, value, dt, keyModifiers
    HIDEventData(FGHIDDevice::Item* it, int value, double dt, int keyMods) :
        FGEventData(value, dt, keyMods),
        item(it)
    {
        assert(item);
    }

    FGHIDDevice::Item* item = nullptr;
};

FGHIDDevice::FGHIDDevice(hid_device_info *devInfo, FGHIDEventInput *)
{
    class_id = "FGHIDDevice";
    _hidPath = devInfo->path;

    std::wstring manufacturerName, productName;
    productName = devInfo->product_string ? std::wstring(devInfo->product_string)
                                          : L"unknown HID device";

    if (devInfo->manufacturer_string) {
        manufacturerName = std::wstring(devInfo->manufacturer_string);
        SetName(simgear::strutils::convertWStringToUtf8(manufacturerName) + " " +
            simgear::strutils::convertWStringToUtf8(productName));
    } else {
        SetName(simgear::strutils::convertWStringToUtf8(productName));
    }

    const auto serial = devInfo->serial_number;
    std::string path(devInfo->path);
    // most devices return an empty serial number, unfortunately
    if ((serial != nullptr) && std::wcslen(serial) > 0) {
        SetSerialNumber(simgear::strutils::convertWStringToUtf8(serial));
    }

    SG_LOG(SG_INPUT, SG_DEBUG, "HID device:" << GetName() << " at path " << _hidPath);
}

FGHIDDevice::~FGHIDDevice()
{
    if (_device) {
        hid_close(_device);
    }
}

void FGHIDDevice::Configure(SGPropertyNode_ptr node)
{
    // base class first
    FGInputDevice::Configure(node);

    if (node->hasChild("hid-descriptor")) {
        _haveLocalDescriptor = true;
        if (debugEvents) {
            SG_LOG(SG_INPUT, SG_INFO, GetUniqueName() << " will configure using local HID descriptor");
        }

        for (auto report : node->getChild("hid-descriptor")->getChildren("report")) {
            defineReport(report);
        }
    }

    if (node->hasChild("hid-raw-descriptor")) {
        _rawXMLDescriptor = simgear::strutils::decodeHex(node->getStringValue("hid-raw-descriptor"));
        if (debugEvents) {
            SG_LOG(SG_INPUT, SG_INFO, GetUniqueName() << " will configure using XML-defined raw HID descriptor");
        }
    }

    if (node->getBoolValue("hid-debug-raw")) {
        _debugRaw = true;
    }
}

bool FGHIDDevice::Open()
{
    SG_LOG(SG_INPUT, SG_INFO, "HID open " << GetUniqueName());
    _device = hid_open_path(_hidPath.c_str());
    if (_device == nullptr) {
        SG_LOG(SG_INPUT, SG_WARN, GetUniqueName() << ": HID: Failed to open:" << _hidPath);
        SG_LOG(SG_INPUT, SG_WARN, "\tnote on Linux you may need to adjust permissions of the device using UDev rules.");
        return false;
    }

#if !defined(SG_WINDOWS)
    if (_rawXMLDescriptor.empty()) {
        _rawXMLDescriptor.resize(2048);
        int descriptorSize = hid_get_descriptor(_device, _rawXMLDescriptor.data(), _rawXMLDescriptor.size());
        if (descriptorSize <= 0) {
           SG_LOG(SG_INPUT, SG_WARN, "HID: " << GetUniqueName() << " failed to read HID descriptor");
           return false;
        }

        _rawXMLDescriptor.resize(descriptorSize);
    }
#endif

    if (!_haveLocalDescriptor) {
        bool ok = parseUSBHIDDescriptor();
        if (!ok)
            return false;
    }

    for (auto& v : handledEvents) {
        auto reportItem = itemWithName(v.first);
        if (!reportItem.second) {
            SG_LOG(SG_INPUT, SG_WARN, "HID device:" << GetUniqueName() << " has no element for event:" << v.first);
            continue;
        }

        FGInputEvent_ptr event = v.second;
        if (debugEvents) {
            SG_LOG(SG_INPUT, SG_INFO, "\tfound item for event:" << v.first);
        }

        reportItem.second->event = event;
    }

    return true;
}

bool FGHIDDevice::parseUSBHIDDescriptor()
{
#if defined(SG_WINDOWS)
    if (_rawXMLDescriptor.empty()) {
        SG_LOG(SG_INPUT, SG_ALERT, GetUniqueName() << ": on Windows, there is no way to extract the UDB-HID report descriptor. "
               << "\nPlease supply the report descriptor in the device XML configuration.");
        SG_LOG(SG_INPUT, SG_ALERT, "See this page:<> for information on extracting the report descriptor on Windows");
        return false;
    }
#endif

    if (_debugRaw) {
        SG_LOG(SG_INPUT, SG_INFO, "\nHID: descriptor for:" << GetUniqueName());
        {
            std::ostringstream byteString;

            for (size_t i = 0; i < _rawXMLDescriptor.size(); ++i) {
                byteString << hexTable[_rawXMLDescriptor[i] >> 4];
                byteString << hexTable[_rawXMLDescriptor[i] & 0x0f];
                byteString << " ";
            }
            SG_LOG(SG_INPUT, SG_INFO, "\tbytes: " << byteString.str());
        }
    }

    hid_item* rootItem = nullptr;
    hid_parse_reportdesc(_rawXMLDescriptor.data(), _rawXMLDescriptor.size(), &rootItem);
    if (debugEvents) {
        SG_LOG(SG_INPUT, SG_INFO, "\nHID: scan for:" << GetUniqueName());
    }

    parseCollection(rootItem);

    hid_free_reportdesc(rootItem);
    return true;
}

void FGHIDDevice::parseCollection(hid_item* c)
{
    for (hid_item* child = c->collection; child != nullptr; child = child->next) {
        if (child->collection) {
            parseCollection(child);
        } else {
            // leaf item
            parseItem(child);
        }
    }
}

auto FGHIDDevice::getReport(HID::ReportType ty, uint8_t number, bool doCreate) -> Report*
{
    if (number > 0) {
        _haveNumberedReports = true;
    }

    for (auto report : _reports) {
        if ((report->type == ty) && (report->number == number)) {
            return report;
        }
    }

    if (doCreate) {
        auto r = new Report{ty, number};
        _reports.push_back(r);
        return r;
    } else {
        return nullptr;
    }
}

auto FGHIDDevice::itemWithName(const std::string& name) const -> std::pair<Report*, Item*>
{
    for (auto report : _reports) {
        for (auto item : report->items) {
            if (item->name == name) {
                return std::make_pair(report, item);
            }
        }
    }

    return std::make_pair(static_cast<Report*>(nullptr), static_cast<Item*>(nullptr));
}

uint8_t FGHIDDevice::countWithName(const std::string& name) const
{
    uint8_t result = 0;
    size_t nameLength = name.length();

    for (auto report : _reports) {
        for (auto item : report->items) {
            if (strncmp(name.c_str(), item->name.c_str(), nameLength) == 0) {
                result++;
            }
        }
    }

    return result;
}

void FGHIDDevice::parseItem(hid_item* item)
{
    std::string name = HID::nameForUsage(item->usage >> 16, item->usage & 0xffff);
    if (hid_parse_is_relative(item)) {
        name = "rel-" + name; // prefix relative names
    } else if (HID::shouldPrefixWithAbs(item->usage >> 16, item->usage & 0xffff)) {
        name = "abs-" + name;
    }

    const auto ty = static_cast<HID::ReportType>(item->type);
    auto existingItem = itemWithName(name);
    if (existingItem.second) {
        // type fixup
        const HID::ReportType existingItemType = existingItem.first->type;
        if (existingItemType != ty) {
            // might be an item named identically in input/output and feature reports
            //  -> prefix the feature one with 'feature'
            if (ty == HID::ReportType::Feature) {
                name = "feature-" + name;
            } else if (existingItemType == HID::ReportType::Feature) {
                // rename this existing item since it's a feature
                existingItem.second->name = "feature-" + name;
            }
        }
    }

    // do the count now, after we did any renaming, since we might have
    // N > 1 for the new name
    int existingCount = countWithName(name);
    if (existingCount > 0) {
        if (existingCount == 1) {
            // rename existing item 0 to have the "-0" suffix
            auto existingItem = itemWithName(name);
            existingItem.second->name += "-0";
        }

        // define the new nae
        std::stringstream os;
        os << name << "-" << existingCount;
        name = os.str();
    }

    auto report = getReport(ty, item->report_id, true /* create */);
    uint32_t bitOffset = report->currentBitSize();

    if (debugEvents) {
        SG_LOG(SG_INPUT, SG_INFO, GetUniqueName() << ": add:" << name << ", bits: " << bitOffset << ":" << (int) item->report_size
            << ", report=" << (int) item->report_id);
    }

    Item* itemObject = new Item{name, bitOffset, item->report_size};
    itemObject->isRelative = hid_parse_is_relative(item);
    itemObject->doSignExtend = (item->logical_min < 0) || (item->logical_max < 0);
    report->items.push_back(itemObject);
}

void FGHIDDevice::Close()
{
    if (_device) {
        hid_close(_device);
        _device = nullptr;
    }
}

void FGHIDDevice::update(double dt)
{
    if (!_device) {
        return;
    }

    uint8_t reportBuf[65];
    int readCount = 0;
    while (true) {
        readCount = hid_read_timeout(_device, reportBuf, sizeof(reportBuf), 0);

        if (readCount <= 0) {
            break;
        }

        int modifiers = fgGetKeyModifiers();
        const uint8_t reportNumber = _haveNumberedReports ? reportBuf[0] : 0;
        auto inputReport = getReport(HID::ReportType::In, reportNumber, false);
        if (!inputReport) {
            SG_LOG(SG_INPUT, SG_WARN, GetName() << ": FGHIDDevice: Unknown input report number:" <<
                static_cast<int>(reportNumber));
        } else {
            uint8_t* reportBytes = _haveNumberedReports ? reportBuf + 1 : reportBuf;
            size_t reportSize = _haveNumberedReports ? readCount -  1 : readCount;
            processInputReport(inputReport, reportBytes, reportSize, dt, modifiers);
        }
    }

    FGInputDevice::update(dt);

    for (auto rep : _dirtyReports) {
        sendReport(rep);
    }

    _dirtyReports.clear();
}

void FGHIDDevice::sendReport(Report* report) const
{
    if (!_device) {
        return;
    }

    uint8_t reportBytes[65];
    size_t reportLength = 0;
    memset(reportBytes, 0, sizeof(reportBytes));
    reportBytes[0] = report->number;

// fill in valid data
    for (auto item : report->items) {
        reportLength += item->bitSize;
        if (item->lastValue == 0) {
            continue;
        }

        writeBits(reportBytes + 1, item->bitOffset, item->bitSize, item->lastValue);
    }

    reportLength /= 8;

    if (_debugRaw) {
        std::ostringstream byteString;
        for (size_t i=0; i<reportLength; ++i) {
            byteString << hexTable[reportBytes[i] >> 4];
            byteString << hexTable[reportBytes[i] & 0x0f];
            byteString << " ";
        }
        SG_LOG(SG_INPUT, SG_INFO, "sending bytes: " << byteString.str());
    }


// send the data, based on the report type
    if (report->type == HID::ReportType::Feature) {
        hid_send_feature_report(_device, reportBytes, reportLength + 1);
    } else {
        assert(report->type == HID::ReportType::Out);
        hid_write(_device, reportBytes, reportLength + 1);
    }
}

int FGHIDDevice::maybeSignExtend(Item* item, int inValue)
{
    return item->doSignExtend ? signExtend(inValue, item->bitSize) : inValue;
}

void FGHIDDevice::processInputReport(Report* report, unsigned char* data,
                                     size_t length,
                                     double dt, int keyModifiers)
{
    if (_debugRaw) {
        SG_LOG(SG_INPUT, SG_INFO, GetName() << " FGHIDDeivce received input report:" << (int) report->number << ", len=" << length);
        {
            std::ostringstream byteString;
            for (size_t i=0; i<length; ++i) {
                byteString << hexTable[data[i] >> 4];
                byteString << hexTable[data[i] & 0x0f];
                byteString << " ";
            }
            SG_LOG(SG_INPUT, SG_INFO, "\tbytes: " << byteString.str());
        }
    }

    for (auto item : report->items) {
        int value = extractBits(data, length, item->bitOffset, item->bitSize);

        value = maybeSignExtend(item, value);

        // suppress events for values that aren't changing
        if (item->isRelative) {
            // supress spurious 0-valued relative events
            if (value == 0) {
                continue;
            }
        } else {
            // supress no-change events for absolute items
            if (value == item->lastValue) {
                continue;
            }
        }

        item->lastValue = value;
        if (!item->event)
            continue;

        if (_debugRaw) {
            SG_LOG(SG_INPUT, SG_INFO, "\titem:" << item->name << " = " << value);
        }

        HIDEventData event{item, value, dt, keyModifiers};
        HandleEvent(event);
    }
}

void FGHIDDevice::SendFeatureReport(unsigned int reportId, const std::string& data)
{
    if (!_device) {
        return;
    }

    if (_debugRaw) {
        SG_LOG(SG_INPUT, SG_INFO, GetName() << ": FGHIDDevice: Sending feature report:" << (int) reportId << ", len=" << data.size());
        {
            std::ostringstream byteString;

            for (unsigned int i=0; i<data.size(); ++i) {
                byteString << hexTable[data[i] >> 4];
                byteString << hexTable[data[i] & 0x0f];
                byteString << " ";
            }
            SG_LOG(SG_INPUT, SG_INFO, "\tbytes: " << byteString.str());
        }
    }

    uint8_t buf[65];
    size_t len = std::min(data.length() + 1, sizeof(buf));
    buf[0] = reportId;
    memcpy(buf + 1, data.data(), len - 1);
    size_t r = hid_send_feature_report(_device, buf, len);
    if (r < 0) {
        SG_LOG(SG_INPUT, SG_WARN, GetName() << ": FGHIDDevice: Sending feature report failed, error-string is:\n"
               << simgear::strutils::error_string(errno));
    }
}

const char *FGHIDDevice::TranslateEventName(FGEventData &eventData)
{
    HIDEventData& hidEvent = static_cast<HIDEventData&>(eventData);
    return hidEvent.item->name.c_str();
}

void FGHIDDevice::Send(const char *eventName, double value)
{
    auto item = itemWithName(eventName);
    if (item.second == nullptr) {
        SG_LOG(SG_INPUT, SG_WARN, GetName() << ": FGHIDDevice:unknown item name:" << eventName);
        return;
    }

    int intValue = static_cast<int>(value);
    if (item.second->lastValue == intValue) {
        return; // not actually changing
    }

    lastEventName->setStringValue(eventName);
    lastEventValue->setDoubleValue(value);

    // update the stored value prior to sending
    item.second->lastValue = intValue;
    _dirtyReports.insert(item.first);
}

void FGHIDDevice::defineReport(SGPropertyNode_ptr reportNode)
{
    const int nChildren = reportNode->nChildren();
    uint32_t bitCount = 0;
    const auto rty = HID::reportTypeFromString(reportNode->getStringValue("type"));
    if (rty == HID::ReportType::Invalid) {
        SG_LOG(SG_INPUT, SG_WARN, GetName() << ": FGHIDDevice: invalid report type:" <<
               reportNode->getStringValue("type"));
        return;
    }

    const auto id = reportNode->getIntValue("id");
    if (id > 0) {
        _haveNumberedReports = true;
    }

    auto report = new Report(rty, id);
    _reports.push_back(report);

    for (int c=0; c < nChildren; ++c) {
        const auto nd = reportNode->getChild(c);
        const int size = nd->getIntValue("size", 1); // default to a single bit
        if (nd->getNameString() == "unused-bits") {
            bitCount += size;
            continue;
        }

        if (nd->getNameString() == "type" || nd->getNameString() == "id") {
            continue; // already handled above
        }

        // allow repeating items
        uint8_t count = nd->getIntValue("count", 1);
        std::string name = nd->getNameString();
        const auto lastHypen = name.rfind("-");
        std::string baseName = name.substr(0, lastHypen + 1);
        int baseIndex = std::stoi(name.substr(lastHypen + 1));

        const bool isRelative = (name.find("rel-") == 0);
        const bool isSigned = nd->getBoolValue("is-signed", false);

        for (uint8_t i=0; i < count; ++i) {
            std::ostringstream oss;
            oss << baseName << (baseIndex + i);
            Item* itemObject = new Item{oss.str(), bitCount, static_cast<uint8_t>(size)};
            itemObject->isRelative = isRelative;
            itemObject->doSignExtend = isSigned;
            report->items.push_back(itemObject);
            bitCount += size;
        }
    }
}


} // of anonymous namespace


int extractBits(uint8_t* bytes, size_t lengthInBytes, size_t bitOffset, size_t bitSize)
{
    const size_t wholeBytesToSkip = bitOffset >> 3;
    const size_t offsetInByte = bitOffset & 0x7;

    // work out how many whole bytes to copy
    const size_t bytesToCopy = std::min(sizeof(uint32_t), (offsetInByte + bitSize + 7) / 8);
    uint32_t v = 0;
    // this goes from byte alignment to word alignment safely
    memcpy((void*) &v, bytes + wholeBytesToSkip, bytesToCopy);

    // shift down so lowest bit is aligned
    v = v >> offsetInByte;

    // mask off any extraneous top bits
    const uint32_t mask = ~(0xffffffff << bitSize);
    v &= mask;

    return v;
}

int signExtend(int inValue, size_t bitSize)
{
    const int m = 1U << (bitSize - 1);
    return (inValue ^ m) - m;
}

void writeBits(uint8_t* bytes, size_t bitOffset, size_t bitSize, int value)
{
    size_t wholeBytesToSkip = bitOffset >> 3;
    uint8_t* dataByte = bytes + wholeBytesToSkip;
    size_t offsetInByte = bitOffset & 0x7;
    size_t bitsInByte = std::min(bitSize, 8 - offsetInByte);
    uint8_t mask = 0xff >> (8 - bitsInByte);

    *dataByte |= ((value & mask) << offsetInByte);

    if (bitsInByte < bitSize) {
        // if we have more bits to write, recurse
        writeBits(bytes, bitOffset + bitsInByte, bitSize - bitsInByte, value >> bitsInByte);
    }
}

FGHIDEventInput::FGHIDEventInput() : FGEventInput("Input/HID", "/input/hid"),
                                     d(new FGHIDEventInputPrivate)
{
    d->p = this; // store back pointer to outer object on pimpl
}

FGHIDEventInput::~FGHIDEventInput()
{
}

void FGHIDEventInput::reinit()
{
    SG_LOG(SG_INPUT, SG_INFO, "Re-Initializing HID input bindings");
    FGHIDEventInput::shutdown();
    FGHIDEventInput::init();
    FGHIDEventInput::postinit();
}

void FGHIDEventInput::postinit()
{
    SG_LOG(SG_INPUT, SG_INFO, "HID event input starting up");

    hid_init();

    hid_device_info* devices = hid_enumerate(0 /* vendor ID */, 0 /* product ID */);

    for (hid_device_info* curDev = devices; curDev != nullptr; curDev = curDev->next) {
        d->evaluateDevice(curDev);
    }

    hid_free_enumeration(devices);
}

void FGHIDEventInput::shutdown()
{
    SG_LOG(SG_INPUT, SG_INFO, "HID event input shutting down");
    FGEventInput::shutdown();

    hid_exit();
}

//
// read all elements in each input device
//
void FGHIDEventInput::update(double dt)
{
    FGEventInput::update(dt);
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGHIDEventInput> registrantFGHIDEventInput;

///////////////////////////////////////////////////////////////////////////////////////////////

void FGHIDEventInput::FGHIDEventInputPrivate::evaluateDevice(hid_device_info* deviceInfo)
{
    // allocate an input device, and add to the base class to see if we have
    // a config
    p->AddDevice(new FGHIDDevice(deviceInfo, p));
}

///////////////////////////////////////////////////////////////////////////////////////////////
