!include "MUI.nsh"

!system 'osgversion --so-number > %TEMP%\osg-so-number.txt'
!system 'osgversion --version-number > %TEMP%\osg-version.txt'

!define /file OSGSoNumber $%TEMP%\osg-so-number.txt
!define /file OSGVersion $%TEMP%\osg-version.txt

!echo "osg-so is ${OSGSoNumber}"

Name "FlightGear Nightly"
OutFile fgfs_win32_nightly.exe

InstallDir $PROGRAMFILES\FlightGear-nightly

; Request admin privileges for Windows Vista
RequestExecutionLevel highest

; don't hang around
AutoCloseWindow true
		
!define UninstallKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\FlightGear-nightly"
!define FGBinDir "flightgear\projects\VC90\Win32\Release"
!define FGRunBinDir "fgrun\msvc\9.0\Win32\Release"
!define OSGInstallDir "install\msvc90\OpenSceneGraph"
!define OSGPluginsDir "${OSGInstallDir}\bin\osgPlugins-${OSGVersion}"

!define ThirdPartyBinDir "3rdParty\bin"

!define MUI_ICON "flightgear\projects\VC90\flightgear.ico"
!define MUI_UNICON "flightgear\projects\VC90\flightgear.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "flightgear\package\Win-NSIS\fg-install-header.bmp" ; optional



;!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"
;!define MUI_UNWELCOMEFINISHPAGE_BITMAP "welcome.bmp"

!insertmacro MUI_PAGE_WELCOME
; include GPL license page
!insertmacro MUI_PAGE_LICENSE "flightgear\Copying"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN $INSTDIR\fgrun.exe
!define MUI_FINISHPAGE_RUN_TEXT "Run FlightGear now"
!insertmacro MUI_PAGE_FINISH


!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; The stuff to install
Section "" ;No components page, name is not important  
        
  SetShellVarContext all
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  File ${FGBinDir}\fgfs.exe
  File ${FGBinDir}\fgjs.exe
  File ${FGBinDir}\terrasync.exe
  File ${FGRunBinDir}\fgrun.exe
  
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osg.dll
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osgDB.dll
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osgGA.dll
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osgParticle.dll
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osgText.dll
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osgUtil.dll
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osgViewer.dll
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osgSim.dll
  File ${OSGInstallDir}\bin\osg${OSGSoNumber}-osgFX.dll
  
  File ${OSGInstallDir}\bin\ot12-OpenThreads.dll
  
  File ${ThirdPartyBinDir}\*.dll
  
  ; VC runtime redistributables
  File "$%VCINSTALLDIR%\redist\x86\Microsoft.VC90.CRT\*.dll"
  
  File /r ${FGRunBinDir}\locale
  
  SetOutPath $INSTDIR\osgPlugins-${OSGVersion}
  File ${OSGPluginsDir}\osgdb_ac.dll
  File ${OSGPluginsDir}\osgdb_jpeg.dll
  File ${OSGPluginsDir}\osgdb_rgb.dll  
  File ${OSGPluginsDir}\osgdb_png.dll
  File ${OSGPluginsDir}\osgdb_txf.dll
  
  
  Exec '"$INSTDIR\fgrun.exe"  --silent --fg-exe="$INSTDIR\fgfs.exe" --ts-exe="$INSTDIR\terrasync.exe" '
  
  CreateShortCut "$SMPROGRAMS\FlightGear-nightly.lnk" "$INSTDIR\fgrun.exe" 
  WriteUninstaller "$INSTDIR\FlightGear_Uninstall.exe"
  
  WriteRegStr HKLM ${UninstallKey} "DisplayName" "FlightGear Nightly"
  WriteRegStr HKLM ${UninstallKey} "DisplayVersion" "2.1"
  WriteRegStr HKLM ${UninstallKey} "UninstallString" "$INSTDIR\FlightGear_Uninstall.exe"
  WriteRegStr HKLM ${UninstallKey} "UninstallPath" "$INSTDIR\FlightGear_Uninstall.exe"
  WriteRegDWORD HKLM ${UninstallKey} "NoModify" 1
  WriteRegDWORD HKLM ${UninstallKey} "NoRepair" 1
  WriteRegStr HKLM ${UninstallKey} "URLInfoAbout" "http://www.flightgear.org/"
 
SectionEnd



Section "Uninstall"
   
  SetShellVarContext all
  
  
  Delete "$SMPROGRAMS\FlightGear-nightly.lnk"
  
  RMDir /r "$INSTDIR"
  
  DeleteRegKey HKLM ${UninstallKey}

SectionEnd
 