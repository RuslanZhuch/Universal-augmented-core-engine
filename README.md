# Universal-augmented-core-engine
C++ Direct3d 12 engine for fast prototyping. Uses Blender for content creation. 

Lightweight c++20 game engine based on Direct3d 12.
It would support data streaming, node-based render and logic scripting. 

# Installation 

It requers Visual studio 2022 with c++20 (/std:c++latest)

# Some comments

I'm experimenting now with c++20 stuff like modules and concepts, and Visual studio's Intellisence still has a lot of bugs. It just doesn't work. 

I consider to move from asio-based tcp/ip client to something more lightweight, maybe WinSockets. But I wish it would be something more cross platform.

I try do not use dynamic allocation, so some custom allocators were created. They are quite raw right now and not optimized for multithreading.
