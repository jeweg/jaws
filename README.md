# jaws

My latest personal render engine / Vulkan experiment in development.

Some of my goals here:
* Getting familiar with Vulkan, of course!
* C++17, modern CMake use
* Not necessarily TDD, but have tests
* See how my approach to use **vcpkg**  (see [vcpkg_cmake](https://github.com/jeweg/vcpkg-cmake)) instead of the more common git submodules + `add_subdirectory`holds up in practice (so far: mixed)
* In that vein, use libraries liberally, I'm not trying to go handmade or self-contained here. Those are valid approaches, but here I'd like to get familiar with well-regarded libraries through this as well. Libraries in use at the moment:
	* **shaderc** for *GLSL* compilation
	* **vulkan-memory-allocator** because it's certainly better than what I could do at this point in my Vulkan career
	* **spirv-cross**
    * **volk**
	* **abseil**, mainly for its hash tables
	* **xxhash** for file content hashes (Abseil's hashing for all the rest at the moment)
	* **spdlog**
	* **glm**
	* **fmt**
    * **glfw3**
	* **googletest**

## Status

I'm trying to balance moving forward with building some foundations along the way.
I've climbed the hill of seeing my first Vulkan triangle, but then I've gone back to building some things I feel will be useful.
Those were tangents for sure, but I wasn't trying to get quickly to any particular goal (which would have helped progress, but this isn't the day job).

### What I have so far:
* Vulkan and device initialization
* Queue selection heuristics
* Runtime GLSL shader compilation
* Shader code and includes from filesystem or from memory (abstracted with a virtual filesystem)
* Automatic shader recompilation when any involved file has changed
* Pools for user-managed resources like buffers and images
* LRU caches for other objects like shader modules

### What I want next:
* Headless and compute-only working
* Abstract WSI/swapchain render loop so we we're not limited to rendering into a surface
* Descriptor set layouts generated from the spirv-cross reflection data
* A chunked lifetime concept similar to frame context in [Granite](https://github.com/Themaister/Granite)
* Shader reloading in action
* More object types generated internally on demand and cached (pipelines and friends!)
* Integration of [Dear ImGui](https://github.com/ocornut/imgui), [RenderDoc](https://renderdoc.org/), [Fossilize](https://github.com/ValveSoftware/Fossilize) 

And of course, and hopefully along the way towards the above...
* Actually render something nice.
