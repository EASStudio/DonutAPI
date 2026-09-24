# Intro
 
I started the API on February 11th, 2026 when I was 16. I was bored and instead of having a life I decided to make something. 
I always liked playing around with different things in the terminal, especially making simple games. I started to want to make a cross platform text based rpg,
called Quest. It's supposed to be a rogue-like and dwarf fortress-like game but in development I realized that I needed a something to handle math, graphics, 
input, ect. I needed a cross platform terminal graphics and input library in the big 26, now I started to look and eventually found javidx9's olcPixelGameEngine.
Now it was cross platform, it was written in c++ and had other language support, but the thing is it was odd to use. Call it a skill issue but I was looking
for something simple like raylib, with all the features that the olcPixelGameEngine has. So I decided to make my own, in three months no less. If I didn't come
across Javidx9's engine this probably won't exist and I wouldn't have learned as much as I did from making it. So thanks Javidx9 for the engine.

Enjoy, and please go check his out: https://github.com/OneLoneCoder/olcPixelGameEngine

## DonutAPI 1.0:

### Features

On May 17th, 2026 I realsed the first verion of DonutAPI it had:
- **Cross-platform**: Windows, Linux, macOS
- **Pixel-perfect rendering** with colored characters
- **Sprite system** (create, save, load, draw)
- **Shape drawing** (lines, rectangles, circles, triangles, polygons, etc.)
- **Map generation**: Random + Perlin noise
- **Audio support**: Background music + SFX (WAV files)
- **Input handling**: Keyboard + Mouse
- **Texture system**
- **Billboard sprite rendering** (for pseudo-3D)
- **Rich math library** (vectors, matrices, quaternions, perlin noise)
- **Multiple language bindings**:
  - Full C++ API
  - Plain **C wrapper** (`DonutAPI_C`) — perfect for Rust, Zig, Swift, C
  - SWIG bindings: Python, Java, C#, etc.
	

# DonutAPI 1.1:

On September 24th, 2026 I realsed DonutAPI to 1.1 with these new things:
- **Fixed comments**: Correctly tells user what functions do
- **WFC functions added**: Support with textures and strings
- **ECS functions added**: Entity, Components, and System manager
- **More Colors for Strings Added**: Gold, Orange, Charcool, ect...
- **Rectangle struct and check collison points**: Check collision points for rects and circles
- **Rectangle and circle drawing functions**: Draw percise rectangles and circles with collision
- **Timer**: Timer ticks down form delta time
- **Mouse functions**
- **Math Helpers**