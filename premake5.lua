include "./Vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"


workspace "GearHead"

    architecture "x86_64"
    configurations{ "Debug", "Release", "Dist"}

    solution_items{ ".editorconfig" }

	flags{ "MultiProcessorCompile" }
    
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


group "Core"
    include "GearHead-Engine"

group ""

group "Misc"
    include "Game"

group ""