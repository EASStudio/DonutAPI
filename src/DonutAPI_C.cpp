#include "DonutAPI_C.h"
#include "DonutAPI.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <cstdlib>
#include <cstring>

// Internal helpers

// UTF-8 std::string to std::wstring  (BMP codepoints only)
static std::wstring _toWide(const char* s)
{
    if (!s) return {};
    std::string str(s);
    return std::wstring(str.begin(), str.end());
}

// Borrow a DN_Texture pointer as a Texture reference
static Texture _asTex(const DN_Texture* dt)
{
    Texture t;
    t.width = dt->width;
    t.height = dt->height;
    t.pixels = dt->pixels;
    return t;
}

// Borrow a DN_Vector2 pointer as a Vector2 reference
static Vector2 _asVec2(const DN_Vector2* vc2)
{
    Vector2 vc;
    vc.x = vc2->x;
    vc.y = vc2->y;
    return vc;
}

// Logging

extern "C" void DN_InternalLog(const char* msg) 
{ 
    InternalLog(msg ? msg : "");
}

extern "C" int  DN_CheckStatus(void) 
{ 
    return CheckStatus();
}

extern "C" void DN_DebugLog(const char* msg) 
{ 
    DebugLog(msg);
}

// String window

extern "C" void DN_ClearScreen(void) 
{ 
    ClearScreen();
}

extern "C" void DN_Delay(int ms) 
{ 
    Delay(ms);
}

extern "C" void DN_SetMessage(const char* msg, int ttl) 
{ 
    SetMessage(msg ? msg : "", ttl);
}

extern "C" int  DN_GetHudTTL(void) 
{ 
    return GetHudTTL();
}

extern "C" void DN_TickHudTTL(void) 
{ 
    TickHudTTL();
}

extern "C" const char* DN_GetHudMessage(void)
{
    // Return pointer into a static std::string so the C caller gets a stable
    // const char* that outlives the call
    static std::string buf;
    buf = GetHudMessage();
    return buf.c_str();
}

// Map

extern "C" void DN_SetMapVector(const char* flat, int w, int h)
{
    if (!flat || w <= 0 || h <= 0) return;
    std::vector<std::vector<char>> m(h, std::vector<char>(w));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            m[y][x] = flat[y * w + x];
    SetMapVector(m);
}

extern "C" DN_IntPair DN_GetConsoleSize(void)
{
    auto p = GetConsoleSize();
    return { p.first, p.second };
}

extern "C" char* DN_GenerateRandMap(int w, int h, const DN_TileDef* tiles, int tc, unsigned int seed)
{
    std::vector<TileDef> tv(tc);
    for (int i = 0; i < tc; ++i) { tv[i].tile = tiles[i].tile; tv[i].weight = tiles[i].weight; }

    auto map = GenerateRandMap(w, h, tv, seed);

    char* flat = static_cast<char*>(std::malloc(static_cast<size_t>(w * h)));
    if (!flat) return nullptr;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            flat[y * w + x] = map[y][x];
    return flat;
}

extern "C" char* DN_GeneratePerlinMap(int w, int h, const DN_TileDef* tiles, int tc, float scale, unsigned int seed)
{
    std::vector<TileDef> tv(tc);
    for (int i = 0; i < tc; ++i) { tv[i].tile = tiles[i].tile; tv[i].weight = tiles[i].weight; }

    auto map = GeneratePerlinMap(w, h, tv, scale, seed);

    char* flat = static_cast<char*>(std::malloc(static_cast<size_t>(w * h)));
    if (!flat) return nullptr;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            flat[y * w + x] = map[y][x];
    return flat;
}

extern "C" void DN_FreeMap(char* ptr) 
{ 
    std::free(ptr);
}

extern "C" const char* DN_TileColor(char tile, const char* keys, const char** colors, int n)
{
    static std::string buf;
    std::unordered_map<char, std::string> m;
    for (int i = 0; i < n; ++i) m[keys[i]] = colors[i];
    buf = TileColor(tile, m);
    return buf.c_str();
}

extern "C" void DN_DisplayMap(int rows, int sx, int sy, const char* keys, const char** colors, int n)
{
    std::unordered_map<char, std::string> m;
    for (int i = 0; i < n; ++i) m[keys[i]] = colors[i];
    DisplayMap(rows, sx, sy, m);
}

