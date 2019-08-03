# REI: Rendering Experiment Infrastructure

This project is basically a personal playground for trying out all kinds of rendering stuff, mostly in real-time side.

Currently the goal is to implement a real-time ray-traced physically-based renderer, capable of rendering some procedurally generated meshes and several analytical light sources. After a initial stage, the built-in hybrid pipeline finally produces some :rocket: nice-looking :sparkles: images. Check the screenshots below!

## Features

- A hybrid render pipeline implemented as raytracing-from-G-buffer, for full-dynamic global illumination
- Multi-bounce indirect illumination, accumulated across frames (though noisy and converges slowly ...)
- Stochastic area shadow
- GGX + Lambertian surface shading
- Extensible and platform-agonistic Renderer API*

*: potentially :), since the actual implementation is Windows-only at this time...

## Screenshots

Dynamic soft shadows and multi-bounce indirect illumination (accumulated over frames): 

![](/docs/img/screenshots/raytraced_demo.png)

You can see how the image converges when the camera is moved (discarding all previous frames): 

![](/docs/img/screenshots/raytraced_demo.gif)

## Download

Latest build, containing sample apps that produce the presented screenshots, can be downloaded on [the release page](https://github.com/lhiuming/REI/releases). (Windows Only)

(Notice: in order to run real-time ray-traced samples, you would need a RTX graphic card.)

## Build

Requirements: 

- [CMake](https://cmake.org/) 3.13 (or higher),
- [Visual Studio](https://visualstudio.microsoft.com/) 2017 (or higher), with the following components:
	- Game development with C++ (Toolset),
	- Windows 10 SDK 10.0.17763.0 (or higher).
- Most updated driver for your RTX graphic card.

Clone the repository and update submodules, then generate a Visual Studio solution with cmake: 

	// in repository root directory 
	mkdir build
	cd build
	cmake ../
	// open the solution file
	ii REI.sln

Now you can build and debug any of the projects under the "REI" solution:

1. Select and set "Samples/rt_raytracing_demo" as start-up project.
2. Press "F5" to build and start debugging. You should be able to see some cuboids and spheres sitting on a thick plane. Try to move the camera with dragging and scrolling.

## Reference 

Here are some excellent articles/papers/talks that help me a lot when I design the abstraction layer and build some of the shinny features: 

1. Colin Barré-Brisebois, et al, ["Hybrid Rendering for Real-Time Ray Tracing" in \<Ray Tracing Gems\>](http://www.realtimerendering.com/raytracinggems/). Good reference on how to design a hybrid pipeline utilizing today's not-that-powerful raytracing hardware. 
2. Eric Heitz, Stephen Hill and Morgan McGuire, [\<Combining Analytic Direct Illumination and Stochastic Shadows\>](https://eheitzresearch.wordpress.com/705-2/). The idea of stochastic (raytraced) shadow for noise-free analytic area lights.
3. Graham Wihlidal, [\<Halcyon: Rapid Innovation using Modern Graphics\>](https://www.khronos.org/assets/uploads/developers/library/2019-reboot-develop-blue/SEED-EA_Rapid-Innovation-Using-Modern-Graphics_Apr19.pdf). Shares some great designs decision for an experimental graphic engine. 
4. Tomas Akenine-Möller, et al, [\<Real-Time Rendering\>](http://www.realtimerendering.com/)(4th Edition). "The Book" for general real-time topics. Chapter 9 is a comprehensive introduction to physically-based rendering, including an up-to-date overview of PBR practice in real-time application. 
