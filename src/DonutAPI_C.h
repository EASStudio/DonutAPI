/*

This is our C wrapper for C, Swift, Zig and Rust languge bindings
Include this file if using C, Swift, Zig, and Rust

*/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DONUTAPI_C_VERSION "1.0"

#if defined(_WIN32)
	#ifdef DONUTAPI_C_EXPORTS
		#define DN_API __declspec(dllexport)  
#elif defined(DONUTMATH_STATIC)
	#define DN_API                         
#else
	#define DN_API __declspec(dllimport)   
#endif
	#elif defined(BUILD_LIBTYPE_SHARED)
		#define DN_API __attribute__((visibility("default")))
#else
	#define DN_API
#endif

	typedef enum
	{
		DN_BLACK = 0x0000,
		DN_DARK_BLUE = 0x0001,
		DN_DARK_GREEN = 0x0002,
		DN_DARK_CYAN = 0x0003,
		DN_DARK_RED = 0x0004,
		DN_DARK_MAGENTA = 0x0005,
		DN_BROWN = 0x0006,
		DN_GREY = 0x0007,
		DN_DARK_GREY = 0x0008,
		DN_BLUE = 0x0009,
		DN_GREEN = 0x000A,
		DN_CYAN = 0x000B,
		DN_RED = 0x000C,
		DN_MAGENTA = 0x000D,
		DN_YELLOW = 0x000E,
		DN_WHITE = 0x000F
	} DN_COLOR;

	typedef enum
	{
		DN_PIXEL_SOLID = 0x2588,
		DN_PIXEL_THREEQUARTERS = 0x2593,
		DN_PIXEL_HALF = 0x2592,
		DN_PIXEL_QUARTER = 0x2591
	} DN_PIXEL_TYPE;

	enum class DN_TerminalMode
	{
		DN_Auto,           // Try existing first, fallback to spawn
		DN_Existing,       // Force use current terminal
		DN_SpawnNew        // Old behavior (xterm etc.)
	};

	// Keys Codes
	
	// Use when no key is pressed
	#define DN_KEY_NULL 0

	// F1-F24 | F13-F24 are very uncommon, they are on custom or old keyboards 

	#define DN_KEY_F1 112
	#define DN_KEY_F2 113
	#define DN_KEY_F3 114
	#define DN_KEY_F4 115
	#define DN_KEY_F5 116
	#define DN_KEY_F6 117
	#define DN_KEY_F7 118
	#define DN_KEY_F8 119
	#define DN_KEY_F9 120
	#define DN_KEY_F10 121
	#define DN_KEY_F11 122
	#define DN_KEY_F12 123
	#define DN_KEY_F13 124
	#define DN_KEY_F14 125
	#define DN_KEY_F15 126
	#define DN_KEY_F16 127
	#define DN_KEY_F17 128
	#define DN_KEY_F18 129
	#define DN_KEY_F19 130
	#define DN_KEY_F20 131
	#define DN_KEY_F21 132
	#define DN_KEY_F22 133
	#define DN_KEY_F23 134
	#define DN_KEY_F24 135

	// 1-9

	#define DN_KEY_0 48
	#define DN_KEY_1 49
	#define DN_KEY_2 50
	#define DN_KEY_3 51
	#define DN_KEY_4 52
	#define DN_KEY_5 53
	#define DN_KEY_6 54
	#define DN_KEY_7 55
	#define DN_KEY_8 56
	#define DN_KEY_9 57

	// Alphabet

	#define DN_KEY_A 65
	#define DN_KEY_B 66
	#define DN_KEY_C 67
	#define DN_KEY_D 68
	#define DN_KEY_E 69
	#define DN_KEY_F 70
	#define DN_KEY_G 71
	#define DN_KEY_H 72
	#define DN_KEY_I 73
	#define DN_KEY_J 74
	#define DN_KEY_K 75
	#define DN_KEY_L 76
	#define DN_KEY_M 77
	#define DN_KEY_N 78
	#define DN_KEY_O 79
	#define DN_KEY_P 80
	#define DN_KEY_Q 81
	#define DN_KEY_R 82
	#define DN_KEY_S 83
	#define DN_KEY_T 84
	#define DN_KEY_U 85
	#define DN_KEY_V 86
	#define DN_KEY_W 87
	#define DN_KEY_X 88
	#define DN_KEY_Y 89
	#define DN_KEY_Z 90

	// Grammar

	#define DN_KEY_COMMA 188
	#define DN_KEY_PERIOD 190
	#define DN_KEY_SEMICOLON 186
	#define DN_KEY_QUOTE 222
	#define DN_KEY_BACKQUOTE 192
	#define DN_KEY_LEFT_BRACKET 219
	#define DN_KEY_RIGHT_BRACKET 221
	#define DN_KEY_BACKSLASH 220

	// Common keys

	#define DN_KEY_MINUS 189
	#define DN_KEY_EQUAL 187
	#define DN_KEY_LEFT_ALT 18
	#define DN_KEY_RIGHT_ALT 18
	#define DN_KEY_CAPS 20
	#define DN_KEY_LEFT_CTR 17
	#define DN_KEY_RIGHT_CTR 17
	#define DN_KEY_LEFT_SHIFT 16
	#define DN_KEY_RIGHT_SHIFT 16
	#define DN_KEY_ENTER 13
	#define DN_KEY_SPACE 32
	#define DN_KEY_TAB 9
	#define DN_KEY_ESC 27
	#define DN_KEY_DELETE 46
	#define DN_KEY_INSERT 45
	#define DN_KEY_HOME 36
	#define DN_KEY_END 35
	#define DN_KEY_PG_UP 33
	#define DN_KEY_PG_DOWN 34
	#define DN_KEY_UP 38
	#define DN_KEY_DOWN 40
	#define DN_KEY_LEFT 37
	#define DN_KEY_RIGHT 39

	// Keypad

	#define DN_KEY_NUMLOCK 144
	#define DN_KEY_KEYPAD_0 96
	#define DN_KEY_KEYPAD_1 97
	#define DN_KEY_KEYPAD_2 98
	#define DN_KEY_KEYPAD_3 99
	#define DN_KEY_KEYPAD_4 100
	#define DN_KEY_KEYPAD_5 101
	#define DN_KEY_KEYPAD_6 102
	#define DN_KEY_KEYPAD_7 103
	#define DN_KEY_KEYPAD_8 104
	#define DN_KEY_KEYPAD_9 105
	#define DN_KEY_KEYPAD_ADD 107
	#define DN_KEY_KEYPAD_SUBTRACT 109
	#define DN_KEY_KEYPAD_MULTIPLY 106
	#define DN_KEY_KEYPAD_EQUAL 12
	#define DN_KEY_KEYPAD_COMMA 194
	#define DN_KEY_KEYPAD_DECIMAL 110
	#define DN_KEY_KEYPAD_ENTER 13

	typedef struct 
	{
		bool k_Pressed;
		bool k_Released;
		bool k_Held;
	} DN_KeyState;

	typedef struct 
	{
    int  x;      // character-cell column
	int  y;      // character-cell row
	bool leftHeld;
	bool rightHeld;
	bool middleHeld;
	bool leftPressed;
	bool rightPressed;
	bool middlePressed;
	bool leftReleased;
	bool rightReleased;
	bool middleReleased;
	int  wheelDelta;      // +1 scroll up | -1 scroll down | 0 no scroll
	} DN_MouseState;

	typedef struct 
	{
		int width;
		int height;
		unsigned short* pixels;   // flat row-major array: pixels[y * width + x]
	} DN_Texture;

	typedef struct 
	{ 
		double hz;
		double beats;
	} DN_Music;

	typedef struct 
	{
		char tile; 
		int weight; 
	} DN_TileDef;
	
	typedef struct 
	{ 
		int first; 
	    int second; 
	} DN_IntPair;

	typedef struct 
	{
		float x;
		float y;
	} DN_Vector2;

	// Logging

	DN_API void DN_InternalLog(const char* msg);
	DN_API int  DN_CheckStatus(void);
	DN_API void DN_DebugLog(const char* message);

	// String Window

	DN_API void DN_ClearScreen(void);
	DN_API void DN_Delay(int millisec);
	DN_API void DN_SetMessage(const char* msg, int ttl);
	DN_API const char* DN_GetHudMessage(void);                                                                                         // static buffer
	DN_API int DN_GetHudTTL(void);
	DN_API void DN_TickHudTTL(void);

	// Map Functions

	DN_API void DN_SetMapVector(const char* flatMap, int width, int height);                                                           // Copy a flat (row-major) char map into the library's internal map vector
	DN_API DN_IntPair DN_GetConsoleSize(void);                                                                                         // Returns the current console / terminal size in columns and rows
	DN_API char* DN_GenerateRandMap(int width, int height, const DN_TileDef* tiles, int tileCount, unsigned int seed);                 // Generate a random map; Returns a heap-allocated flat char array [height*width]; The caller MUST call DN_FreeMap() when finished
	DN_API char* DN_GeneratePerlinMap(int width, int height, const DN_TileDef* tiles, int tileCount, float scale, unsigned int seed);  // Generate a perlin noise map; Returns a heap-allocated flat char array [height*width]; The caller MUST call DN_FreeMap() when finished
	DN_API void DN_FreeMap(char* ptr);                                                                                                 // Free a map array returned by DN_GenerateRandMap or DN_GeneratePerlinMap
	DN_API const char* DN_TileColor(char tile, const char* tileKeys, const char** tileColors, int count);                              // Return the ANSI color escape for a tile character; tileKeys[i] = char key tileColors[i] = ANSI escape string for that keyReturns a pointer into a static buffer
	DN_API void DN_DisplayMap(int rows, int spritex, int spritey, const char* tileKeys, const char** tileColors, int count);           // Display the current map in the terminal.tileKeys / tileColors work the same way as in DN_TileColor

	// Sprite Functions

	DN_API void DN_CreateSprite(int w, int h);
	DN_API void DN_DestroySprite(void);
	DN_API void DN_SetSprite(int x, int y, short pixel);
	DN_API void DN_SetColor(int x, int y, unsigned short color);
	DN_API short DN_GetSprite(int x, int y);
	DN_API unsigned short DN_GetColor(int x, int y);
	DN_API short DN_SampleSprite(float x, float y);
	DN_API unsigned short DN_SampleColor(float x, float y);
	DN_API bool DN_SaveSprite(const char* path);  // Save sprite to file; Path is a UTF-8 string
	DN_API bool DN_LoadSprite(const char* path);  // Load sprite from file; Path is a UTF-8 string

	// Init Functions
	DN_API int  DN_InitWindow(int width, int height, int fontw, int fonth, DN_TerminalMode mode = DN_TerminalMode::DN_Auto);
	DN_API void DN_DestroyWindow(void);
	DN_API void DN_SetWindowName(const char* name);  // Set the window title; Name is a UTF-8 string
	DN_API void DN_SetFPS(int fps);
	DN_API bool DN_WindowShouldClose(void);
	DN_API void DN_PollInputState(void);             // Poll keyboard / mouse state; Call once per frame before DN_GetKey
	DN_API DN_KeyState DN_GetKey(int keycode);
	DN_API void DN_FlushKeys(void);
	DN_API DN_MouseState DN_GetMouseState(void);
	DN_API int DN_GetMouseX(void);                      // Returns mouse X position
	DN_API int DN_GetMouseY(void);                      // Returns mouse Y position
	DN_API void DN_ShowConsoleCursor(bool visible);
	DN_API const char* DN_PollInput(void);           // Blocking text-input read (replacement for std::cin); Returns a pointer into a static buffer
	DN_API void DN_UpdateScreen(void);

	// Draw Functions

	DN_API void DN_DrawPixel(int x, int y, short pixel, unsigned short color);
	DN_API void DN_Clip(int* x, int* y);
	DN_API void DN_Fill(int x1, int y1, int x2, int y2, short pixel, unsigned short color);
	DN_API void DN_DrawString(int x, int y, const char* s, unsigned short color);            // Draw a UTF-8 string at (x, y)
	DN_API void DN_DrawStringAlpha(int x, int y, const char* s, unsigned short color);
	DN_API void DN_DrawSprite(int x, int y);
	DN_API void DN_DrawLine(int x1, int y1, int x2, int y2, short pixel, unsigned short color);
	DN_API void DN_DrawRectangle(int x, int y, int side, short pixel, unsigned short color);
	DN_API void DN_FillRectangle(int x, int y, int side, short pixel, unsigned short color);
	DN_API void DN_DrawRotableRectangle(int x, int y, int side, float rotation, short pixel, unsigned short color);
	DN_API void DN_FillRotableRectangle(int x, int y, int side, float rotation, short pixel, unsigned short color);
	DN_API void DN_DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short pixel, unsigned short color);
	DN_API void DN_FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short pixel, unsigned short color);
	DN_API void DN_DrawTriangleStrip(const DN_Vector2* points, int pointCount, short pixel, unsigned short color);
	DN_API void DN_FIllTriangleStrip(const DN_Vector2* points, int pointCount, short pixel, unsigned short color);
	DN_API void DN_DrawTriangleFan(const DN_Vector2* points, int pointCount, short pixel, unsigned short color);
	DN_API void DN_FillTriangleFan(const DN_Vector2* points, int pointCount, short pixel, unsigned short color);
	DN_API void DN_DrawCircle(int xc, int yc, int r, short pixel, unsigned short color);
	DN_API void DN_FillCircle(int xc, int yc, int r, short pixel, unsigned short color);
	DN_API void DN_DrawCircleSector(DN_Vector2 center, float radius, float startAngle, float endAngle, int segments, short pixel, unsigned short color);
	DN_API void DN_FillCircleSector(DN_Vector2 center, float radius, float startAngle, float endAngle, int segments, short pixel, unsigned short color);
	DN_API void DN_DrawEllipse(int xc, int yc, int a, int b, int angle, short pixel, unsigned short color);
	DN_API void DN_FillEllipse(int xc, int yc, int a, int b, int angle, short pixel, unsigned short color);
	DN_API void DN_DrawWireFrameModel(const float* coords, int count, float x, float y, float r, float s, unsigned short color, short pixel); 

	// Utility
	DN_API float DN_GetElapsedTime(void);
	DN_API int DN_GetScreenWidth(void);
	DN_API int DN_GetScreenHeight(void);
	DN_API int DN_GetRandomValue(int min, int max);
	DN_API int DN_SetRandomSeed(unsigned int seed);
	DN_API DN_IntPair DN_GetMousePos(void);

	// Texture 
	DN_API DN_Texture DN_CreateTexture(int width, int height);
	DN_API void DN_DestroyTexture(DN_Texture* tex);
	DN_API void DN_SetTexPixel(DN_Texture* tex, int x, int y, unsigned short color);
	DN_API unsigned short DN_GetTexPixel(const DN_Texture* tex, int x, int y);
	DN_API unsigned short DN_SampleTexture(const DN_Texture* tex, float u, float v);
	DN_API bool DN_SaveTexture(const DN_Texture* tex, const char* path); 
	DN_API bool DN_LoadTexture(const char* path, DN_Texture* outTex);    

	// Audio 
	DN_API void DN_CreateMusicFile(const char* filename, double bpm, double beat, int sr, const DN_Music* notes, int noteCount, float volume); 
	DN_API void DN_PlayMusicFile(const char* filename); 
	DN_API void DN_StopMusic(void);
	DN_API bool DN_IsMusicPlaying(void);
	DN_API void DN_PlaySFX(const char* filename, float volume);

	// Sprite billboard rendering 
	DN_API void DN_DrawBillboard(const double* zBuffer, double worldX, double worldY, double dirX, double dirY, double planeX, double planeY, double posX, double posY, const DN_Texture* sprite, float scale); 

#ifdef __cplusplus
}
#endif
