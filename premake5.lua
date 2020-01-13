workspace "OpenGL-Sandbox"
	architecture "x64"
	startproject "OpenGL-Sandbox"

	configurations
	{
		"Debug",
		"Release"
	}
	
	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "OpenGL-Core/vendor/glfw/include"
IncludeDir["Glad"] = "OpenGL-Core/vendor/Glad/include"
IncludeDir["ImGui"] = "OpenGL-Core/vendor/imgui"
IncludeDir["glm"] = "OpenGL-Core/vendor/glm"
IncludeDir["stb_image"] = "OpenGL-Core/vendor/stb_image"

group "Dependencies"
	include "OpenGL-Core/vendor/glfw"
	include "OpenGL-Core/vendor/Glad"
	include "OpenGL-Core/vendor/imgui"

group ""

project "OpenGL-Core"
	location "OpenGL-Core"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "glpch.h"
	pchsource "OpenGL-Core/src/glpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
	}

	-- Exclude all folders in Platform, since we'll add them per platform
	removefiles { "%{prj.name}/src/Platform/**" }

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}"
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"ImGui"
	}

	filter "system:linux"
		pic "On"
		systemversion "latest"

		-- Add Linux-specific files
		files
		{
			"%{prj.name}/src/Platform/Linux/**.h",
			"%{prj.name}/src/Platform/Linux/**.cpp"
		}

		links
		{
			"Xrandr",
			"Xi",
			"GLEW",
			"GLU",
			"GL",
			"X11"
		}

 		defines
		{
			"GLCORE_PLATFORM_LINUX"
		}

	filter "system:windows"
		systemversion "latest"

		-- Add Windows-specific files
		files
		{
			"%{prj.name}/src/Platform/Windows/**.h",
			"%{prj.name}/src/Platform/Windows/**.cpp"
		}

		links
		{
			"opengl32.lib"
		}

		defines
		{
			"GLCORE_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "GLCORE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "GLCORE_RELEASE"
		runtime "Release"
		optimize "on"

project "OpenGL-Sandbox"
	location "OpenGL-Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"OpenGL-Core/vendor/spdlog/include",
		"OpenGL-Core/src",
		"OpenGL-Core/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}"
	}

	links
	{
		"OpenGL-Core"
	}

	filter "system:linux"
	systemversion "latest"

		defines
		{
			"GLCORE_PLATFORM_LINUX"
		}

		links
		{
			"GLFW",
			"Glad",
			"ImGui",
			"X11",
			"pthread",
			"dl"
		}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"GLCORE_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "GLCORE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "GLCORE_RELEASE"
		runtime "Release"
		optimize "on"
