project "Game"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")



	files
	{
		"src/**.hpp",
		"src/**.cpp"
	}

	includedirs
	{
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.VulkanSDK}",
		"%{wks.location}/GearHead-Engine/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}"
	}

	links
	{
		"GearHead-Engine"
	}
    filter "system:windows"
		systemversion "latest"
		defines{
			"GEARHEAD_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "GEARHEAD_DEBUG"
		runtime "Debug"
		symbols "on"

		links{ "%{Library.spdlog_debug}"}
		
		filter "configurations:Release"
		defines "GEARHEAD_RELEASE"
		runtime "Release"
		optimize "on"
		links{ "%{Library.spdlog_release}"}
		
		filter "configurations:Dist"
		defines "GEARHEAD_DIST"
		runtime "Release"
		optimize "on"
		links{ "%{IncludeDir.spdlog_release}"}