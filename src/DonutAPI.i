/*                                                                                      DonutAPI.i — SWIG interface file
                                                                                       Covers these 13 languages via SWIG 
                                                                               Python, Java, JavaScript (Node.js), Octave, Ruby, Perl
                                                                                            Lua, PHP, Go, C#, R, D, C++ 

                                                                                             C, Swift, Rust, Zig —
                                                                      Cannot use SWIG directly because they require plain C linkage.  
                                                                         Use DonutAPI_C.h / DonutAPI_C.cpp instead for those
*/

%module DonutAPI

// Code injected verbatim into the generated C++ wrapper 
%{
#include "DonutAPI.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
%}

// Standard-library SWIG support

%include <stdint.i>
%include <std_string.i>
%include <std_wstring.i>
%include <std_vector.i>
%include <std_pair.i>

// Suppress macros that confuse the SWIG parser 
// DNAPI expands to dllexport / visibility("default") annotations.
// Defining it away means SWIG sees a clean function prototype

#define DNAPI
// Guard against Windows-only macros leaking in on the parse host

#ifndef _WIN32
#  define _CRT_SECURE_NO_WARNINGS
#  define NOMINMAX
#endif

// Suppress types that cannot be wrapped 
// Audio contains a C99 flexible array member (uint16_t samples[]).
// SWIG has no way to model that; callers use the plain WAV file API.

%ignore Audio;

// CHAR_INFO is a platform shim for the Win32 struct; it is an internal
// detail and is not part of the public API surface.

%ignore CHAR_INFO;

// Suppress wstring-based functions (replaced by UTF-8 helpers below)

%ignore DrawString;
%ignore DrawStringAlpha;
%ignore SetWindowName;
%ignore SaveSprite;
%ignore LoadSprite;
%ignore SaveTexture;
%ignore LoadTexture;

// Ignore the complex C++11 default arguments
%ignore GenerateRandMap;
%ignore GeneratePerlinMap;

// Suppress unordered_map-based functions (replaced by vector helpers)

%ignore TileColor;
%ignore DisplayMap;

// Suppress raw-pointer array function (replaced by vector helper)

%ignore DrawBillboard;

// Suppress inline tile-color string constants
// These are C++ inline variables; SWIG support is inconsistent across
// language backends.  Users should write the ANSI escape codes directly
// or call one of the TileColorArr helpers.

%ignore TILE_RESET;
%ignore TILE_BLACK;
%ignore TILE_DARK_BLUE;
%ignore TILE_DARK_GREEN;
%ignore TILE_DARK_CYAN;
%ignore TILE_DARK_RED;
%ignore TILE_DARK_MAGENTA;
%ignore TILE_BROWN;
%ignore TILE_GREY;
%ignore TILE_DARK_GREY;
%ignore TILE_BLUE;
%ignore TILE_GREEN;
%ignore TILE_CYAN;
%ignore TILE_RED;
%ignore TILE_MAGENTA;
%ignore TILE_YELLOW;
%ignore TILE_WHITE;

// Template instantiations
// Every concrete template the public API exposes must be %template'd so
// SWIG can generate a wrapper type for it in the target language

%template(CharVector)        std::vector<char>;
%template(CharVectorVector)  std::vector<std::vector<char>>;
%template(StringVector)      std::vector<std::string>;
%template(TileDefVector)     std::vector<TileDef>;
%template(DoubleVector)      std::vector<double>;
%template(FloatPair)         std::pair<float,float>;
%template(FloatPairVector)   std::vector<std::pair<float,float>>;
%template(IntPair)           std::pair<int,int>;

// UTF-8 / vector replacements for suppressed function

%inline %{

// Helpers: wstring to UTF-8 string 
static std::wstring _dn_toWide(const std::string& s)
{
    return std::wstring(s.begin(), s.end());
}

/// SetWindowName — accepts a UTF-8 string instead of wstring
static void SetWindowNameStr(const std::string& name)
{
    SetWindowName(_dn_toWide(name));
}

/// DrawString — accepts a UTF-8 string instead of wstring
static void DrawStringStr(int x, int y, const std::string& s,
                          unsigned short color = WHITE)
{
    DrawString(x, y, _dn_toWide(s), color);
}

/// DrawStringAlpha — accepts a UTF-8 string instead of wstring
static void DrawStringAlphaStr(int x, int y, const std::string& s,
                               unsigned short color = WHITE)
{
    DrawStringAlpha(x, y, _dn_toWide(s), color);
}

/// SaveSprite — accepts a UTF-8 path instead of wstring
static bool SaveSpriteStr(const std::string& path)
{
    return SaveSprite(_dn_toWide(path));
}

/// LoadSprite — accepts a UTF-8 path instead of wstring
static bool LoadSpriteStr(const std::string& path)
{
    return LoadSprite(_dn_toWide(path));
}

/// SaveTexture — accepts a UTF-8 path instead of wstring
static bool SaveTextureStr(const Texture& tex, const std::string& path)
{
    return SaveTexture(tex, _dn_toWide(path));
}

/// LoadTexture — accepts a UTF-8 path instead of wstring
static bool LoadTextureStr(const std::string& path, Texture& outTex)
{
    return LoadTexture(_dn_toWide(path), outTex);
}

// Helpers: parallel-vector replacements for unordered_map APIs 
static std::string TileColorArr(char tile,
                                const std::vector<char>&        keys,
                                const std::vector<std::string>& vals)
{
    std::unordered_map<char, std::string> m;
    for (size_t i = 0; i < keys.size() && i < vals.size(); ++i)
        m[keys[i]] = vals[i];
    return TileColor(tile, m);
}

static void DisplayMapArr(int rows, int spritex, int spritey,
                          const std::vector<char>&        keys,
                          const std::vector<std::string>& vals)
{
    std::unordered_map<char, std::string> m;
    for (size_t i = 0; i < keys.size() && i < vals.size(); ++i)
        m[keys[i]] = vals[i];
    DisplayMap(rows, spritex, spritey, m);
}

// Helper: DrawBillboard with a vector z-buffer 
static void DrawBillboardVec(const std::vector<double>& zBuffer,
                             double worldX, double worldY,
                             double dirX,   double dirY,
                             double planeX, double planeY,
                             double posX,   double posY,
                             const Texture& sprite,
                             float scale = 1.0f)
{
    if (!zBuffer.empty())
        DrawBillboard(zBuffer.data(), worldX, worldY, dirX, dirY, 
                      planeX, planeY, posX, posY, sprite, scale);
}

%}  // end %inline

// Pull in the real header
// SWIG will parse every declaration that has not been suppressed above
// and generate a wrapper for it in the target language.
%include "DonutAPI.h"
