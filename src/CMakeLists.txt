
include_directories(${PROJECT_SOURCE_DIR})

foreach( mylibfolder 
		AIModel
		Airports
		Aircraft
		ATC
		Autopilot
		Cockpit
		Environment
		GUI
		Input
		Instrumentation
		Model
		MultiPlayer
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

