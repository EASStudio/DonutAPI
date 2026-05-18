/*
													DonutAPI 1.0 is a light weight terminal API | Simple graphics as in Shapes and pixals or String Characters
																					Start of Dev 2/11/2026 | End of dev 5/16/2026


Author: Elijah Spraggins

DonutAPI 1.0:
Full transparency I remade the OneLoneCoder ConsoleGameEngine project, I made it portable and add my own features, like the map generation, math functions, ect. 
I thought it would be a fun project to try and make a libray portable and usable across different platforms. Full Credit to Javidx9 for the inspiration and the source code. 
(Look to Projects I used to look at code). Plans for this project is to make it into a graphics library like Raylib and SDL, it will be two separate library's.

Features DonutAPI 1.0:
-Create a window with a set FPS and title
-Gen and Display a random map or a map with perlin noise gen
-Draws and color basic shapes
-Create, save, and load sprites
-Create, save and load textures
-Create and playes a WAV file
-Key State
-Mouse State

Projects I used:
-Javidx9 : https://github.com/OneLoneCoder/Javidx9/tree/0c8ec20a9ed3b2daf76a925034ac5e7e6f4096e0/ConsoleGameEngine
-Raylib : https://www.raylib.com/cheatsheet/cheatsheet.html

Features planned for graphics library:
-Real Window creation and basic shape drawing
-Audio, Input and 3D rendering
-Language support
-Vector2, Vector3, Vector4, Matrix3, Matrix4 functions and defs

*/

#ifndef DONUTAPI_H
#define DONUTAPI_H

#define DONUTAPI_VERSION "1.0"

#if defined(_WIN32)
	#ifdef DONUTAPI_EXPORTS
		#define DNAPI __declspec(dllexport)    
#elif defined(DONUTAPI_STATIC)
	#define DNAPI                           
#else
	#define DNAPI __declspec(dllimport)    
#endif
	#elif defined(BUILD_LIBTYPE_SHARED)
	#define DNAPI __attribute__((visibility("default")))
#else
	#define DNAPI
#endif

#if defined(_WIN32) && !defined(UNICODE)
	#error "Enable UNICODE for your compiler"
#endif

#include <unordered_map>
#include <iostream>
#include <random>
#include <chrono>

// Platform Includes

#if defined(_WIN32)
	#define _CRT_SECURE_NO_WARNINGS
	#define NOMINMAX
	#include <stdio.h>
	#include <windows.h>
#else
	#include <termios.h>
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <stdio.h>
	#include <fcntl.h>       
	#include <sys/stat.h>
	#include <signal.h>
	#include <cerrno>

// Mock CHAR_INFO for Linux and MacOS
struct CHAR_INFO
{
	union {
		wchar_t UnicodeChar;
		char AsciiChar;
	} Char;
	unsigned short Attributes; // Stores color info
};
#endif

#include "DonutMath.h"

inline const std::string TILE_RESET = "\033[0m";           
inline const std::string TILE_BLACK = "\033[30m";
inline const std::string TILE_DARK_BLUE = "\033[34m";
inline const std::string TILE_DARK_GREEN = "\033[32m";
inline const std::string TILE_DARK_CYAN = "\033[36m";
inline const std::string TILE_DARK_RED = "\033[31m";
inline const std::string TILE_DARK_MAGENTA = "\033[35m";
inline const std::string TILE_BROWN = "\033[33m";
inline const std::string TILE_GRAY = "\033[37m";
inline const std::string TILE_DARK_GRAY = "\033[90m";
inline const std::string TILE_BLUE = "\033[94m";
inline const std::string TILE_GREEN = "\033[92m";
inline const std::string TILE_CYAN = "\033[96m";
inline const std::string TILE_RED = "\033[91m";
inline const std::string TILE_MAGENTA = "\033[95m";
inline const std::string TILE_YELLOW = "\033[93m";
inline const std::string TILE_WHITE = "\033[97m";

// Background Colors | For pixel windowing 
enum COLOR : unsigned short
{
	BLACK = 0x0000,
	DARK_BLUE = 0x0001,
	DARK_GREEN = 0x0002,
	DARK_CYAN = 0x0003,
	DARK_RED = 0x0004,
	DARK_MAGENTA = 0x0005,
	BROWN = 0x0006,
	GRAY = 0x0007,
	DARK_GRAY = 0x0008,
	BLUE = 0x0009,
	GREEN = 0x000A,
	CYAN = 0x000B,
	RED = 0x000C,
	MAGENTA = 0x000D,
	YELLOW = 0x000E,
	WHITE = 0x000F,
};

