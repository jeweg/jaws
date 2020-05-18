# [2019-7-31 Wed]
Cleaned up a bit. The examples and tests now use a convenience wrapper around add_executable now.
This is because settings some compiler options for all executables is useful, but we still would not
want to export such settings publicly with the jaws lib.
Ditched vulkan.hpp and it's dependency on a specific version of the vulkan sdk. Turns out the
vulkan sdk comes with a matching vulkan.hpp anyway, so we use that now. Eventually I'll have to
figure out how to live without the sdk, but for now this nicely does. The sdk also comes with shaderc.

TODO:
** Static lib/context
We still don't really have a vision regarding the static state/context. I would like to support
jaws as a static library as well, but then we can't use global variables like the instance and the
fatal handler. I think I need some more inspiration from other libs.
** Vulkan
Working on the vulkan example should be next on the agenda. Ignore OpenGL for now. I need to see
something nice first.

# [2019-8-01 Thu]
More cleanups. I've had a couple of crashes now with VS 2019, even after disabling all plugins.
That's really silly, and the VS editor feels quite clunky anyway.
Now that CLion has experimental visual C++ debugger support, I'll give that another change.
With CLion I immediately ran into another silliness: it only supports NMake as CMake generator,
the worst generator in the world. Here's the feature request: [[https://youtrack.jetbrains.com/issue/CPP-2659]].
A workaround mentioned there seems to work: use the command line CLion itself uses to configure the build once
with e.g. Ninja, and from then on CLion uses cmake --build in blissful ignorance.
Note that you should call the command from the target build dir.
Of course this has problems later on when clion needs to rerun cmake...
    Cannot generate into D:\__jw__\repos\projects\jaws\cmake-build-debug-visual-studio-2019
    It was created with incompatible generator 'Ninja'
    Please either delete it manually or select another generation directory
    [Failed to reload]
I need to move on for now.. let's use the fricking NMake generator.
At least debugging seems to work.
I've also tried to use the clang-format feature that's built into clion starting with 2019.1.
It worked after a while. In the status bar, there's a clangFormat label that can be used to open the used
clangformat file. when editing it in clion, it actually takes effect right away and is used in formatting
operations (ctrl-alt-L). Otherwise it seems to be cached? Anyway, it seems to be used even for on-the-fly
formatting, which is fine.
I've also cleaned up the main wrapper and gave it an example.

New useful shortcuts:
- double-shift navigates to anything
- ctrl-j inserts snippets. I've added separator snippets.

When working on the vulkan example, I ran into the problem that vulkan.hpp could not be indexed
by clion's code insight features due to its file size ("the file size exceeds configured limit").
I fixed it by choosing "Help->edit custom properties" and pasting
    idea.max.intellisense.filesize=999999
    idea.max.content.load.filesize=600000
there (yes, not clion-specific). Probably only the 1st one was necessary for this, though.

TODO:
** Compile-time logging
I haven't gotten this to work yet. Might wanna analyze the sdplog code. See example-logging.

