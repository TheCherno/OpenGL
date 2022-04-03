project "OpenGL-Tests"
  kind "ConsoleApp"
  language "C++"
  cppdialect "C++17"
  staticruntime "on"

  targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
  objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

  files
  {
    "src/**.h",
    "src/**.cpp",
    "vendor/googletest/include/gtest/**.h",
    "vendor/googletest/include/gtest/**.hpp",
    "vendor/googletest/src/gtest-all.cc"
  }

  files
  {-- Dependencies
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/contrib/unzip/**',
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/contrib/irrXML/**',
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/contrib/zlib/*',
		-- Common
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/code/Common/**',
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/code/PostProcessing/**',
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/code/Material/**',
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/code/CApi/**',
		-- Importers
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/code/Collada/**',
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/code/Obj/**',
		-- 'assimp/code/Blender/**', 'assimp/contrib/poly2tri/poly2tri/**',
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/code/FBX/**',
		-- 'assimp/code/glTF2/**',
		-- 'assimp/code/glTF/**',
		'../OpenGL-Core/${IncludeDir.assimp}/assimp/code/Assbin/**' 
  }

  includedirs
  {
    "mylib",
    "vendor/googletest/include",
    "vendor/googletest/",
    "../OpenGL-Sandbox/src",
    "../OpenGL-Core/vendor/spdlog/include",
		"../OpenGL-Core/src",
		"../OpenGL-Core/vendor",
    "../OpenGL-Core/%{IncludeDir.GLFW}",
		"../OpenGL-Core/%{IncludeDir.Glad}",
		"../OpenGL-Core/%{IncludeDir.ImGui}",
		"../OpenGL-Core/%{IncludeDir.glm}",
		"../OpenGL-Core/%{IncludeDir.stb_image}",
		"../OpenGL-Core/%{IncludeDir.assimp}/include",
		"../OpenGL-Core/%{IncludeDir.assimp}/contrib/irrXML",
		"../OpenGL-Core/%{IncludeDir.assimp}/contrib/zlib",
		"../OpenGL-Core/%{IncludeDir.assimp}/contrib/rapidjson/include"
  }

  links
  {
    "OpenGL-Sandbox",
    "OpenGL-Core"
  }

  filter "configurations:Debug"
    defines "GLCORE_DEBUG"
    runtime "Debug"
    symbols "on"
