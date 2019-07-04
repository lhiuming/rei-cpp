# REI: Rendering Experiment Infrastructure

This project is still at its :clap:*initial stage*:clap:, so really no interseted stuff is presented :(.

Currently the first goal is to implement a real-time ray-traced physically based renderer, with procedurally generated mesh and several analytical light sources. Stay tuned! :rocket::sparkles:

## Screenshots

> Look what I found! A boring cube floating in the air!  ---- lhiuming

![](/docs/img/screenshots/raytraced_dull_cube.png)

## Download

Unstable build for samples can be downloaded on [the release page](https://github.com/lhiuming/REI/releases). Notice that for real-time ray-traced samples, only Windows machine equipped with RTX hardware is supported.

## Build

Requirements: 

- [CMake](https://cmake.org/) 3.13 (or higher),
- [Visual Studio](https://visualstudio.microsoft.com/) 2017 (or higher), with the following components:
	- Game development with C++ (Toolset),
	- Windows 10 SDK 10.0.17763.0 (or higher).
- Most updated driver for your RTX graphic card.

Clone the repository and update submodules, then generate a Visual Studio solution with cmake: 

	// in root directory of this repository
	mkdir build
	cd build
	cmake ../
	ii REI.sln

Now you can build and debug any of the projects under the "REI" solution:

1. Select and set "Samples/rt_raytracing_demo" as start-up project.
2. Press "F5" to build and start debugging. You should be able to see a floating cube. Try to move the camera with dragging and scrolling.

