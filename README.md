# Apollo Engine

## What?

Apollo is just my own spin on a C++ game engine. Not my first try either, in fact, this is the second iteration of a project formerly called Breakout. I became dissatisfied with the way I had built the core of the engine, and I figured it was as good an opportunity as any to leverage the full potential of SDL3!
Thus, this rendering backend of the engine uses SDL's new GPU API to handle to low-level graphics shenanigans.

## Why?

I'm deeply interested in game engine programming, this project is just meant to be a learning exercise more than anything.
If you want an engine to make games, this probably isn't the right tool for the job, there are many infinitely better alternatives out there. Check out [Godot](https://godotengine.org/) if you haven't already.
If you're clueless about game engines, and want to get some insight into how they're put together: welcome, make yourself at home, feel free to look around!

## Quick Setup Guide

### A Note On Platforms

As of right now, I have only tested the project on Windows, so other platforms aren't *officially* supported. But given that it builds with Clang and runs with Vulkan, in theory nothing stops you from making it work on Linux and Mac.
I've been told it can build on Linux, but you might have to fiddle around a little. SDL requires a window management backend like Wayland, you'll find more information [here](https://wiki.libsdl.org/SDL3/README-wayland).
The compiler might also give you some warnings which will prevent the build from completing, for now the "fix" is to suppress them using the cmake `CMAKE_CXX_FLAGS` variable.

For various reasons, I have decided to use C++20 in my code, and the project will **not** build without it. This shouldn't be a problem, as this version is well supported by all the major toolchains, you'll just have to make sure
you're using a recent enough version of your compiler. The build system is CMake, in part because it is the one I'm most comfortable with, but more importantly because it's kind of the "de facto standard[^1]" used by all my dependencies.
You'll need version 3.24 or higher for this to work.

[^1]: We could argue about build systems all day, as we all know there are plenty of alternatives. CMake being such a popular option does make my life that much easier,
but if you'd like to add support for another build system, feel free to open an issue. Just be aware I won't necessarily be able to maintain it on my own.

### How To Build

First, clone the repo: `git clone --recurse-submodules git@github.com:EddieBreeg/Apollo.git`. If you forgot the recursive flag, you can download the submodules after the fact:
`git submodule update --init`.

Then, configure the project with CMake: `cmake --preset <configurePreset>`. If you use MSVC or Ninja, use the appropriate preset from [CMakePresets.json](CMakePresets.json).
Otherwise, you can still use the `_default` preset and specify your generator of choice using the `-G` flag (see [this page](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#cmake-generators) for details).
You may also want to specify `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` if your intellisense relies on the `compile_commands.json` file.

Finally, build the project with `cmake --build`. When using MSVC/Ninja, build presets are available for the different configurations. Otherwise you'll have to provide the `--config` flag yourself.
