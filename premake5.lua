if _ACTION:startswith("vs") then
    require("vstudio")

    local function precompiledHeaderOutputFile(prj)
        premake.w('<PrecompiledHeaderOutputFile>$(OutDir)sharedpch.pch</PrecompiledHeaderOutputFile>')
    end

    premake.override(premake.vstudio.vc2010.elements, "clCompile", function(base, prj)
        local calls = base(prj)
        table.insert(calls, precompiledHeaderOutputFile)
        return calls
    end)
end

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

solution "imge"
    architecture "x86_64"
    startproject "platform"
    language "C++"
    cppdialect "C++17"
    characterset "MBCS"
    toolset "msc-ClangCL"
    debugformat "c7"
    symbols "on"
    rtti "off"
    exceptionhandling "off"

    configurations { "Debug", "Release" }
    flags { "MultiProcessorCompile" }

    targetdir ("bin/" .. outputdir)
    objdir ("bin/int/" .. outputdir)

    filter "configurations:Debug"
        runtime "Debug"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter "system:windows"
        defines { "PLATFORM_WINDOWS" }

project "platform"
    kind "ConsoleApp"
    pchheader "pch.h"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs 
    {
        "**"
    }

    links
    {
    }

    dependson { "sharedpch" }

project "rhi"
    kind "SharedLib"
    pchheader "pch.h"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs 
    {
        "**"
    }

    links
    {
        "d3d11",
        "dxguid",
        "d3dcompiler"
    }

    dependson { "sharedpch" }

project "system"
    kind "SharedLib"
    pchheader "pch.h"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs 
    {
        "**"
    }

    links
    {
        "d3dcompiler",
    }

    dependson { "sharedpch" }

project "foundation"
    kind "SharedLib"
    pchheader "pch.h"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/*.cpp"
    }

    includedirs 
    {
        "**"
    }

    links
    {
    }

    dependson { "sharedpch" }

project "imgui"
    kind "SharedLib"
    pchheader "pch.h"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs 
    {
        "**"
    }

    links
    {
        "d3dcompiler"
    }

    dependson { "sharedpch" }

project "ecs"
    kind "SharedLib"
    pchheader "pch.h"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs 
    {
        "**"
    }

    dependson { "sharedpch" }

project "asset_loading"
    kind "SharedLib"
    pchheader "pch.h"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs 
    {
        "**"
    }

    links
    {
    }

    dependson { "sharedpch" }


project "sharedpch"
    kind "SharedLib"
    pchheader "pch.h"
    pchsource "foundation/src/pch/pch.cpp"

    files
    {
        "foundation/src/pch/*.h",
        "foundation/src/pch/pch.cpp"
    }

    includedirs 
    {
        "platform/src",
    }