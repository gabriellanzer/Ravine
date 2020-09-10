# Ravine
Ravine is a graphics framework built with Vulkan:

![Preview Image](/images/preview_1_0a.png)

## Used libraries:
- Vulkan
- GLFW
- ImGUI
- Volk
- GLM
- FMT
- Assimp
- EA's Stl
- Google's ShaderC
- SPIRV-Reflect

## Features so far:
- Integrated UI with ImGUI
- Loading of 3D meshes and animations with Assimp
- Support for animation blending (WIP)
- Runtime Shader Compilation with ShaderC
- Vulkan wrapping structures for:
  - Logical Device
  - Swap Chain
  - Render Pass
  - Window
  - Virtual Camera
- Vulkan auxiliar functions for:
  - Dynamic (Host Visible/Coherent) Buffer creation
  - Persistent (Device Local) Buffer Creation
  - Texture 2D creation
  - Layout transition
  - MipMap creation
  - Window Resize Events
  - Animation Blending
 
## Included pipeline(s):
 - Skinned Phong with Texture
 - Skinned Wireframe
 - Static Phong with Texture
 - Static Wireframe
 
## Descriptor Set Frequencies:
 - Global (frame related set)
 - Material (material related set)
 - Model (instance related set)

---

## TODO(s):
 - Integrate with [Ravine ECS](https://github.com/gabriellanzer/Ravine-ECS) architecture
 - Individual update rate for UI (render fixed in 60FPS, blit every other frames)
 - Optimize animations performance
 - Scene Graph (of sorts, ECS based)
 - VkPipeline abstraction and boilerplate for ease of use
 - Node Editor for:
   - Render Pass Graph
   - Animation State Machines
 - Resource Manager (integrated with ECS)
 - Audio (duh)
   - 2D/3D Sound Sources
   - 7.1 Spatial Support
   - Mixers and Audio Source Pools
 - PBR default pipeline
 - Render Pass with deferred renering
 - Get rid of the 2K lines *ravine.cpp* file
 - Project a layer of abstraction for DirectX 12.0