// The state of the terminals window mode
enum class TerminalMode
{
	Auto,           
	Existing,       
	SpawnNew        
};

// Pixel fullness
enum PIXEL_TYPE
{
	PIXEL_SOLID = 0x2588,
	PIXEL_THREEQUARTERS = 0x2593,
	PIXEL_HALF = 0x2592,
	PIXEL_QUARTER = 0x2591
};

// Key Codes 
enum KEYS
{
	// Use when no key is pressed
	KEY_NULL = 0,

	// F1-F24 | F13-F24 are very uncommon, they are on custom or old keyboards 

	KEY_F1 = 112,
	KEY_F2 = 113,
	KEY_F3 = 114,
	KEY_F4 = 115,
	KEY_F5 = 116,
	KEY_F6 = 117,
	KEY_F7 = 118,
	KEY_F8 = 119,
	KEY_F9 = 120,
	KEY_F10 = 121,
	KEY_F11 = 122,
	KEY_F12 = 123,
	KEY_F13 = 124,
	KEY_F14 = 125,
	KEY_F15 = 126,
	KEY_F16 = 127,
	KEY_F17 = 128,
	KEY_F18 = 129,
	KEY_F19 = 130,
	KEY_F20 = 131,
	KEY_F21 = 132,
	KEY_F22 = 133,
	KEY_F23 = 134,
	KEY_F24 = 135,

	// 1-9

	KEY_0 = 48,
	KEY_1 = 49,
	KEY_2 = 50,
	KEY_3 = 51,
	KEY_4 = 52,
	KEY_5 = 53,
	KEY_6 = 54,
	KEY_7 = 55,
	KEY_8 = 56,
	KEY_9 = 57,

	// Alphabet

	KEY_A = 65,
	KEY_B = 66,
	KEY_C = 67,
	KEY_D = 68,
	KEY_E = 69,
	KEY_F = 70,
	KEY_G = 71,
	KEY_H = 72,
	KEY_I = 73,
	KEY_J = 74,
	KEY_K = 75,
	KEY_L = 76,
	KEY_M = 77,
	KEY_N = 78,
	KEY_O = 79,
	KEY_P = 80,
	KEY_Q = 81,
	KEY_R = 82,
	KEY_S = 83,
	KEY_T = 84,
	KEY_U = 85,
	KEY_V = 86,
	KEY_W = 87,
	KEY_X = 88,
	KEY_Y = 89,
	KEY_Z = 90,

	// Grammar

	KEY_COMMA = 188,
	KEY_PERIOD = 190,
	KEY_SEMICOLON = 186,
	KEY_QUOTE = 222,
	KEY_BACKQUOTE = 192,
	KEY_LEFT_BRACKET = 219,
	KEY_RIGHT_BRACKET = 221,
	KEY_BACKSLASH = 220,

	// Common keys

	KEY_MINUS = 189,
	KEY_EQUAL = 187,
	KEY_LEFT_ALT = 18,
	KEY_RIGHT_ALT = 18,
	KEY_CAPS = 20,
	KEY_LEFT_CTR = 17,
	KEY_RIGHT_CTR = 17,
	KEY_LEFT_SHIFT = 16,
	KEY_RIGHT_SHIFT = 16,
	KEY_ENTER = 13,
	KEY_SPACE = 32,
	KEY_TAB = 9,
	KEY_ESC = 27,
	KEY_DELETE = 46,
	KEY_INSERT = 45,
	KEY_HOME = 36,
	KEY_END = 35,
	KEY_PG_UP = 33,
	KEY_PG_DOWN = 34,
	KEY_UP = 38,
	KEY_DOWN = 40,
	KEY_LEFT = 37,
	KEY_RIGHT = 39,

	// Keypad

	KEY_NUMLOCK = 144,
	KEY_KEYPAD_0 = 96,
	KEY_KEYPAD_1 = 97,
	KEY_KEYPAD_2 = 98,
	KEY_KEYPAD_3 = 99,
	KEY_KEYPAD_4 = 100,
	KEY_KEYPAD_5 = 101,
	KEY_KEYPAD_6 = 102,
	KEY_KEYPAD_7 = 103,
	KEY_KEYPAD_8 = 104,
	KEY_KEYPAD_9 = 105,
	KEY_KEYPAD_ADD = 107,
	KEY_KEYPAD_SUBTRACT = 109,
	KEY_KEYPAD_MULTIPLY = 106,
	KEY_KEYPAD_EQUAL = 12,
	KEY_KEYPAD_COMMA = 194,
	KEY_KEYPAD_DECIMAL = 110,
	KEY_KEYPAD_ENTER = 13,
};