# [2019-8-03 Sat]

 When I started the vulkan example, I set about imitating a glfw3 vulkan sample, but today it became
 clear that things were wrong with it, e.g. the spir-v shader code was somehow not working. I decided to take a step back and follow these samples:
 [[https://github.com/KhronosGroup/Vulkan-Hpp/tree/master/samples]]

# [2019-8-05 Mon]

shaderc from the vulkan sdk seems unusable (no debug libs, and hence iterator debug level errors).
ive tried the shaderc from vcpkg again (which was recently updated), and it seems all it's missing
is the package config file. I've copied the one I made in my vcpkg fork, and that seems to work.
I should work on adding that to the official vcpkg repo.
Now only one link error left, see:
https://stackoverflow.com/questions/37900051/vkcreatedebugreportcallback-ext-not-linking-but-every-other-functions-in-vulkan:

# [2019-8-09 Fri]

I'm getting crashes that seem related to the synchronisation we do.
Let's do some checking...
- we get 3 swap chain images, as expected for mailbox mode IIRC
- for each swap chain image, we create a swap chain image view.
  2d, 1 level, 1 layer, hopefully the same image format.
- for each swap chain image view, we create a framebuffer with it as the only attachment.

- TODO
The render pass is not clear to me yet...
yes, the render pass holds all used attachments and the subpasses reference a subset of those, but where is the actual attachment bound? We only seem to describe it indirectly all the time.

- for each swap chain framebuffer, we create a command buffer with
-- render pass begin, binding the particular framebuffer, the one render pass, and sets a clear color.
-- bind the one pipeline we use in the graphics slot.
-- draw 3 vertices
-- render pass end, command buffer end.

In the mean time, adding more logging and a cmd-buffer-specific clear color showed me that we in fact get frames
rendered, and each command buffer works exactly once without the error msg. after having used each of the 3
command buffers once, we get the error each frame:
    [2019-08-10 22:18:09.008] [General] [info] Got 3 swap chain images
    [2019-08-10 22:18:10.490] [General] [info] Successfully compiled shader (0 errors, 0 warnings)
    [2019-08-10 22:18:10.519] [General] [info] Successfully compiled shader (0 errors, 0 warnings)
    [2019-08-10 22:18:10.525] [General] [info] curr_swapchain_image_index = 0
    [2019-08-10 22:18:10.528] [General] [info] curr_swapchain_image_index = 1
    [2019-08-10 22:18:10.529] [General] [info] curr_swapchain_image_index = 2
    [2019-08-10 22:18:10.530] [General] [info] curr_swapchain_image_index = 0
    VUID-vkQueueSubmit-pCommandBuffers-00071(ERROR / SPEC): msgNum: 0 - VkCommandBuffer 0x1dbc29f32d0[] is already in use and is not marked for simultaneous use. The Vulkan spec states: If any element of the pCommandBuffers member of any element of pSubmits was not recorded with the VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, it must not be in the pending state. (https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-vkQueueSubmit-pCommandBuffers-00071)
I don't know what "in use" means here and what it means for it to be in "pending state".
Regarding pending state, we find:
[[https://www.khronos.org/registry/vulkan/specs/1.1/html/vkspec.html#commandbuffers-lifecycle][A command buffer's lifecycle]]]
This really just says to me that we try to submit a cmd buffer again before its execution has finished. I don't know yet how that could happen.
The wait/signal semaphores are just used to synchronize graphics queue submit and presentation, and that works, at least for the first three frames.

Emptying the cmd buffers to just begin/end gets me a couple more errors regarding image layout transitions, but the one about the simultaneous use does not go away.

I tried forcing the present mode to FIFO, and I got the exact same problematic behavior, indicating that I'm missing something fundamental here.

# [2019-8-12 Mon]

So the state is that we get the error. The plan for today:

** Find if we can suppress it by marking our command buffers for simultaneous use. This will have to be solved later, but I want to look at a different problem first:
=> Yes, the errors goes away. We of course still have the underlying problem, and these messages at the end are probably related:
    VUID-vkDestroySemaphore-semaphore-01137(ERROR / SPEC): msgNum: 0 - Cannot call vkDestroySemaphore on VkSemaphore 0x1c[]
    that is currently in use by a command buffer. The Vulkan spec states: All submitted batches that refer to semaphore must
     have completed execution (https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-vkDestroyS
    emaphore-semaphore-01137)
        Objects: 1
            [0] 0x1c, type: 5, name: NULL
We choose to ignore that for now. Let's look at the missing tringle first.

** Figure out why we see the clear color change, but never see the triangle. Maybe this will lead us to a problem that once solved solves other things with it.
Let's see. We see the surface getting cleared, so it looks like we properly set up the physical and logical device, the surface, the swap chain.

Do we get an error if we miss a vertex and/or fragment shader?
=> The fragment shader is actually optional (only depth output is not undefined then)
=> Omitting the vertex shader gives the expected error
Let's keep both shaders, they look okay.

Comparing pipeline setup steps to [[https://github.com/Overv/VulkanTutorial/blob/master/code/15_hello_triangle.cpp]]:
=> Found something: I've been setting the color write mask to empty.
=> I see a triangle!

Now let's drop the simultaneous use bit again.
    [2019-08-12 11:14:45.349] [General] [info] frame 0: curr_swapchain_image_index = 0
    [2019-08-12 11:14:45.351] [General] [info] frame 1: curr_swapchain_image_index = 1
    [2019-08-12 11:14:45.351] [General] [info] frame 2: curr_swapchain_image_index = 2
    [2019-08-12 11:14:45.352] [General] [info] frame 3: curr_swapchain_image_index = 0
    VUID-vkQueueSubmit-pCommandBuffers-00071(ERROR / SPEC): msgNum: 0 - VkCommandBuffer 0x16b70bf9060[] is already in use an
    d is not marked for simultaneous use. The Vulkan spec states: If any element of the pCommandBuffers member of any elemen
    t of pSubmits was not recorded with the VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, it must not be in the pending stat
    e. (https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-vkQueueSubmit-pCommandBuffers-000
    71)
=> Turns out I needed more semaphores.
=> And fences to throttle the CPU and avoid CPU using GPU resources that are still in flight.

Now the triangle works!
Next up: [[https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation][swap chain recreation]]
Last recommendation:
    reinterpret_cast<uint64_t &>(dispatchableHandle)
    (uint64_t)(nondispatchableHandle)
...Alright?
So most handle types are non-dispatchable, with the exceptions of
VkInstance, VkPhysicalDevice, VkDevice, VkQueue, VkCommandBuffer.

# [2019-8-14 Wed]

Made our custom debug callback work. Turns out we were just missing the msg type flags. Now we see all the errors regarding query retrieval through the logger.
Can we figure this out?
Recording timestamps:
- query pool must be reset before writing
- at vkCmdWriteTimestamp time, the used queries must be unavailable.
The error we are getting:
"vkGetQueryPoolResults() on VkQueryPool 0x1b[] and query ...: waiting on a query that has been reset and not issued yet"
The specs say:
"If VK_QUERY_RESULT_WAIT_BIT is set, Vulkan will wait for each query to be in the available state before retrieving the numerical
results for that query. In this case, vkGetQueryPoolResults is guaranteed to succeed and return VK_SUCCESS if the queries become
available in a finite time (i.e. if they have been issued and not reset).
If queries will never finish (e.g. due to being reset but not issued), then vkGetQueryPoolResults may not return in finite time."
I still don't get what they mean with "issued" here -- the timestamp writes are in the command buffers.
We could just ignore this and aggresively multi-buffer the queries so they are read with a couple of frames delay. That should make us safe.. enough, I guess.
I'd still like to understand the issue.
=> Ah, stupid me. I queried the whole pool of 100 results, but only wrote two. The error was completely right:
queries 2 through 99 were reset, but never written to ("issued").

Now I'd also like to give objects names for better diagnostics.
We ran into the problem that the api wants a uint64_t for the object handle: https://github.com/KhronosGroup/Vulkan-Docs/issues/368

Results today:
- I didn't see an object name in neither nsight nor renderdoc. Not sure if I'm doing something wrong.
- The timing now works, but it's way too slow. We might need to implement the same in OpenGL as a baseline,
  I can't afford doing something fundamentally broken(slow) this early.

Also, I should work more on the main loop. The frames in flight can be decoupled better from the swap chain.
See [[https://gamedev.stackexchange.com/a/145091][here]].

Some more puzzle pieces:
- having a command buffer for each image in a swap chain (or frame in flight) is common. After all, we need that if we want to have multiple frames in flight in some form.
- to actually render changing things, we must rebuild the command buffers OR put the changes in secondary command buffers and call those from static primary command buffers.

# [2019-08-15 Thu]

I found that our code does not build in Release mode. The problem is that CMake links the debug versions of libraries even in release build types.
I've expedrimented with the global property DEBUG_CONFIGURATIONS and the CMAKE_MAP_IMPORTED_CONFIG_<config> variables to no avail so far.
It doesn't seem to be a problem of any particular lib itself -- it started to break with abseil-strings, but that seems sound and it also happens and happened with other libs,
but those didn't break because they didn't trigger iterator debug level comparisons etc.
Is this caused by vcpkg somehow?


jaws> run-cmake.bat
jaws> cmake --build build-windows --verbose --clean-first --config Release -j 8

This works fine. And if we take a look at the linker invocation:
    C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.22.27905\bin\HostX64\x64\link.exe [...] "D:\__jw__\vcpkg_private\installed\x64-windows\lib\absl_strings.lib" [...]
That's the correct release lib (debug versions are in x64-windows/debug/lib).
So what's going wrong from clion?

Is it the nmake generator? (the above uses msbuild)
Switched the above lines to nmake makefiles generator. =>
Breaks in the same way. cmake version 3.15.2. The NMake generator seems to be broken...?
Using jom on the generated make files does not help since the wrong libraries are already set there.
This is a problem.

I need to get this fixed, but for that I need a minimal reproduction.
=> I can reproduce it there. we don't actually need a real library to define an imported lib, we can just use dummy files. We don't need
the linking to succeed, we just want to see which one it tries to link.
Anyway, in that case I can fix it with the line
    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO "RELEASE")
However, when I try that in jaws with the nmake generator, I run into the problem that the generator does not properly fall back to
the IMPORTED_LOCATION property for Vulkan::Vulkan. Didn't I fix that a couple of years ago in CMake? I seem to remember exactly that.
=> Yes, here: https://gitlab.kitware.com/cmake/cmake/merge_requests/186
Oh, and I actually left the solution there:
    set(CMAKE_MAP_IMPORTED_CONFIG_DEVELOPER Developer RelWithDebInfo Release "")
The empty string causes fallback to the configuration-less properties.
Also, I've found I need to use uppercase? Not sure.
But it does work now!

Back to the vulkan stuff:
- [[https://www.reddit.com/r/vulkan/comments/8umnz1/what_is_the_proper_way_to_query_datatimestamps/]]
- [[https://www.reddit.com/r/vulkan/comments/b6m6wx/how_to_use_timestamps/]]

I see now that I mixed up the units and the 1-triangle pass actually takes 3-4 microseconds. That might be okay.

Drawing 3 mio triangles with a discard-only fragment shader takes 46 milliseconcs, though, that's worrying.
- 540 us if we enable the rasterizer discard ("primitives are discarded immediately before the rasterization stage").
- 5800 us with a 20x20 window (was 200x200)
- with a 1920x1080 window about 2.2135e+06us (2.2 seconds!) and the GPU seemed to freeze.. arg, I must still be doing something wrong.
The 1-tri version works alright at 1920x1080: 12 us for the render pass.

=> Okay, I need a fresh start.. Maybe go with the vulkan tutorial or sascha willems hello-vulkan and do those experiments there as well.


# [2019-08-16 Fri]

No fresh start today. Worked on the shader compiler and started a file observer. I want to get the shader hot reload working soon,
having that is just so much fun (and fun is motivation to proceed).

The goal now is to get that working in a rough form. Not sure yet how to link those file changes to shader recompilation and pipeline rebuilds (we need to do that, yes?),
but just having the file observer would motivate me enough for now, I guess.

Next I'd like to improve the swap chain like in the Vulkan Cookbook pg. 498.

After that I'd like to actually render something that requires at least more than one pipeline.

By using the std C++ filesystem API we've switched to C++-17.

# [2019-08-17 Sat]

CMake cleanups -- the experiments and examples now use a common utility function to create the executable target and its common
properties.

# [2019-08-18 Sun]

Made an abseil hashing experiment after some problems. Turns out it needs std::hash for std::filesystem::path due to some
shortcomings of MSVC, and there is none. It should work now, we're officially using flat_hash_map from abseil.
The file observer (simple implementation, polling, just mod timestamps) works now.
We could go back to actually recreating parts of the vulkan data structures.
I also found that the vulkan example crashes when the window is minimized, not only when it is resized. Handling this should be one of the next points on the agenda.
I'm also thinking about dropping vulkan.hpp after seeing a vulkan panel discussion; it's good and a great example on how one should make a C-library wrapper for C++,
but it has its problems -- first and foremost, noone uses it and the spec, tutorial code, samples, validation layers all use the
C names. Maybe we should just change back to those. Not critical, though.
I chose to start the vulkan2 example. Here we want to try the cleaned up frameResources from the vulkan cookbook and maybe use the GPUOpen vulkan memory allocator.

You know what? For vulkan2, let's go back to raw vulkan and a custom loader:
 [[https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1]]

# [2019-08-19 Mon]

Started progress with the loader. Getting one thing out of the way: I don't like the aggregate initialization syntax. It leads to a guessing game of
which argument belongs to which parameter. The vulkan.hpp named parameter idiom was nice, and the next best thing without it is assigning each
parameter separately. Will that be somehow less efficient?
Well, for the following code compiler explorer creates *exactly* the same code for foo1 and foo2 on the compilers I've tested (gcc 5.5, gcc 9.2, msvc 19.15, clang 8.0.0).

    struct Foo {
        int a;
        const char* b;
        int c;
        float d;
    };
    extern void ext(const Foo&);
    void foo1() {
        ext(Foo {
            3, "xyz", 4, 7.3f
        });
    }
    void foo2() {
        Foo f;
        f.a = 3;
        f.b = "xyz";
        f.c = 4;
        f.d = 7.3f;
        ext(f);
    }

That should end that potential discussion. But we careful with the initialization:

void foo3() {
    Foo f;
    f.a = 3;
    f.d = 7.3f;
    ext(f);
}
void foo4() {
    Foo f = {};
    f.a = 3;
    f.d = 7.3f;
    ext(f);
}

foo3 does not zero-initilaize the (other) data members, foo4 does. gcc 9.2:
    foo3():
        sub     rsp, 40
        mov     rdi, rsp
        mov     DWORD PTR [rsp], 3
        mov     DWORD PTR [rsp+20], 0x40e9999a
        call    ext(Foo const&)
        add     rsp, 40
        ret

Let's take a closer look at this.
gcc 9.2 and msvc 19.15 agree on the offsets and size here:
    static_assert(offsetof(Foo, a) == 0);
    static_assert(offsetof(Foo, b) == 8);
    static_assert(offsetof(Foo, c) == 16);
    static_assert(offsetof(Foo, d) == 20);
    static_assert(sizeof(Foo) == 24);
Apparently it wanted to align the pointer on a multiple of 8 (natural alignment).
So the above assembly code writes 3 to f.a and the float representation of (I assume/hope) 7.3 to f.d
and that's exactly it.

    foo4():
        sub     rsp, 40
        pxor    xmm0, xmm0
        mov     rdi, rsp
        movaps  XMMWORD PTR [rsp], xmm0
        mov     DWORD PTR [rsp+16], 0
        mov     DWORD PTR [rsp], 3
        mov     DWORD PTR [rsp+20], 0x40e9999a
        call    ext(Foo const&)
        add     rsp, 40
        ret

First it clears the xmm0 register to zero (that's 128bit/16 bytes).
Then it moves those 16 zero bytes via movaps ("Move Aligned Packed Single-Precision Floating-Point Values")
at the start of the struct. Since that only clears the first 16 bytes, the remaining 4 bytes (dword) is then
manually (mov) to 0. We then write the 3 to f.a, and the float to f.d.
clang 8 does the same thing.
msvc 19.15 does it a slightly differently:

        sub     rsp, 72                             ; 00000048H
        xorps   xmm0, xmm0
        mov     DWORD PTR f$[rsp], 3
        movdqu  XMMWORD PTR f$[rsp+4], xmm0
        lea     rcx, QWORD PTR f$[rsp]
        movss   xmm0, DWORD PTR __real@40e9999a
        movss   DWORD PTR f$[rsp+20], xmm0
        call    void ext(Foo const &)                    ; ext
        add     rsp, 72                             ; 00000048H
        ret     0

It writes the 3 to f.a and then moved the xmm0 zeros via movdqu ("Move Unaligned Double Quadword")
with offset of 4 bytes. This fills bytes [4, ..., 19]  with zero. It later writes the float to f.d (rsp+20),
using the xmm0 register instead of mov (movss does single-precision (4 bytes) and for xmm registers leaves
the other 3 dwords unchanged). I don't know what the lea instruction is supposed to help with.
This all seems like a lot of work for setting 24 bytes with values known at compile-time, but what do I know.

As for the vulkan loading: I've decided to use github.com/zeux/volk. It's just a waste of time to do it myself and I eventually want a generated
method anyway.

Also for the usual vulkanstuff-to-string (VkResult etc) let's not do that by hand..
let's generate the code ourselves.
Example: https://github.com/zeux/volk/blob/master/generate.py
spec: https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/master/xml/vk.xml

Also: add a mode where we try to force a present queue different form the graphics queue. this needs to be tested.
https://stackoverflow.com/questions/51149001/can-graphics-and-present-queues-really-be-different

Damn, I totally missed the need to set a define to get the full set of platform-specifics...
https://community.khronos.org/t/help-needed-with-khr-extensions/7231/2
That's a problem because volk queries the defines (in my case VK_USE_PLATFORM_WIN32_KHR) in volk.c and I don't
control its build process at all... under these circumstances I don't see how it can be built by its own CMakeLists at all,
unless it does conditional defines, which of course it does not. The author wasn't kidding when he wrote that volk.c
should be added to our own sources. Why does it even has a CMakeLists then?
See this issue: https://github.com/zeux/volk/issues/9

Status: global_context pretty much holds what I wanted it to do.
all window/surface-specific stuff goes into window_context (maybe rename to surface context):
the surface format selection logic, swap chain, swap chain images, image views, a step of the
render loop (the looping itself should be controlled externally so we can have multiple windows).
things like the Handle function can move into more shared code and we might want to define
an exception so we can report errors from ctor/dtor. Not sure about that yet.

# [2019-08-22 Thu]

Today I took a little survey of SIMD wrapper libraries. Candidates on github:

vcdevel/vc
    SIMD Vector Classes for C++

aff3ct/MIPP
    MIPP is a portable wrapper for SIMD instructions written in C++11. It supports NEON, SSE, AVX and AVX-512.

ospray/tsimd
    Fundamental C++ SIMD types for Intel CPUs (sse, avx, avx2, avx512)

p12tic/libsimdpp
    Portable header-only zero-overhead C++ low level SIMD library

QuantStack/xsimd
    Modern, portable C++ wrappers for SIMD intrinsics and parallelized, optimized math implementations (SSE, AVX, NEON, AVX512)

-> vcpkg: only xsimd available (about 3 months old)
-> maintained? tsimd is oldest (last commit oct 2 2018), the others are quite current

this probably excludes tsimd.
How to decide between the other, I have no clue.

I've found an interesting discussion at https://stackoverflow.com/a/36852518.
Let's search for "vectorcall" in the code of the libraries (for lack of a better metric, I'm using this as an indicator for the
maturity of the windows support).
=> no special handling in xsimd, MIPP
Vc looks particularly good here, libsimdpp does it via a compiler flag in cmake.

Also, I've tried to use volk in a better way. First I was going to add it to vcpkg, but I quickly realized it has no prerequisites for
importing the installed libs in any form and I don't want the vcpkg to add the required configs machinery. Really volk should do this.
I'm trying something else: I'm going to extend the volk cmakelists and try to submit it as push request.

# [2019-09-03 Tue]

Made volk's cmake well-behaved and got the pull request accepted: [[https://github.com/zeux/volk/pull/28]]
This is good news, as now we can make a vcpkg portfile for it: [[https://github.com/microsoft/vcpkg/pull/8035]]

As for the SIMD wrapper libraries survey the other day: there's another interesting new project at [[https://github.com/mitsuba-renderer/enoki]]

For vulkan, there are a couple of interesting routes.
- the maister's blog (TODO: find the url again!) has a lot of good info.
- bgfx seems to have everything figured out.
  What I want from this is a feeling for a good API, particularly for things like how data updates are exposed.
I've started documents in the knowledge repo for these.

Let's start once again with something like the granite init model. I need to properly integrate volk anyway.

# [2019-09-05 Thu]

Well, the whole volk thing got messy. I found that volk doesn't really lend itself to being built outside of one's project, so the whole
installation and vcpkg thing makes no sense.
However, I'm not sure it needs to be that way. The root of the problem is that volk.c has compile-time conditions using the various platform defines.
I will need to dig a bit deeper into those and maybe find another way to query the same information at runtime.
My idea is that using the ifdefs in volk.h is fine -- they can be expected to be properly set when the user includes the header. So making the existence of
the function pointers depend on the defines is okay. The problem is also using the defines in volk.c when loading the function pointers.
Let's take this opportunity to get a bit deeper into this.
We use a couple of function pointers and trace their way through the files.
Let's start with a simple non-extension call, let's say vkCreateInstance.

** vkCreateInstance

vulkan.h includes vulkan_core.h and that defines:

The function pointer type definition PFN_vkCreateInstance:
    typedef VkResult (VKAPI_PTR *PFN_vkCreateInstance)(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);

Plus the actual function declaration for static linking:
    #ifndef VK_NO_PROTOTYPES
    ...
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance( const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);

volk comes in, sets VK_NO_PROTOTYPES (so no function declarations), and...
in volk.h:
    extern PFN_vkCreateInstance vkCreateInstance;

in volk.c:
    static void volkGenLoadLoader(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
    {
        vkCreateInstance = (PFN_vkCreateInstance)load(context, "vkCreateInstance");
        ...

    volkGenLoadLoader is called from either volkInitialize or volkInitializeCustom, depending on whether the user already has the
    vulkan dynamic lib loaded and has a bootstrapping vkGetInstanceProcAddr in hand.

** vkDeviceWaitIdle

This is a device-level function, let's see that volk does here.
vulkan_core. h defines
    typedef VkResult (VKAPI_PTR *PFN_vkDeviceWaitIdle)(VkDevice device);
and
    #ifndef VK_NO_PROTOTYPES
    ...
    VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device);
So nothing different there.
in volk.h:
    struct VolkDeviceTable
    {
        ...
        PFN_vkDeviceWaitIdle vkDeviceWaitIdle;

in volk.c:
    static void volkGenLoadDevice(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
    {
        ...
        vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)load(context, "vkDeviceWaitIdle");

Now volkGenLoadDevice is also called from volkLoadInstance:
    void volkLoadInstance(VkInstance instance)
    {
        volkGenLoadInstance(instance, vkGetInstanceProcAddrStub);
        volkGenLoadDevice(instance, vkGetInstanceProcAddrStub);
    }

This has got to be the vulkan loader dispatch code stuff for the device-related functions. Passing the instance gets us
functions that internally choose the proper VkDevice (depending on that? I don't know).

    static void volkGenLoadDeviceTable(struct VolkDeviceTable* table, void* context, PFN_vkVoidFunction (*load)(void*, const char*))
    {
        ...
        table->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)load(context, "vkDeviceWaitIdle");

So we have one version that sets the global function pointer (only using one device in an app), and one that puts the function pointers
into a table so we can have mulitple devices active without the dynamic dispatch stuff.

** Non-platform-specific extension: VK_KHR_external_fence

in vulkan_core.h:
    #define VK_KHR_external_fence 1
    #define VK_KHR_EXTERNAL_FENCE_SPEC_VERSION 1
    #define VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME "VK_KHR_external_fence"
    typedef VkFenceImportFlags VkFenceImportFlagsKHR;
    typedef VkFenceImportFlagBits VkFenceImportFlagBitsKHR;
    typedef VkExportFenceCreateInfo VkExportFenceCreateInfoKHR;
volk doesn't do anything depnding on VK_KHR_external_fence, probably due to the fact that this extension doesn't define functions.

** Platform-specific extension: VK_KHR_external_fence_win32

in vulkan_win32.h:

    #define VK_KHR_external_fence_win32 1
    #define VK_KHR_EXTERNAL_FENCE_WIN32_SPEC_VERSION 1
    #define VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME "VK_KHR_external_fence_win32"
    typedef struct VkImportFenceWin32HandleInfoKHR {
        VkStructureType                      sType;
        const void*                          pNext;
        VkFence                              fence;
        VkFenceImportFlags                   flags;
        VkExternalFenceHandleTypeFlagBits    handleType;
        HANDLE                               handle;
        LPCWSTR                              name;
    } VkImportFenceWin32HandleInfoKHR;

    typedef struct VkExportFenceWin32HandleInfoKHR {
        VkStructureType               sType;
        const void*                   pNext;
        const SECURITY_ATTRIBUTES*    pAttributes;
        DWORD                         dwAccess;
        LPCWSTR                       name;
    } VkExportFenceWin32HandleInfoKHR;

    typedef struct VkFenceGetWin32HandleInfoKHR {
        VkStructureType                      sType;
        const void*                          pNext;
        VkFence                              fence;
        VkExternalFenceHandleTypeFlagBits    handleType;
    } VkFenceGetWin32HandleInfoKHR;

    typedef VkResult (VKAPI_PTR *PFN_vkImportFenceWin32HandleKHR)(VkDevice device, const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo);
    typedef VkResult (VKAPI_PTR *PFN_vkGetFenceWin32HandleKHR)(VkDevice device, const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle);

    #ifndef VK_NO_PROTOTYPES
    VKAPI_ATTR VkResult VKAPI_CALL vkImportFenceWin32HandleKHR(
        VkDevice                                    device,
        const VkImportFenceWin32HandleInfoKHR*      pImportFenceWin32HandleInfo);

    VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceWin32HandleKHR(
        VkDevice                                    device,
        const VkFenceGetWin32HandleInfoKHR*         pGetWin32HandleInfo,
        HANDLE*                                     pHandle);
    #endif

So again we have the function pointer declarations always defined, and the function prototypes in the VK_NO_PROTOTYPES block.
Note that vulkan_win32.h only gets included if *VK_USE_PLATFORM_WIN32_KHR* is defined, and that has to be done by the user.

In volk.h:
    #if defined(VK_KHR_external_fence_win32)
        PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR;
        PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR;
    #endif

volk makes it depend on VK_KHR_external_fence_win32, the official compile-time way to query the extension.
In volk.c:

    static void volkGenLoadDevice(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
    {
        ...
        #if defined(VK_KHR_external_fence_win32)
            vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)load(context, "vkGetFenceWin32HandleKHR");
            vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)load(context, "vkImportFenceWin32HandleKHR");
        #endif
and
    static void volkGenLoadDeviceTable(struct VolkDeviceTable* table, void* context, PFN_vkVoidFunction (*load)(void*, const char*))
    {
        ...
        #if defined(VK_KHR_external_fence_win32)
            table->vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)load(context, "vkGetFenceWin32HandleKHR");
            table->vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)load(context, "vkImportFenceWin32HandleKHR");
        #endif

No surprises at this point.

# [2019-09-06 Fri]

Made jaws::vulkan::handle_enumerate today. Works nicely and I must say I'm a little proud of it.
Currently working on jaws/vulkan/to_string.hpp and .cpp, porting utils.hpp/cpp from jaws/attic.

I decided to make these functions convert to std::string instead of directly logging or printing.
This seems the most flexible to me.

# [2019-09-08 Fri]

Today I'm contemplating going back to using vulkan-hpp. The reason is this:
now that I've made a genric enumerate work like this:

	template <typename ResultElemType, typename ... FirstArgs, typename FuncPtrType>
	std::vector<ResultElemType> enumerated(FuncPtrType func_ptr, FirstArgs ... first_args)
	{
		VkResult result;
		std::vector<ResultElemType> result_list;
		uint32_t count = std::numeric_limits<uint32_t>::max();
		do {
			result = func_ptr(first_args..., &count, nullptr);
			JAWS_VK_HANDLE_RETURN(result, result_list);
			if (result == VK_SUCCESS && count > 0) {
				result_list.resize(count);
				result = func_ptr(first_args..., &count, result_list.data());
				JAWS_VK_HANDLE_RETURN(result, result_list);
			}
		} while (result == VK_INCOMPLETE);
		if (result == VK_SUCCESS) {
			JAWS_ASSUME(count <= result_list.size());
			result_list.resize(count);
		}
		return result_list;
	}

I first needed a version of enumerate-style functions that don't return a result.
That's okay. Then I ran into a problem with vkGetPhysicalDeviceQueueFamilyProperties2:
	VUID-VkQueueFamilyProperties2-sType-sType(ERROR / SPEC): msgNum: 0 - vkGetPhysicalDeviceQueueFamilyProperties2:
	parameter pQueueFamilyProperties[0].sType must be VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 The Vulkan spec states:
	sType must be VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 (https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkQueueFamilyProperties2-sType-sType)
The problem here is that after resizing the vector properly, it expects the sType fields of the objects to be set properly.
This really has only one clear solution: use wrapper objects with constructors that set those fields,
i.e. exactly what vulkan-hpp does.
Otoh, using dynamic dispatch with vulkan-hpp is quite painful...
Well, for now I'm passing a prototype element and clone it when resizing the vector. that works a lot better syntax-wise than a constructor lambda.

Another thing I've tried was have automatic detection of void-returning functions vs VkResult-returning functions with sfinae with this (compiler explorer):

	#include <type_traits>
	#include <iostream>

	void some_function() {}
	int some_function2() { return -1; }

	template <typename funcptr, typename... Args>
	void foo(funcptr func_ptr, Args ...args,
	 std::enable_if_t<std::is_same_v<int, decltype(func_ptr(args...))> >* = 0)
	{
		int status = func_ptr();
		std::cout << "status version\n";
	}

	template <typename funcptr, typename... Args>
	void foo(funcptr func_ptr, Args ...args,
	 std::enable_if_t<std::is_same_v<void, decltype(func_ptr(args...))> >* = 0)
	{
		func_ptr();
		std::cout << "non-status version\n";
	}

	int main()
	{
		foo(some_function2);
		foo(some_function);
	};

That works in clang, but leads to internal compiler errors in gcc x86-64 trunk.
Let's keep it the manual way.

# [2019-09-10 Tue]

We probably want something like this: https://github.com/lethal-guitar/RigelEngine/blob/master/cmake/Modules/FindFilesystem.cmake
Also what the heck, multi-line comments work? Awesome!

# [2019-09-14 Sat]

As a follow-up to the string_view stuff: I started to have weird crashes when using absl::StrCat. I found out that the temporary
AlphaNum argument objects hold a std::string as absl::string_view, and that type seems to decide to just typeef to std::string_view
in my code, but not when abseil itself is built. That's already bad, and it all crashes down because std::string_view in GCC7 and
absl::string_view both hold a size_t and a pointer, but in flipped order, so a pointer would get interpreted as size and used in
allocations.
So I tried making a custom triplet to pass in -std=c++17, and it works in so far as everything receives that flag.
I was surprised, though, so see that it would still crash (can be reproduced w/ the string_line_builder_test).
Why?

```cpp
absl/strings/string_view.h says:
    #ifdef ABSL_HAVE_STD_STRING_VIEW
    #include <string_view>  // IWYU pragma: export
    namespace absl {
    using std::string_view;
    }  // namespace absl
    #else  // ABSL_HAVE_STD_STRING_VIEW
    // abseil's string_view implementation...

So it looks like ABSL_HAVE_STD_STRING_VIEW is defined in our code, but not at abseil build time.
Let's see why this is so.
First let's see the values of the built-in defines in GCC:
    touch /tmp/foo.h
    g++ -E -std=c++1z -dM /tmp/foo.h
That tells us
    #define __cplusplus 201703L
    #define __has_include(STR) __has_include__(STR)

absl/base/config.h has this code:
    #ifdef __has_include
    #if __has_include(<string_view>) && __cplusplus >= 201703L
    #define ABSL_HAVE_STD_STRING_VIEW 1
    #endif
    #endif
In GCC7 with -std=c++17, <string_view> is available.
Why does the detection still fail when building abseil?
Okay, so when compiling abseil, __cplusplus has the value
    note: #pragma message: __cplusplus=201103L
This was shown by
    #define QUOTE(name) #name
    #define STR(macro) QUOTE(macro)
    #pragma message "__cplusplus=" STR(__cplusplus)
So, it's C++11 mode it looks like. Why?
This is the full command line of the compiler call producing the above output:

usr/bin/c++
    -D__CLANG_SUPPORT_DYN_ANNOTATION__
    -I/home/jw/jw/vcpkg-jaws/buildtrees/abseil/src/b629c52e44-9bc7332865
    -fPIC
    -std=c++1z   <===========================
    -g
    -Wall
    -Wextra
    -Wcast-qual
    -Wconversion-null
    -Wmissing-declarations
    -Woverlength-strings
    -Wpointer-arith
    -Wunused-local-typedefs
    -Wunused-result
    -Wvarargs
    -Wvla
    -Wwrite-strings
    -Wno-missing-field-initializers
    -Wno-sign-compare
    -std=gnu++11   <===========================
    -MD
    -MT absl/base/CMakeFiles/base.dir/internal/sysinfo.cc.o
    -MF absl/base/CMakeFiles/base.dir/internal/sysinfo.cc.o.d
    -o absl/base/CMakeFiles/base.dir/internal/sysinfo.cc.o
    -c /home/jw/jw/vcpkg-jaws/buildtrees/abseil/src/b629c52e44-9bc7332865/absl/base/internal/sysinfo.cc
```

VCPKG_CXX_FLAGS is not appended to the end of the command line, so it seems I cannot overwrite other options.

# [2019-09-20 Fri]

We've got a pull request [[https://github.com/microsoft/vcpkg/pull/8248]] that adds c++17 as a feature.
Now we must install abseil as "vcpkg install abseil[c++17]" (quote everything after install or else zsh interprets
it as something). This works. I added a test in jaws because stuff like that is easily overlooked.

# [2019-09-23 Mon]

I've experimented some with bgfx. Everyone praises it, but I find it weirdly hard to use. Integrating it wasted a lot of time,
the shader compiler cannot be invoked from code without writing everything yourself at the moment, but what really kills me
is the lack of documentation. For instance, apparently, gl_VertexIndex is not supported, but try to find that in the docs.
Also the shader compiler will process that just fine, but it will then err out at runtime, without an error message.
Overall the error handling is very poor. I can't get up to speed with this at the moment, and I don't trust it to let me
figure it out enough.

# [2019-09-28 Fri]

Started watching the stream series niagara.

Ran into interesting problems debugging an example:
    /home/jw/jw/repos/jaws/_build/clion-debug/_output/jaws-example-vulkan-headless
    /home/jw/jw/repos/jaws/_build/clion-debug/_output/jaws-example-vulkan-headless: Symbol `vkCreateShaderModule' causes overflow in R_X86_64_PC32 relocation
    /home/jw/jw/repos/jaws/_build/clion-debug/_output/jaws-example-vulkan-headless: Symbol `vkCreateShaderModule' causes overflow in R_X86_64_PC32 relocation
    /home/jw/jw/repos/jaws/_build/clion-debug/_output/jaws-example-vulkan-headless: Symbol `vkEnumerateInstanceVersion' causes overflow in R_X86_64_PC32 relocation

This seems to be related to GCC7's -pie default where it will compile executables as shared objects, basically.
I added --no-pie to add_jaws_executable(). This didn't help.

Additionally (might be related, but not sure) the example crashes as soon as it wanted to call a vulkan function.
This seems to be a bigger problem with shared objects, address space randomization(?), and vulkan loading in general.
I'm not even sure if this can work.

For now I forced jaws to be a static library. This fixes things for now.

Also: this: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples

# [2019-09-28 Sat]

Compiling glsl to spir-v, then using spirv-cross to reflect over that to set descriptor set layouts for a pipeline
seems to work. I'd like to plan out how I'd like the shader stuff to work together. Doesn't mean we implement
all of it now or ever, but it makes me feel better about it.

The shader subsystem should work like this:
1. the shader system is set up with virtual file systems (n many, addressed by name). *rationale: built-in memory vfs plus user's real-file-system-backed vfs in the same shader system*
   the vfs abstracts file system reflection (which we later use for include file resolves).
   In its simplest form this is simply the real file system.
   The vfs interface should be powerful enough to allow it to easily specify a full file tree w/ full contents
   in code (the compiled-in resource compiler can then just be a code generator for such a vfs specification).
2. The shader system can be instructed to compile a single shader (i.e. a pipeline's particular stage) by
   specifying a vfs name and a shader source file.
   The type of shader can be specified or deduced from an extension (if any) or set with the code pragma. *preferably in that order*
   It will compile the specified source files, resolving include files to possibly yield more files using the originally specified vfs.
   It will store a list of file dependencies for this shader.
   Optionally and if supported, it will set up automatic observation of all addressed files through the vfs interface.
   *triggering a refresh for a shader module is always possibly, the file observation just adds an automatic path*
   The subsystem owns the returned shader handles, and each handle has a hash value attached through which we
   will detect necessary pipeline rebuilds.
3. [disconnected workflow] At pipeline creation time we have a function here that sets up the descriptor set layouts
   for a set of shader handles.

# [2019-09-30 Mon]

I tried to add the outcome lib (standalone non-boost version), ut it doesn't compile with gcc-7 when compiled standalong, and the vcpkg port (which apparently some
excludes building tests) does not install config packages. That's too much trouble.
I'm not the only one with those build problems: https://www.reddit.com/r/cpp/comments/7vdpyu/outcome_accepted_into_the_boost_c_libraries/dttbwff/

# 2019.10.01

Switched this file to markdown as well. Org mode support is just too spotty while everything syntax highlights markdown.

I need to think about data flow and caching a bit more.

This is what I know about how Granite does it:
* Create shader: file name + ? -> Vulkan::Shader.
	spir-v reflection is performed here to find out:
	* which binding points and locations are active?
    * how many bytes of push constants are in use?
* ...
* Create program: device.request_program(shader...)
	Here descriptor set layouts and allocators are created by processing all the
	reflection data we have at this point.

	hash(descriptor set layouts) + hash(push constant ranges) -> pipeline layout

Let's dig deeper here.
Device::request_program has these variants:
```cpp
// Request shaders and programs. These objects are owned by the Device.
Shader *request_shader(const uint32_t *code, size_t size);
Shader *request_shader_by_hash(Util::Hash hash);
```
both look up by hash, but the first variant computes the hash from the specified code first.
So this is specifically spir-v -> Shader object (which contains reflected data as described above.

```cpp
Program *request_program(const uint32_t *vertex_data, size_t vertex_size, const uint32_t *fragment_data,
						 size_t fragment_size);
Program *request_program(const uint32_t *compute_data, size_t compute_size);
Program *request_program(Shader *vertex, Shader *fragment);
Program *request_program(Shader *compute);
```
Here we also have the overloads of Shader and spir-v code, probably for convenience.
I don't know why the non-compute pipeline is limited to just vertex and fragment shader, probably it's just that he didn't
need the other stages.

The shaders are held in Device as `VulkanCache<Shader> shaders;` which is a
non-owning (I think) map of Shader objects. The key for all the hash tables is always a hash value directly.
`Shader *Device::request_shader(const uint32_t *data, size_t size)`:

So this doesn't seem particularly magic. We can look up the transform output if we have the hash of the input.

# 2019.10.02
## Data transformations (again)

First off: we don't have to model these very explicitly. If the user drives data updates, she can just pass on results
from transformation stage to transformation stage. A stage doesn't need to know anything about the input but
an opaque hash value. An example:
```cpp
Shader s = get_shader("shader_source.frag");
Program p = get_program(s, s2, ...);
bind_program(p);
```
Like in granite, each stage could just use the hash of the inputs for lookup.
This works fine in the forward direction, but what if we want `get_shader` to also observe the source file
and update the shader when it changes? In such a case we expect the program to update as well.
Of course, if we do the above code all the time, it could just work itself out. 
The code effectively refreshes (or changes) the dependencies through control flow, the dependencies are never
persisted.

This is very similar to the distinction between **immediate mode GUI** and a **retained mode GUI**.

Is this maybe the essence of the difference between immediate mode and retained mode?
Dependencies (structure, layout, are pretty much other words for them) as reflectable models in retained mode vs 
them being implicitly defined by the control flow itselt?

The following are some thoughts I'm just having, I'm not sure how much sense they make:

In a sense, I guess that mmediate mode is always at least the lowest layer. We can after all consider chained function calls
an immediate mode. People build retained mode on top of things.
In reverse, this might mean that dependencies on the lowest level are always defined by control flow.
Retained mode things model dependencies and reason about them (like "solving"/optimizing a layout description),
then at a particular time they "dump" the result in immediate mode fashion internally, e.g. when actually drawing
the resulting controls in the correct order and states. This effectily replaces the single immediate call or calls like
"updateGui()" with the concrete control flow necessary for the current state of the GUI. 
Retained systems at their outermost interface must be connected to the immediate world, we often call that "polling"
in that context.

In my context this could mean something like this: our retained system models the dependencies (n source files -> shader) and (n shaders -> program).
When told so (polling), it evaluates any changed nodes in the resulting tree of dependencies, redoing any needed data transformations.

The question is whether we really want to start a retained system at this layer. I kinda feel there's benefit in delaying this
and just offer immediate mode building blocks here.
We can support shader source observing with immediate mode as well, as long as we "live" the immediate mode style and don't expect
a "setup.... while (1) { render-what-we-setup-earlier }" style. Instead, we must actively drive the rendering each frame.
Note that this is just what a retained system would have to do.
I think as long as we're still in self-finding phase regarding the engine, we should hold off on the retained idea and play around 
with flexible immediate mode style pieces more.

Let's take another look at granite:
Granite's shader class wraps the vulkan resource and the extra data we need with it:
```cpp
class Shader : public HashedObject<Shader>
{
...
private:
	Device *device;
	VkShaderModule module;
	ResourceLayout layout;

	void update_array_info(const spirv_cross::SPIRType &type, unsigned set, unsigned binding);
};
```
What exactly is a `HashedObject`?
```cpp
template <typename T>
using HashedObject = Util::IntrusiveHashMapEnabled<T>;
```
Okay, what is that then?
```cpp
template <typename T>
class IntrusiveHashMapEnabled : public IntrusiveListEnabled<T>
{
public:
	void set_hash(Util::Hash hash) { intrusive_hashmap_key = hash; }
	Util::Hash get_hash() const { return intrusive_hashmap_key; }
private:
	Hash intrusive_hashmap_key = 0;
};
template <typename T>
struct IntrusiveListEnabled
{
	IntrusiveListEnabled<T> *prev = nullptr;
	IntrusiveListEnabled<T> *next = nullptr;
};
```
Okay, so it stores a hash value intrusively (i.e., in the user's objects).
Notably, it looks like Shader is not a ref-counted class (I assume if it was it'd be done intrusively as well).
So where are Shaders stored in a hash table or list?
```cpp
VulkanCache<Shader> shaders;
```

```cpp
template <typename T>
using VulkanCache = Util::IntrusiveHashMap<T>;
```

```cpp
#ifdef GRANITE_VULKAN_FILESYSTEM
	ShaderManager &get_shader_manager();
	TextureManager &get_texture_manager();
	void init_shader_manager_cache();
	void flush_shader_manager_cache();
#endif
```

# 2019.10.06

xxhash has a vcpkg port, but it sucks. No config is installed, and
I need either a cmake static lib target or direct access to the .c file,
both I don't get if I just have the include path through other more properly
supported vcpkg packages.
git submodule it is.

Also, I'm currently leaning towards somewhat retained. I fear that
with immediate style (rebuild dependencies and structures all the time)
we will have to do more hash computation of large data structures than
otherwises necessary and I'd like to avoid that.

# 2019.10.07

spdlog has a new version, and it looks good and someone should update the vcpkg port.
I worry a bit about its process-global logger registry and the default logger,
does that work in static/shared combinations on windows and linux?
I would think so, spdlog seems widely enough used, so maybe there's something
to be learned there.
I want something like that for convenience, too, after all.
Passing a Jaws* around is okay, but it's really cumbersome for JAWS_FATAL
and friends.

# 2019.10.08

spdlog new version is integrated (as submodule for now, don't want to create the
vcpkg port now). 
