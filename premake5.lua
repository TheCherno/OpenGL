-- FractalVisualizer
workspace "FractalVisualizer"
	architecture "x64"
	startproject "FractalVisualizer"

	configurations {
		"Debug",
		"Release"
	}
	
	flags {
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to OpenGL-Core
IncludeDir = {}
IncludeDir["GLFW"] = "vendor/GLFW/include"
IncludeDir["Glad"] = "vendor/Glad/include"
IncludeDir["ImGui"] = "vendor/imgui"
IncludeDir["glm"] = "vendor/glm"
IncludeDir["stb"] = "vendor/stb"

-- Projects
group "Dependencies"
	include "OpenGL-Core/vendor/GLFW"
	include "OpenGL-Core/vendor/Glad"
	include "OpenGL-Core/vendor/imgui"
group ""

include "OpenGL-Core"
include "FractalVisualizer"