// Key States
struct KeyState
{
	bool k_Pressed;
	bool k_Released;
	bool k_Held;
};

// Mouse States
struct MouseState
{
	int  x = 0;      
	int  y = 0;      

	bool leftHeld = false;
	bool rightHeld = false;
	bool middleHeld = false;
	bool leftPressed = false;
	bool rightPressed = false;
	bool middlePressed = false;
	bool leftReleased = false;
	bool rightReleased = false;
	bool middleReleased = false;
	int  wheelDelta = 0;      
};

// Texture
struct Texture
{
	int width;
	int height;
	unsigned short* pixels;   // flat row-major array: pixels[y * width + x]
};

// Audio
#pragma pack(push, 1)
struct Audio
{
	// RIFF Chunk
	uint32_t riffId;        // "RIFF"  = 0x46464952
	uint32_t riffChunkSize; // file size - 8
	uint32_t waveId;        // "WAVE"  = 0x45564157

	// fmt Chunk
	uint32_t fmtId;         // "fmt "  = 0x20746D66
	uint32_t fmtChunkSize;  // 16 for PCM
	uint16_t formatCode;    // 1 = PCM

	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;      // sampleRate * blockAlign
	uint16_t blockAlign;    // numChannels * bitsPerSample / 8
	uint16_t bitsPerSample;

	// data Chunk
	uint32_t dataId;        // "data"  = 0x61746164

	uint32_t dataChunkSize;

	uint16_t samples[];     // interleaved PCM samples — MUST be last member
};
#pragma pack(pop)

// Music 
struct Music
{
	double hz;
	double beats;
};

// Tile defintion
struct TileDef
{
	char tile;
	int weight;
};

// Logging Functions

DNAPI void InternalLog(const std::string& msg);  // Log info to your compiler/terminal
DNAPI int CheckStatus();                         
DNAPI void DebugLog(const char* message);        // Exported manual log function   

// String Window

DNAPI void ClearScreen();                                                                                                                                                             // Clears the terminal screen and all its content (cls / clear)
DNAPI void Delay(int millisec);                                                                                                                                                       // Sets a delay in milliseconds 
DNAPI void SetMessage(const std::string& msg, int ttl = 180);                                                                                                                         // Sets a message passed in to the terminal
DNAPI std::string GetHudMessage();                                                                                                                                                    // Gets the message passed from SetMessage
DNAPI int GetHudTTL();                                                                                                                                                                // Gets the message TTL
DNAPI void TickHudTTL();                                                                                                                                                              // Gets the message tick

// Map Functions

DNAPI void SetMapVector(const std::vector<std::vector<char>>& userMap);                                                                                                               // Allows user to set their map vector to the generted map vector
DNAPI std::pair<int, int> GetConsoleSize();                                                                                                                                           // Gets how big your screen is in rows and cols
DNAPI std::string TileColor(char tile, const std::unordered_map<char, std::string>& colorMap);                                                                                        // Adds color to character for map tiles 
DNAPI void DisplayMap(int rows, int spritex, int spritey, const std::unordered_map<char, std::string>& tileColors);                                                                   // Display the map and takes in padding, sprite x, sprite y, and color

#ifndef SWIG
	// Standard C++ compilers see this version
	DNAPI std::vector<std::vector<char>> GenerateRandMap(int width, int height, std::vector<TileDef> tiles, unsigned int seed = std::random_device{}());
	DNAPI std::vector<std::vector<char>> GeneratePerlinMap(int width, int height, std::vector<TileDef> tiles, float scale = 0.1f, unsigned int seed = std::random_device{}());
#else
	// SWIG's parser only sees this simplified version
	DNAPI std::vector<std::vector<char>> GenerateRandMap(int width, int height, std::vector<TileDef> tiles, unsigned int seed = 0);	
	DNAPI std::vector<std::vector<char>> GeneratePerlinMap(int width, int height, std::vector<TileDef> tiles, float scale = 0.1f, unsigned int seed = 0);
#endif

// Sprite Functions

