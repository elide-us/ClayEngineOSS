# ClayEngineOSS
A game engine for Windows

# Features
- DirectX 11/12, CUDA
- WinSocks2 NBIO/IOCP
- Client/Server (MMO)
- Voxel LOD
- ARPG systems

#### GAME/BOLTS/ClayEngine A DirectX MMO Engine

Eliminate the editor! Build the game in the game.

Advanced debugging features allow for limitless game configurations from single player to massive scale complex network infrastructure.s

Unleash Your Creativity with our GAME Engine: Craft captivating stories, quests, characters, and world events effortlessly.

Empower Your Vision with BOLTS: Seamlessly build massive scale client-server game systems with ease.

Sculpt Your World with ClayEngine: Dive into the immersive voxel-based LOD system for unparalleled detail and depth. Experience the future of game development today!

## GAME - Generic Adventure Maker Engine
The game is the editor, the editor is the game.

## BOLTS - Basic Online Logic Technology System
Advanced networking stack for thousands of simultaneous connections to the same game world and tools that allow the players to create infinite adventures!

## ClayEngine
An advanced Voxel LOD terrain system for rendering game worlds of limitless scale.

# How To Build
Clone the repo to your local system. Inside the repo, clone the DirectXTK repo from https://github.com/microsoft/DirectXTK.git (June 2024)

Folders should look like this:
```
+ ClayEngine
+--+ Artifacts
|  +--- Debug
|  +--- Release
+--+ ClayEngineClient
|  +--- content
+--- ClayEngineLibrary
+--- ClayEngineHeadless
+--- DirectXTK
```

Open the Visual Studio solution file with Visual Studio 2022 and build the ClayEngineClient project. This should cause the DirectX toolkit to compile shaders on first build.