#!/usr/bin/ruby

require 'ERB'

$osgLibs = ['osgFX', 'osgParticle', 'osg', 'osgGA', 'osgText', 'osgUtil', 'osgSim', 'osgViewer', 'osgDB']
$osgPlugins = ['ac', 'osg', 'freetype', 'qt', 'imageio', 'rgb', 'txf']
$osgDylibVersion='63'
$alutSourcePath='/Library/Frameworks/ALUT.framework'

def fix_install_names(object)
  #puts "fixing install names for #{object}"
  
  $osgLibs.each do |l|
    oldName = "lib#{l}.#{$osgDylibVersion}.dylib"
    newName = "@executable_path/../Frameworks/#{oldName}"
    `install_name_tool -change #{oldName} #{newName} #{object}`
  end
  
  oldName = "libOpenThreads.12.dylib"
  newName= "@executable_path/../Frameworks/#{oldName}"
  `install_name_tool -change #{oldName} #{newName} #{object}`
  
  alutBundlePath = "@executable_path/../Frameworks/Alut.framework"
  alutLib = "Versions/A/ALUT"
  `install_name_tool -change #{$alutSourcePath}/#{alutLib} #{alutBundlePath}/#{alutLib} #{object}`
end

prefixDir=Dir.pwd + "/dist"
dmgDir=Dir.pwd + "/image"
srcDir=Dir.pwd + "/flightgear"
dmgPath = Dir.pwd + "/fg_mac_nightly.dmg"

puts "Erasing previous image dir"
`rm -rf #{dmgDir}`

osgVersion = `export PKG_CONFIG_PATH=#{prefixDir}/lib/pkgconfig;  pkg-config --modversion openscenegraph`
osgVersion = osgVersion.chomp # strip trailing newlines
puts "osgVersion='#{osgVersion}'"

bundle=dmgDir + "/FlightGear.app"
contents=bundle + "/Contents"
macosDir=contents + "/MacOS"
frameworksDir=contents +"/Frameworks"
resourcesDir=contents+"/Resources"
osgPluginsDir=contents+"/PlugIns/osgPlugins-#{osgVersion}"
volName="\"FlightGear Nightly Build\""

puts "Creating directory structure"
`mkdir -p #{macosDir}`
`mkdir -p #{frameworksDir}`
`mkdir -p #{resourcesDir}`
`mkdir -p #{osgPluginsDir}`

puts "Copying binaries"
bins = ['fgfs', 'terrasync', 'fgjs']
bins.each do |b|
  `cp #{prefixDir}/bin/#{b} #{macosDir}/#{b}`
  fix_install_names("#{macosDir}/#{b}")
end

puts "copying libraries"
$osgLibs.each do |l|
  libFile = "lib#{l}.#{$osgDylibVersion}.dylib"
  `cp #{prefixDir}/lib/#{libFile} #{frameworksDir}`
  fix_install_names("#{frameworksDir}/#{libFile}")
end

# and not forgetting OpenThreads
libFile = "libOpenThreads.12.dylib"
`cp #{prefixDir}/lib/#{libFile} #{frameworksDir}`

$osgPlugins.each do |p|
  pluginFile = "osgdb_#{p}.so"
  `cp #{prefixDir}/lib/osgPlugins-#{osgVersion}/#{pluginFile} #{osgPluginsDir}`
  fix_install_names("#{osgPluginsDir}/#{pluginFile}")
end

# custom ALUT
# must copy frameworks using ditto
`ditto #{$alutSourcePath} #{frameworksDir}/ALUT.framework`

# Info.plist
template = File.read("#{srcDir}/package/mac/nightly.plist.in")
output = ERB.new(template).result(binding)

File.open("#{contents}/Info.plist", 'w') { |f|
  f.write(output)
}

`cp #{srcDir}/package/mac/nightly-readme.rtf #{dmgDir}/ReadMe.rtf`
`cp #{srcDir}/package/mac/FlightGear.icns #{resourcesDir}/FlightGear.icns`
`cp #{srcDir}/COPYING #{dmgDir}`

puts "Creating DMG"

createArgs = "-format UDBZ -imagekey bzip2-level=9 -quiet -volname #{volName}"

`rm #{dmgPath}`
`hdiutil create -srcfolder #{dmgDir} #{createArgs} #{dmgPath}`