// Sprite 

extern "C" void DN_CreateSprite(int w, int h) 
{ 
    CreateSprite(w, h);
}

extern "C" void DN_DestroySprite(void) 
{ 
    DestroySprite();
}

extern "C" void DN_SetSprite(int x, int y, short p)
{ 
    SetSprite(x, y, p);
}

extern "C" void DN_SetColor(int x, int y, unsigned short c) 
{ 
    SetColor(x, y, c);
}

extern "C" short DN_GetSprite(int x, int y) 
{ 
    return GetSprite(x, y);
}

extern "C" unsigned short DN_GetColor(int x, int y)
{ 
    return GetColor(x, y);
}

extern "C" short DN_SampleSprite(float x, float y)
{ 
    return SampleSprite(x, y);
}

extern "C" unsigned short DN_SampleColor(float x, float y)
{ 
    return SampleColor(x, y);
}

extern "C" bool DN_SaveSprite(const char* path)
{
    return SaveSprite(_toWide(path));
}

extern "C" bool DN_LoadSprite(const char* path)
{
    return LoadSprite(_toWide(path));
}

// Initialisation 

// Conversion helper: map C wrapper DN_TerminalMode to internal TerminalMode
static TerminalMode _fromDNMode(DN_TerminalMode m)
{
    switch (m)
    {
        case DN_TerminalMode::DN_Auto:      return TerminalMode::Auto;
        case DN_TerminalMode::DN_Existing:  return TerminalMode::Existing;
        case DN_TerminalMode::DN_SpawnNew:  return TerminalMode::SpawnNew;
        default:                            return TerminalMode::Auto;
    }
}

extern "C" int  DN_InitWindow(int width, int height, int fontw, int fonth, DN_TerminalMode mode)
{ 
    return InitWindow(width, height, fontw, fonth, _fromDNMode(mode));
}

extern "C" void DN_DestroyWindow(void) 
{ 
    DestroyWindow();
}

extern "C" void DN_SetWindowName(const char* name)
{ 
    SetWindowName(_toWide(name));
}

extern "C" void DN_SetFPS(int fps) 
{
    SetFPS(fps);
}

extern "C" bool DN_WindowShouldClose(void) 
{ 
    return WindowShouldClose();
}

extern "C" void DN_PollInputState(void) 
{ 
    GetKeyState();
}

extern "C" DN_KeyState DN_GetKey(int kc)
{
    KeyState k = GetKey(kc);
    return { k.k_Pressed, k.k_Released, k.k_Held };
}

extern "C" void DN_FlushKeys(void) 
{ 
    FlushKeys(); 
}

extern "C" DN_MouseState DN_GetMouseState(void)
{
    MouseState ms = GetMouseState();
    DN_MouseState r;
    r.x = ms.x;
    r.y = ms.y;
    r.leftHeld = ms.leftHeld;
    r.rightHeld = ms.rightHeld;
    r.middleHeld = ms.middleHeld;
    r.leftPressed = ms.leftPressed;
    r.rightPressed = ms.rightPressed;
    r.middlePressed = ms.middlePressed;
    r.leftReleased = ms.leftReleased;
    r.rightReleased = ms.rightReleased;
    r.middleReleased = ms.middleReleased;
    r.wheelDelta = ms.wheelDelta;
    return r;
}

extern "C" int DN_GetMouseX(void) 
{
    return GetMouseX();
}

extern "C" int DN_GetMouseY(void)
{
    return GetMouseY();
}

extern "C" void DN_ShowConsoleCursor(bool v) 
{ 
    ShowConsoleCursor(v);
}

extern "C" const char* DN_PollInput(void)
{
    static std::string buf;
    buf = PollInput();
    return buf.c_str();
}

extern "C" void DN_UpdateScreen(void)
{ 
    UpdateScreen();
}

// Draw 

extern "C" void DN_DrawPixel(int x, int y, short pixel, unsigned short color) 
{ 
    DrawPixel(x, y, pixel, color); 
}

extern "C" void DN_Clip(int* x, int* y) 
{ 
    Clip(*x, *y);
}

extern "C" void DN_Fill(int x1, int y1, int x2, int y2, short pixel, unsigned short color)
{
    Fill(x1, y1, x2, y2, pixel, color);
}

