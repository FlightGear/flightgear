
include_directories(${PROJECT_SOURCE_DIR})

# note order here affects link order, and hence linking correctness
# on systems with a traditional ld (eg, GNU ld on Linux)
foreach( mylibfolder 
		Airports
		Aircraft
		Radio
		ATC
		ATCDCL
		Autopilot
		Cockpit
		Environment
		GUI
		Input
		Instrumentation
		Model
		MultiPlayer
    AIModel
    Navaids
		Network
		Scenery
		Scripting
		Sound
		Systems
		Time
		Traffic
		FDM
		Main
    )

    add_subdirectory(${mylibfolder})
endforeach( mylibfolder )

