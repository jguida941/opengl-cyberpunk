# CS330 Content

This repository contains course work and utilities for CS-330 Computational Graphics and Visualization.

## What Is In This Repo

- `3DShapes/`: shared primitive mesh code used by projects
- `Libraries/`: bundled OpenGL-related third-party libraries
- `Projects/`: milestone and assignment projects (`1-2` through `8-2`)
- `Utilities/7-1_FinalProject/`: final project scene (picnic table + soda can + pizza + dice + ball)

## Recent Scene Detail Update

The final project has been updated back to higher polygon detail for visible mesh elements:

- soda can rim uses torus rings again
- dice pips use sphere meshes again
- torus segment density increased (`64 x 64`)

## Quick Setup

Run one of these scripts from the repo root:

- macOS:
  - Double-click `setup-macos.command` in Finder
  - or run `bash setup-macos.command`
- Windows (PowerShell):
  - `powershell -ExecutionPolicy Bypass -File .\setup-windows.ps1`

## Build/Open Notes

This coursework is primarily configured with Visual Studio solution files (`.sln`, `.vcxproj`).

- Recommended on Windows: open `Utilities/7-1_FinalProject/7-1_FinalProjectMilestones.sln`
- On macOS: use the setup script to install tooling and dependencies, then build/port with your preferred OpenGL workflow.