extern "C" void DN_DrawString(int x, int y, const char* s, unsigned short color)
{
    DrawString(x, y, _toWide(s), color);
}

extern "C" void DN_DrawStringAlpha(int x, int y, const char* s, unsigned short color)
{
    DrawStringAlpha(x, y, _toWide(s), color);
}

extern "C" void DN_DrawSprite(int x, int y) 
{ 
    DrawSprite(x, y);
}

extern "C" void DN_DrawLine(int x1, int y1, int x2, int y2, short pixel, unsigned short color)
{
    DrawLine(x1, y1, x2, y2, pixel, color);
}

extern "C" void DN_DrawRectangle(int x, int y, int s, short pixel, unsigned short color)
{
    DrawRectangle(x, y, s, pixel, color);
}

extern "C" void DN_FillRectangle(int x, int y, int s, short pixel, unsigned short color)
{
    FillRectangle(x, y, s, pixel, color);
}

extern "C" void DN_DrawRotableRectangle(int x, int y, int s, float rot, short pixel, unsigned short color)
{
    DrawRotableRectangle(x, y, s, rot, pixel, color);
}

extern "C" void DN_FillRotableRectangle(int x, int y, int s, float rot, short pixel, unsigned short color)
{
    FillRotableRectangle(x, y, s, rot, pixel, color);
}

extern "C" void DN_DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short pixel, unsigned short color)
{
    DrawTriangle(x1, y1, x2, y2, x3, y3, pixel, color);
}

extern "C" void DN_FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short pixel, unsigned short color)
{
    FillTriangle(x1, y1, x2, y2, x3, y3, pixel, color);
}

extern "C" void DN_DrawTriangleStrip(const DN_Vector2* points, int pointCount, short pixel, unsigned short color)
{
    if (!points || pointCount <= 0) return;

    std::vector<Vector2> verts(pointCount);
    for (int i = 0; i < pointCount; ++i)
    {
        verts[i].x = points[i].x;
        verts[i].y = points[i].y;
    }
    DrawTriangleStrip(verts.data(), pointCount, pixel, color);
}

extern "C" void DN_FIllTriangleStrip(const DN_Vector2* points, int pointCount, short pixel, unsigned short color)
{
    if (!points || pointCount <= 0) return;

    std::vector<Vector2> verts(pointCount);
    for (int i = 0; i < pointCount; ++i)
    {
        verts[i].x = points[i].x;
        verts[i].y = points[i].y;
    }
    FillTriangleStrip(verts.data(), pointCount, pixel, color);
}

extern "C" void DN_DrawTriangleFan(const DN_Vector2* points, int pointCount, short pixel, unsigned short color) 
{
    if (!points || pointCount <= 0) return;
    std::vector<Vector2> verts(pointCount);
    for (int i = 0; i < pointCount; ++i)
    {
        verts[i].x = points[i].x;
        verts[i].y = points[i].y;
    }
    DrawTriangleFan(verts.data(), pointCount, pixel, color);
}

extern "C" void DN_FillTriangleFan(const DN_Vector2* points, int pointCount, short pixel, unsigned short color)
{
    if (!points || pointCount <= 0) return;
    std::vector<Vector2> verts(pointCount);
    for (int i = 0; i < pointCount; ++i)
    {
        verts[i].x = points[i].x;
        verts[i].y = points[i].y;
    }
    FillTriangleFan(verts.data(), pointCount, pixel, color);
}

extern "C" void DN_DrawCircle(int xc, int yc, int r, short pixel, unsigned short color)
{
    DrawCircle(xc, yc, r, pixel, color);
}

extern "C" void DN_FillCircle(int xc, int yc, int r, short pixel, unsigned short color)
{
    FillCircle(xc, yc, r, pixel, color);
}

extern "C" void DN_DrawCircleSector(DN_Vector2 center, float radius, float startAngle, float endAngle, int segments, short pixel, unsigned short color) 
{
    DrawCircleSector(_asVec2(&center), radius, startAngle, endAngle, segments, pixel, color);
}

extern "C" void DN_FillCircleSector(DN_Vector2 center, float radius, float startAngle, float endAngle, int segments, short pixel, unsigned short color)
{
    FillCircleSector(_asVec2(&center), radius, startAngle, endAngle, segments, pixel, color);
}

