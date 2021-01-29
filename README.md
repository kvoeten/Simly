# SIMLY

SIMLY is a project that enables rapid prototyping of Mixed Reality devices by offering ready to use, scalable communication technologies. This repository is for the Simly Unreal Engine plugin, which provides Blueprint interfaces with connected XR devices, handling most of the communication logic over TCP without requiring the user to implement such technologies themselves. 

# Available functionality

As this is a prototype system, it only offers a small set of functionality. 

1. The plugin includes a wrapper for Realsense Camera's (which requires the RealSense plugin to be installed!) that utilizes Unreal's (now built in) Lidar Point Cloud plugin to stream realtime point-clouds recorded with the camera into the 3D environment. 
2. The plugin offers functionality for a 4 analog sensor Simly interface through the 'force-sensor' class, and a 'angle request' function for interfacing with a motor. For further functionality, the system will have to be expanded. (These functions were created to prototype concepts, more generic functions aren't implemented yet and due to the project being finished likely never will be.)
3. The plugin offers a modified version of Jan Kaniewski's TCP convenience wrapper to easily establish TCP Communications: https://github.com/getnamo/tcp-ue4
4. The plugin requires you to set-up a simly system to interface with, more information on this can be found in the documentation.

# Installation

Make sure to install the Realsense Wrapper for Unreal Engine: https://github.com/IntelRealSense/librealsense/tree/master/wrappers/unrealengine4

Then download this repository and place it in the project or engine's plugins folder like any other unreal plugin. 

# Documentation

For more detailled instructions and some guides for setting up the whole Simly system with multiple XR devices, consult the documatation located at: https://simly.kazvoeten.com/
