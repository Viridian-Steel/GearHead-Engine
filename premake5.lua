workspace "GearHead"
    architecture "x64"

    configurations{ "Debug", "Release", "Dist"}


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "GearHead-Engine"
    location "GearHead-Engine"
    kind "SharedLib"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")


    pchheader "ghpch.h"
    pchsource "GearHead-Engine/src/ghpch.cpp"

    files {	"%{prj.name}/src/**.h", "%{prj.name}/src/**.cpp"}

	includedirs { "%{prj.name}/vendor/spdlog/include", "%{prj.name}/src/" }

    filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
    
        defines{ "GEARHEAD_PLATFORM_WINDOWS", "GEARHEAD_BUILD_DLL"}

        postbuildcommands{ ("{COPYDIR} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Game") }
    
    filter "configurations:Debug"
        defines "GEARHEAD_DEBUG"
        symbols "On"
    
    filter "configurations:Release"
        defines "GEARHEAD_RELEASE"
        optimize "On"
    
    filter "configurations:Dist"
        defines "GEARHEAD_DIST"
        optimize "On"

project "Game"
    location "Game"
    kind "ConsoleApp"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
		
	files {"%{prj.name}/src/**.h", "%{prj.name}/src/**.cpp"}

    includedirs {"GearHead-Engine/vendor/spdlog/include", "GearHead-Engine/src"}

	links { "GearHead-Engine" }

    filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
    
        defines{ "GEARHEAD_PLATFORM_WINDOWS" }

    filter "configurations:Debug"
        defines "GEARHEAD_DEBUG"
        symbols "On"
    
    filter "configurations:Release"
        defines "GEARHEAD_RELEASE"
        optimize "On"
    
    filter "configurations:Dist"
        defines "GEARHEAD_DIST"
        optimize "On"