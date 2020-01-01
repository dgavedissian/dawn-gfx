# dawn-gfx
Multi-threaded graphics abstraction layer. Supports OpenGL 3+, WebGL and Vulkan.

### Dependencies

Most dependencies are pinned using CMake's `FetchContent` feature. However, the OpenGL and Vulkan renderers
both have their own dependencies:

#### OpenGL renderer

Windows and macOS have no dependencies, as OpenGL functions are loaded from the driver directly.
Linux platforms require Xorg development libraries.

Ubuntu:

    $ sudo apt-get install xorg-dev
    
Arch Linux:

    $ sudo pacman -S xorg-server

    
#### Vulkan renderer

The latest Vulkan SDK by LunarG needs to be installed from [here](https://vulkan.lunarg.com/sdk/home). Linux platforms also require Xorg development libraries.

Arch Linux provides Vulkan development libraries in the following package:

    $ sudo pacman -S vulkan-devel
