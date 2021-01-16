
Imge is a game engine project inspired by [Our Machinery](https://ourmachinery.com/) blogs. It is a base foundation rather than a game engine right now but more things will come.

I wrote rendering code using shaders from my [previous project](https://github.com/imgeself/PlaygroundEngine) in [System.cpp](https://github.com/imgeself/imge/blob/main/system/src/System.cpp) for demonstration. But I will implement proper scene management, scene rendering, render graphs, etc.
I wrote a [blog post](https://imgeself.github.io/posts/2021-01-16-imge/) about this project and my thoughts on The Machinery. Feel free to check that out. 

It has these features right now:
- DLL hot reloading
- Archetype based ECS implementation
- Custom allocators
- Custom containers
- GLTF scene loader
- Profiler, Logger
- Imgui
- Windows platform layer
- Direct3D11 rendering API

Requirements for building:
- Visual Stduio 2019
- Windows 10 64bit
- [Clang tools for Visual Studio](https://docs.microsoft.com/en-us/cpp/build/clang-support-msbuild)

Projects used in this project:
- [Dear Imgui](https://github.com/ocornut/imgui)
- [stbimage](https://github.com/nothings/stb)
- [cgltf](https://github.com/jkuhlmann/cgltf)
- [premake5](https://github.com/premake/premake-core)