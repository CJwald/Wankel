-- premake5.lua
workspace "Wankel"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Wankel-App"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Wankel"
	include "Wankel/Build-Wankel.lua"
group ""

include "WankelExternal.lua"
include "Wankel-App/Build-Wankel-App.lua"