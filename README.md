# DonutAPI 1.0

A lightweight, cross-platform terminal game engine inspired by OneLoneCoder's ConsoleGameEngine.

Write games and simple software directly in the terminal with pixels, sprites, shapes, maps, audio, and input — no heavy dependencies.

## Features

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

## Start

### 1. Download

Download the latest release for your platform from the [Releases page](../../releases).

### 2. Example (C++)

```cpp
#include "DonutAPI.h"

int main()
{
    InitWindow(120, 40, 8, 16);
    SetWindowName(L"My First Donut Game");
    ShowConsoleCursor(false);
    SetFPS(60);

    while (!WindowShouldClose())
    {
        Fill(0, 0, 120, 40, PIXEL_SOLID, BLACK);

        DrawString(10, 10, L"Hello, DonutAPI!", WHITE);
        FillCircle(60, 20, 8, PIXEL_SOLID, RED);

        UpdateScreen();
    }

    DestroyWindow();
    return 0;
}
```
## License

MIT License

Copyright (c) 2026 EAS_Studio

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