extern "C" void DN_DrawEllipse(int xc, int yc, int a, int b, int angle, short pixel, unsigned short color)
{
    DrawEllipse(xc, yc, a, b, angle, pixel, color);
}

extern "C" void DN_FillEllipse(int xc, int yc, int a, int b, int angle, short pixel, unsigned short color)
{
    FillEllipse(xc, yc, a, b, angle, pixel, color);
}

extern "C" void DN_DrawWireFrameModel(const float* coords, int n, float x, float y, float r, float s, unsigned short color, short pixel)
{
    std::vector<std::pair<float, float>> verts(n);
    for (int i = 0; i < n; ++i)
    {
        verts[i].first = coords[i * 2];
        verts[i].second = coords[i * 2 + 1];
    }
    DrawWireFrameModel(verts, x, y, r, s, color, pixel);
}

// Utility 

extern "C" float DN_GetElapsedTime(void) 
{ 
    return GetElapsedTime();
}

extern "C" int DN_GetScreenWidth(void) 
{ 
    return GetScreenWidth();
}

extern "C" int DN_GetScreenHeight(void) 
{ 
    return GetScreenHeight(); 
}

extern "C" int DN_GetRandomValue(int min, int max)
{
    return GetRandomValue(min, max);
}

extern "C" int DN_SetRandomSeed(unsigned int seed) 
{
    return SetRandomSeed(seed);
}

extern "C" DN_IntPair DN_GetMousePos(void)
{
    auto p = GetMousePos();
    return { p.first, p.second };
}

// Texture 

extern "C" DN_Texture DN_CreateTexture(int w, int h)
{
    Texture t = CreateTexture(w, h);
    return { t.width, t.height, t.pixels };
}

extern "C" void DN_DestroyTexture(DN_Texture* dt)
{
    if (!dt) return;
    Texture t = _asTex(dt);
    DestroyTexture(t);
    // DestroyTexture zeroes the struct it receives; mirror that here
    dt->pixels = nullptr;
    dt->width = 0;
    dt->height = 0;
}

extern "C" void DN_SetTexPixel(DN_Texture* dt, int x, int y, unsigned short c)
{
    Texture t = _asTex(dt);
    SetTexPixel(t, x, y, c);
}

extern "C" unsigned short DN_GetTexPixel(const DN_Texture* dt, int x, int y)
{
    Texture t = _asTex(dt);
    return GetTexPixel(t, x, y);
}

extern "C" unsigned short DN_SampleTexture(const DN_Texture* dt, float u, float v)
{
    Texture t = _asTex(dt);
    return SampleTexture(t, u, v);
}

extern "C" bool DN_SaveTexture(const DN_Texture* dt, const char* path)
{
    Texture t = _asTex(dt);
    return SaveTexture(t, _toWide(path));
}

extern "C" bool DN_LoadTexture(const char* path, DN_Texture* outDt)
{
    if (!outDt) return false;
    Texture out{};
    bool ok = LoadTexture(_toWide(path), out);
    if (ok) { outDt->width = out.width; outDt->height = out.height; outDt->pixels = out.pixels; }
    return ok;
}

// Audio 

extern "C" void DN_CreateMusicFile(const char* fn, double bpm, double beat, int sr, const DN_Music* notes, int nc, float vol)
{
    // DN_Music and Music are layout-identical (both are {double, double})
    CreateMusicFile(fn, bpm, beat, sr, reinterpret_cast<const Music*>(notes), nc, vol);
}

extern "C" void DN_PlayMusicFile(const char* fn)
{
    // The two trailing output-pointer params are unused by the implementation.
    PlayMusicFile(fn, nullptr, nullptr);
}

extern "C" void DN_StopMusic(void) 
{ 
    StopMusic();
}

extern "C" bool DN_IsMusicPlaying(void) 
{ 
    return IsMusicPlaying();
}

extern "C" void DN_PlaySFX(const char* fn, float vol) 
{ 
    PlaySFX(fn, vol);
}

// Billboard Sprite

extern "C" void DN_DrawBillboard(const double* zBuffer, double wx, double wy,double dx, double dy, double px, double py, double ox, double oy, const DN_Texture* sprite, float scale)
{
    if (!sprite) return;
    Texture t = _asTex(sprite);
    DrawBillboard(zBuffer, wx, wy, dx, dy, px, py, ox, oy, t, scale);
}