DNAPI void CreateSprite(int w, int h);                                      // Create sprite pass in a width and height | Sprite Constructer
DNAPI void DestroySprite();                                                 // Delets sprite | Sprite Deconstructer
DNAPI void Create(int w, int h);                                            // Create sprite pass in a width and height | Function
DNAPI void SetSprite(int x, int y, short p);                                // Set the sprite to the screen
DNAPI void SetColor(int x, int y, unsigned short col);                      // Set the screens color to pixels
DNAPI short GetSprite(int x, int y);                                        // Gets the sprite on screen (for saving and loading)
DNAPI unsigned short GetColor(int x, int y);                                // Gets the color of the screen (for saving and loading)
DNAPI short SampleSprite(float x, float y);                                 // Sets up a sprite sample
DNAPI unsigned short SampleColor(float x, float y);                         // Sets up a color sample
DNAPI bool SaveSprite(std::wstring sFile);                                  // Saves Sprite
DNAPI bool LoadSprite(const std::wstring& sFile);                           // Loads Sprite

// Init Functions   

DNAPI int InitWindow(int width, int height, int fontw, int fonth, TerminalMode mode = TerminalMode::Auto);          // Creates a custom console | Width, Height, Font width, Font Height, Terminal Mode(Only for linux and macOS; so DO NOT TOUCH if on windows pls :) ) | IMPORTANT - WIDTH IS MULTIPYED BY FONT WIDTH AND HEIGHT TO CREATE SCREEN 
DNAPI void DestroyWindow();                                                                                         // Destroys window
DNAPI void SetWindowName(const std::wstring& name);                                                                 // Sets a name for the window
DNAPI void SetFPS(int fps);                                                                                         // Set frame rate | 0 = uncapped 
DNAPI bool WindowShouldClose();                                                                                     // Check if the window is closed                 
DNAPI void GetKeyState();                                                                                           // Get key and mouse input
DNAPI KeyState GetKey(int keycode);                                                                                 // Returns the pressed, held, and released state for a given keycode
DNAPI void FlushKeys();                                                                                             // Clears all key states | call after any blocking action to prevent input bleed
DNAPI MouseState GetMouseState();                                                                                   // Returns full mouse state for this frame (call after GetKeyState)
DNAPI int GetMouseX();                                                                                              // Returns mouse X position
DNAPI int GetMouseY();                                                                                              // Returns mouse Y position
DNAPI std::pair<int, int> GetMousePos();                                                                            // Returns current mouse position as {x, y}
DNAPI void ShowConsoleCursor(bool visible);                                                                         // Show or hide the blinking console text cursor
DNAPI std::string PollInput();                                                                                      // Replaces std::cin >>  | polls key states to read a string, keeping input consistent with the game loop
DNAPI void UpdateScreen();                                                                                          // Updates screen | Call this at the end of all your rendering in your while(!WindowShouldClose())

// Draw Functions

