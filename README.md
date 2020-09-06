# Ravine
Ravine is a graphics framework built with Vulkan.

![Preview Image](/images/preview_1_0a.png)

Used libraries are:
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

It's a layer of framework for you to create your game ontop of (still on WIP):
- Integrated UI with ImGUI.
- Loading of 3D meshes and animations with Assimp.
- Support for animation blending (WIP).
- Runtime Shader Compilation with ShaderC.
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
 
 Included pipeline(s):
 - Skinned Phong with Texture
 - Skinned Wireframe
 - Static Phong with Texture
 - Static Wireframe
 
 Descriptor Set Frequencies:
 - Global (frame related set)
 - Material (material related set)
 - Model (instance related set)
