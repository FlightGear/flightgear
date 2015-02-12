#!/usr/bin/ruby

require 'rubygems' # needed for Ruby 1.8
require 'plist'

$osgLibs = ['osgFX', 'osgParticle', 'osg', 'osgGA', 'osgText', 'osgUtil', 'osgSim', 'osgViewer', 'osgDB']
$osgPlugins = ['ac', 'osg', 'freetype', 'imageio', 'rgb', 'txf', 'mdl', '3ds', 'dds']

def runOsgVersion(option)
  env = "export DYLD_LIBRARY_PATH=#{Dir.pwd}/dist/lib"
  bin = Dir.pwd + "/dist/bin/osgversion"
  return `#{env}; #{bin} --#{option}`.chomp
end

osgVersion = runOsgVersion('version-number')
$osgSoVersion=runOsgVersion('so-number')
$openThreadsSoVersion=runOsgVersion('openthreads-soversion-number')

$codeSignIdentity = ENV['FG_CODESIGN_IDENTITY']
puts "Code signing identity is #{$codeSignIdentity}"

puts "osgVersion=#{osgVersion}, so-number=#{$osgSoVersion}"

def fix_install_names(object)
  puts "fixing install names for #{object}"

  $osgLibs.each do |l|
    oldName = "lib#{l}.#{$osgSoVersion}.dylib"
    newName = "@executable_path/../Frameworks/#{oldName}"
    `install_name_tool -change #{oldName} #{newName} #{object}`
  end

  oldName = "libOpenThreads.#{$openThreadsSoVersion}.dylib"
  newName= "@executable_path/../Frameworks/#{oldName}"
  `install_name_tool -change #{oldName} #{newName} #{object}`
end

$prefixDir=Dir.pwd + "/dist"
dmgDir=Dir.pwd + "/image"
srcDir=Dir.pwd + "/flightgear"

puts "Erasing previous image dir"
`rm -rf #{dmgDir}`

bundle=dmgDir + "/FlightGear.app"

# run macdeployt before we rename the bundle, otherwise it
# can't find the bundle executable
puts "Running macdeployqt on the bundle to copy Qt libraries"
`macdeployqt #{$prefixDir}/fgfs.app`

puts "Moving & renaming app bundle"
`mkdir -p #{dmgDir}`
`mv #{$prefixDir}/fgfs.app #{bundle}`

contents=bundle + "/Contents"
macosDir=contents + "/MacOS"
$frameworksDir=contents +"/Frameworks"
resourcesDir=contents+"/Resources"

# changed this for customised OSG
osgPluginsDir=contents+"/PlugIns/osgPlugins"
volName="\"FlightGear Nightly Build\""

fgVersion = File.read("#{srcDir}/version").strip

dmgPath = Dir.pwd + "/fg_mac_nightly_#{fgVersion}.dmg"

puts "Creating directory structure"
`mkdir -p #{$frameworksDir}`
`mkdir -p #{resourcesDir}`
`mkdir -p #{osgPluginsDir}`

# fix install names on the primary executable
fix_install_names("#{macosDir}/fgfs")

puts "Copying auxilliary binaries"
bins = ['fgjs', 'fgcom']
bins.each do |b|
  if !File.exist?("#{$prefixDir}/bin/#{b}")
    next
  end

  outPath = "#{macosDir}/#{b}"
  `cp #{$prefixDir}/bin/#{b} #{outPath}`
  fix_install_names(outPath)
end

puts "copying libraries"
$osgLibs.each do |l|
  libFile = "lib#{l}.#{$osgSoVersion}.dylib"
  `cp #{$prefixDir}/lib/#{libFile} #{$frameworksDir}`
  fix_install_names("#{$frameworksDir}/#{libFile}")
end

# and not forgetting OpenThreads
libFile = "libOpenThreads.#{$openThreadsSoVersion}.dylib"
`cp #{$prefixDir}/lib/#{libFile} #{$frameworksDir}`

$osgPlugins.each do |p|
  pluginFile = "osgdb_#{p}.dylib"
  `cp #{$prefixDir}/lib/osgPlugins/#{pluginFile} #{osgPluginsDir}`
  fix_install_names("#{osgPluginsDir}/#{pluginFile}")
end

`cp #{srcDir}/package/mac/nightly-readme.rtf #{dmgDir}/ReadMe.rtf`
`cp #{srcDir}/package/mac/FlightGear.icns #{resourcesDir}/FlightGear.icns`
`cp #{srcDir}/COPYING #{dmgDir}`

if File.exist?("#{$prefixDir}/share/flightgear")
  puts "Copying FGCom data files"
  `ditto #{$prefixDir}/share/flightgear #{resourcesDir}`
end

# code sign the entire bundle once complete - v2 code-signing
puts "Signing #{bundle}"
`codesign --deep -s "#{$codeSignIdentity}" #{bundle}`

puts "Creating DMG"

createArgs = "-format UDBZ -imagekey bzip2-level=9 -quiet -volname #{volName}"

`rm -f #{dmgPath}`
`hdiutil create -srcfolder #{dmgDir} #{createArgs} #{dmgPath}`