DNAPI void DrawPixel(int x, int y, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                                                   // Draws pixel the screen | Base color is white; pass any color defined
DNAPI void Clip(int& x, int& y);                                                                                                                                                                               // Screen managment | Base color is white; pass any color defined
DNAPI void Fill(int x1, int y1, int x2, int y2, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                                      // Fills the screen | Base color is white; pass any color defined
DNAPI void DrawString(int x, int y, std::wstring c, unsigned short color = WHITE);                                                                                                                             // Draws a wstring at (x, y) | L"Text"
DNAPI void DrawStringAlpha(int x, int y, std::wstring c, unsigned short color = WHITE);                                                                                                                        // Draw a UTF-8 string at (x, y) | L"Text"
DNAPI void DrawSprite(int x, int y);                                                                                                                                                                           // Draws Sprite | Base color is white; pass any color defined
DNAPI void DrawLine(int x1, int y1, int x2, int y2, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                                  // Draws a line | Base color is white; pass any color defined
DNAPI void DrawRectangle(int x, int y, int sidelength, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                               // Draws a rectangle outline | Base color is white; pass any color defined
DNAPI void FillRectangle(int x, int y, int sidelength, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                               // Draws a full colored rectangle | Base color is white; pass any color defined
DNAPI void DrawRotableRectangle(int x, int y, int sidelength, float rotation, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                        // Draws a rotable rectangle outline | Base color is white; pass any color defined
DNAPI void FillRotableRectangle(int x, int y, int sidelength, float rotation, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                        // Draws a full rotable rectangle | Base color is white; pass any color defined
DNAPI void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                              // Draws a triangle outline | Base color is white; pass any color defined
DNAPI void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                              // Draws a full colored triangle | Base color is white; pass any color defined
DNAPI void DrawTriangleStrip(const Vector2* points, int pointCount, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                  // Draws a triangle strip | Base color is white; pass any color defined
DNAPI void FillTriangleStrip(const Vector2* points, int pointCount, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                  // Draws a colored triangle strip | Base color is white; pass any color defined
DNAPI void DrawTriangleFan(const Vector2* points, int pointCount, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                    // Draws a triangle fan | Base color is white; pass any color defined
DNAPI void FillTriangleFan(const Vector2* points, int pointCount, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                    // Draws a colored triangle fan | Base color is white; pass any color defined
DNAPI void DrawCircle(int xc, int yc, int r, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                                         // Draws a circle outline | Base color is white; pass any color defined
DNAPI void FillCircle(int xc, int yc, int r, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                                         // Draws a full colored circle | Base color is white; pass any color defined
DNAPI void DrawCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                            // Draws a circle stip outline | Base color is white; pass any color defined
DNAPI void FillCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                            // Draws a full colored circle strip | Base color is white; pass any color defined
DNAPI void DrawEllipse(int xc, int yc, int a, int b, int angle, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                      // Draws a ellipse | Base color is white; pass any color defined
DNAPI void FillEllipse(int xc, int yc, int a, int b, int angle, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                      // Draws a full colored ellipse | Base color is white; pass any color defined
DNAPI void DrawPoly(const Vector2* vertices, int count, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                              // Draws a polygon outline | Base color is white; pass any color defined
DNAPI void FillPoly(const Vector2* vertices, int count, short pixel = PIXEL_SOLID, unsigned short color = WHITE);                                                                                              // Draws a full polygon | Base color is white; pass any color defined
DNAPI void DrawWireFrameModel(const std::vector<std::pair<float, float>>& vecModelCoordinates, float x, float y, float r = 0.0f, float s = 1.0f, short pixel = PIXEL_SOLID, unsigned short color = WHITE);     // Draws a wire frame at cords | Base color is white; pass any color defined

// Utility Functions

DNAPI float GetElapsedTime();                   // Returns elapsed time (seconds) since last UpdateScreen call | use for frame-rate-independent movement
DNAPI int GetScreenWidth();                     // Returns the screen width set by InitWindow
DNAPI int GetScreenHeight();                    // Returns the screen height set by InitWindow
DNAPI int GetRandomValue(int min, int max);     // Returns a value in between two numbers
DNAPI int SetRandomSeed(unsigned int seed);     // Sets a random seed value above 0

// Texture Functions

DNAPI Texture CreateTexture(int width, int height);                        // Allocate a blank texture (pixels zeroed)
DNAPI void DestroyTexture(Texture& tex);                                   // Free texture memory and zero the struct
DNAPI void SetTexPixel(Texture& tex, int x, int y, unsigned short color);  // Write a single texel
DNAPI unsigned short GetTexPixel(const Texture& tex, int x, int y);        // Read a single texel (clamped to bounds)
DNAPI unsigned short SampleTexture(const Texture& tex, float u, float v);  // Sample with UV in [0,1]; wraps automatically
DNAPI bool SaveTexture(const Texture& tex, const std::wstring& path);      // Save to .tex file (raw header + pixels)
DNAPI bool LoadTexture(const std::wstring& path, Texture& outTex);         // Load from .tex file into outTex

// Audio Functions

DNAPI void CreateMusicFile(const char* filename, double bpm, double beat, int sr, const Music* notes, int noteCount, float volume = 1.0f);          // Plays a WAV file on a dedicated background-music thread (non-blocking)
DNAPI void PlayMusicFile(const char* filename, void** data, uint32_t* numBytesRead);                                                                // Stops background music and waits for the playback thread to exit
DNAPI void StopMusic();                                                                                                                             // Returns true if a background music track is currently playing
DNAPI bool IsMusicPlaying();                                                                                                                        // Fire and forget SFX | plays on its own thread, never interrupts music or other SFX | Use this for clicks, UI sounds, and any short effect that can overlap.
DNAPI void PlaySFX(const char* filename, float volume = 1.0f);

// Sprite Billboard Rendering 

DNAPI void DrawBillboard // DrawBillboard renders a camera-facing textured sprite into the current frame; It must be called AFTER the wall-rendering pass so the Z-buffer is populated
(
	const double* zBuffer,
	double worldX, double worldY,
	double dirX, double dirY,
	double planeX, double planeY,
	double posX, double posY,
	const Texture& sprite,
	float scale = 1.0f
);

#endif // DONUTAPI_H