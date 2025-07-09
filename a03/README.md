GI - Framework
=======

Global Illumination (GI) framework for use in teaching. Uses [Embree](https://github.com/RenderKit/embree) for fast BVH construction and traversal, [Assimp](https://github.com/assimp/assimp) for asset management, [GLM](https://github.com/g-truc/glm) for vector math, and [Dear ImGui](https://github.com/ocornut/imgui) as user interface.
Also supports cross-compilation to WebAssembly using [Emscripten](https://emscripten.org/).

## Build (Linux)

Install the required dependencies (C++17 development environment, CMake, OpenMP, OpenGL) via your favorite package manager (all pre-installed in the CIP Pool), for example on Ubuntu:

    $ apt-get install build-essential cmake ninja-build libgomp1 libx11-dev xorg-dev libopengl-dev freeglut3-dev

Download and unpack `dependencies.zip` and `data.zip` from StudOn into the same directory where you unpacked the assignment `a##.zip` or in the root directory of the source tree.
To configure and build, navigate to the root directory (where the top-level `CMakeLists.txt` is located) and type: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel`.

## Experimental: Build (Windows)

Note that we only guarantee the code to run on the CIP Pools, Windows support is experimental and might or might not work. You will have to tackle eventual problems yourself.

Install and configure Visual Studio Code for C++ desktop development either using MSVC (<https://code.visualstudio.com/docs/cpp/config-msvc>), or GCC via the Windows Subsystem for Linux (WSL) (<https://code.visualstudio.com/docs/cpp/config-wsl>).
If using the WSL, see `Build (Linux)` above for installing dependencies.
Make sure to install the [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack) for CMake integration.
Download and unpack the `dependencies.zip` and `data.zip` from StudOn into the same directory where you unpacked the assignment `a##.zip` or in the root directory of the source tree.
Open the `a##` folder containing the top-level `CMakeLists.txt` file in VSCode (File -> Open Folder, or hit Ctrl+K Ctrl+O).
If not exectued automatically, CMake configure and build the source (with the `amd64` kit), for example by hitting F1 and typing `configure` or `build` and selecting the respective CMake actions, or by clicking the respective buttons on the blue toolbar on the bottom of your screen.
You might also want to build in `Release` mode, or rendering will take a while.

## Experimental: Build (Emscripten)

To build the experimental web version of the GI framework using WebAssembly, see [emscripten/README.md](./emscripten/README.md).

## Run the Code

To render, execute the `gi` executable in the root directory and optionally provide a path to a JSON configuration file, for example: `gi configs/a01.json`. You may also simply drag-and-drop files onto the preview window.
Note that, if no OpenGL context is available (e.g. when connected to the CIP pools via SSH), rendering is still possible, albeit without the live preview.

## Preview Controls

Use the keys `W A S D R F` to move the camera and drag with the left mouse button pressed to rotate the camera.
You can adjust the camera movement speed via scrolling with the mouse wheel.
Use right mouse click to set the focus plane for depth of field, if the auto focus is disabled.
Press the spacebar to restart rendering and Escape to exit.
See the various GUI options in the top of the preview window to modify the scene and rendering parameters.

## Configs

All options and settings can optionally be imported and exported from/to a single JSON file.
Some example configuration files are located in the `configs` directory.

## Issues / Suggestions / Feedback

Please mail to <nikolai.hofmann@fau.de>.
