project "GearHead-Engine"

    kind "SharedLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "ghpch.hpp"
    pchsource "src/ghpch.cpp"

    files{
        "src/**.hpp",
        "src/**.cpp",
        "Include/glm/**.hpp",
        "Include/glm/**.inl"

    }

    defines{
		"_CRT_SECURE_NO_WARNINGS",
        "GEARHEAD_BUILD_DLL"
	}

    includedirs{
        "src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.VulkanSDK}"
    }

    links{
        "%{Library.GLFW}",
        "%{Library.Vulkan}"
    }

    filter "system:windows"
		systemversion "latest"

		defines{
            "GEARHEAD_PLATFORM_WINDOWS"
		}

		
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Game")
		}

    filter "configurations:Debug"
		defines "GEARHEAD_DEBUG"
		runtime "Debug"
		symbols "on"

		defines {"GEARHEAD_ENABLE_ASSERTS"}
		links
		{
			"%{Library.spdlog_debug}",
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}"
		}
    
    filter "configurations:Release"
		defines "GEARHEAD_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{

			"%{Library.spdlog_release}",
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}

	filter "configurations:Dist"
		defines "GEARHEAD_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.spdlog_release}",
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}
