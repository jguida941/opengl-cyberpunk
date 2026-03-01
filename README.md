# OpenGL Cyberpunk Scene

This repository contains a single OpenGL project: a stylized cyberpunk version of the 6-2 assignment scene.

![Final cyberpunk render](Projects/6-2_Assignment/assets/cyberpunk-final.png)

## Repository Layout

- `3DShapes/`: shared primitive mesh code used by the scene
- `Libraries/`: bundled OpenGL dependencies (GLFW, GLEW, GLM)
- `Projects/6-2_Assignment/`: the only assignment project in this repo
- `Utilities/`: shared utility code used by the project (including shader helpers)
- `setup-macos.command` / `setup-windows.ps1`: environment setup scripts

## Scene Summary

The project starts from the 6-2 assignment base and applies a cyberpunk visual direction:

- light-purple ground plane
- neon-inspired object colors
- cyan and magenta lighting contrast
- deep purple-black background tone

## What Was Built

- stylized 3D seesaw composition using primitive meshes
- shader-lit materials with custom color tuning
- camera movement with mouse-look interaction

## Build And Run (Windows)

1. Open `Projects/6-2_Assignment/6-2_Assignment.sln` in Visual Studio 2022.
2. Set configuration to `Debug` and platform to `Win32`.
3. Press `F5` to build and run.

Optional helper scripts:

- `Projects/6-2_Assignment/scripts/open-solution.bat`
- `Projects/6-2_Assignment/scripts/build-and-run.ps1`

## Controls

- `W` / `A` / `S` / `D`: move camera
- `Mouse`: look around
- `TAB`: toggle mouse capture
- `ESC`: close application

## Requirements

- Windows 10 or Windows 11
- Visual Studio 2022 with Desktop development with C++
- OpenGL-compatible GPU and drivers
