#include "DonutAPI.h"

// Includes
#include <condition_variable>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <cassert>
#include <thread>
#include <mutex>
#include <vector>
#include <cstring>
#include <sstream>
#include <atomic>   

// Audio includes
#if defined(_WIN32)
	#include <audioclient.h>
	#include <mmdeviceapi.h>
#elif defined(__APPLE__)
	#include <AudioToolbox/AudioToolbox.h>
#elif defined(__linux__)
	#include <alsa/asoundlib.h>
	#include <pty.h>       // openpty — links via -lutil
	#include <poll.h>      // poll / POLLHUP — reliable PTY hangup detection
#else
	#error "Your operating system is not supported currently"
#endif

// Window functions
struct DONUTAPI
{
	int screenWidth;
	int screenHeight;
	int spriteWidth;
	int spriteHeight;

	short* s_Sprites = nullptr;
	unsigned short* s_Colors = nullptr;

	std::wstring appName = L"App"; // Default name
	CHAR_INFO* screenBuffer = nullptr;

	std::atomic<bool> gameActive{ true };
	static std::condition_variable gameFinished;
	bool consoleInFocus = true;
	float elapsedTime = 0.0f;

	// Input Function varibles
	KeyState k_keys[256];
	KeyState m_mouse[5];
	short k_keyOldState[256] = { 0 };
	short k_keyNewState[256] = { 0 };
	bool k_mouseOldState[5] = { 0 };
	bool k_mouseNewState[5] = { 0 };

	int m_mousePosX = 0;
	int m_mousePosY = 0;
	bool m_mouseButtons[3] = {};
	bool m_mousePressed[3] = {};
	bool m_mouseReleased[3] = {};
	int m_mouseWheelDelta = 0;

	~DONUTAPI()
	{
		delete[] s_Sprites;
		delete[] s_Colors;
		delete[] screenBuffer;
	}

#if defined(_WIN32)
	HANDLE originalConsole;
	HANDLE d_Console = INVALID_HANDLE_VALUE;
	CONSOLE_SCREEN_BUFFER_INFO originalConsoleInfo;
#else
	bool useExistingTerminal = false;
	bool rawModeEnabled = false;
	struct termios originalTermios;
	struct winsize originalWindow;
	int graphicsFd = -1;       // write graphics here: on Linux PTY also read keyboard here
	int inputFd = -1;          // macOS only: read keyboard from the input FIFO here
	std::string fifoPath;
	std::string inputFifoPath; // macOS only: path of the keyboard-input FIFO
#endif
};

// Core
static DONUTAPI* core = nullptr;
static std::atomic<bool>* g_gameActive = nullptr;
static float g_targetFrameTime = 0.0f;
static std::vector<std::vector<char>> map;
static std::string g_hudMessage;
static int g_hudTTL;

// Audio
static std::atomic<bool> g_musicStop{ false };
static std::atomic<bool> g_musicPlaying{ false };
static std::thread g_musicThread;

// SFX
static std::mutex g_sfxMutex;
static std::vector<std::thread> g_sfxThreads;

DNAPI void InternalLog(const std::string& msg)
{
	std::string formatted = "[DonutAPI Debug] " + msg + "\n";
#if defined(_WIN32)
	OutputDebugStringA(formatted.c_str());
#else
	fprintf(stderr, "%s", formatted.c_str());
#endif
}

// Loading and Unloading library logging
#if defined(_WIN32)

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	if (dwCtrlType == CTRL_CLOSE_EVENT ||
		dwCtrlType == CTRL_C_EVENT ||
		dwCtrlType == CTRL_BREAK_EVENT)
	{
		if (g_gameActive) *g_gameActive = false;
		Delay(500);
		return TRUE;
	}
	return FALSE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InternalLog("DLL Loaded into process.");
		break;
	case DLL_PROCESS_DETACH:
		InternalLog("DLL Unloaded from process.");
		break;
	}
	return TRUE;
}

#else
#include <syslog.h>

__attribute__((constructor))
static void on_load()
{
	openlog(nullptr, LOG_PID, LOG_USER);
	syslog(LOG_INFO, "Library loaded into process (PID: %d).", getpid());
}

__attribute__((destructor))
static void on_unload()
{
	syslog(LOG_INFO, "Library unloaded from process (PID: %d).", getpid());
	closelog();
}

#endif

// Check if library is alive
DNAPI int CheckStatus()
{
	InternalLog("Status check requested.");
	return 1;
}

// Exported manual log function                  
DNAPI void DebugLog(const char* message)
{
	if (message)
	{
		InternalLog(message);
	}
}

// Core
DNAPI void ClearScreen()
{
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coordScreen = { 0, 0 };
	DWORD charsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(hConsole, &csbi))
	{
		DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
		FillConsoleOutputCharacter(hConsole, ' ', cellCount, coordScreen, &charsWritten);
		FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, coordScreen, &charsWritten);
	}
	SetConsoleCursorPosition(hConsole, coordScreen);
#else
	const char* cls = "\033[2J\033[H";
	if (core && core->graphicsFd >= 0)
		write(core->graphicsFd, cls, 7);
	else
		fwrite(cls, 1, 7, stdout), fflush(stdout);
#endif
}

DNAPI void SetMessage(const std::string& msg, int ttl)
{
	g_hudMessage = msg;
	g_hudTTL = ttl;
}

DNAPI std::string GetHudMessage()
{
	return g_hudMessage;
}

DNAPI int GetHudTTL()
{
	return g_hudTTL;
}

DNAPI void TickHudTTL()
{
	if (g_hudTTL > 0) g_hudTTL--;
}

DNAPI void Delay(int millisec)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(millisec));
}

DNAPI void SetMapVector(const std::vector<std::vector<char>>& userMap)
{
	map = userMap;
}

DNAPI std::pair<int, int> GetConsoleSize()
{
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (hConsole == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(hConsole, &csbi))
	{
		return { 80, 25 };
	}

	const int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	const int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	return { cols, rows };
#else 
	struct winsize w;
	ioctl(fileno(stdout), TIOCGWINSZ, &w);

	const int cols = (int)(w.ws_col);
	const int rows = (int)(w.ws_row);
	return { cols, rows };
#endif
}

// Generate a random map based on characters given from tile struct
std::vector<std::vector<char>> GenerateRandMap(int width, int height, std::vector<TileDef> tiles, unsigned int seed)
{
	std::mt19937 rng(seed);

	std::vector<char> pool;
	for (const auto& t : tiles)
		for (int i = 0; i < t.weight; i++)
			pool.push_back(t.tile);

	std::uniform_int_distribution<int> dist(0, (int)pool.size() - 1);

	std::vector<std::vector<char>> map(height, std::vector<char>(width));
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			map[y][x] = pool[dist(rng)];

	return map;
}

// Generate a perlin noise based map with characters given from tile struct
DNAPI std::vector<std::vector<char>> GeneratePerlinMap(int width, int height, std::vector<TileDef> tiles, float scale, unsigned int seed)
{
	std::mt19937 rng(seed);
	std::vector<int> p(256);
	std::iota(p.begin(), p.end(), 0);
	std::shuffle(p.begin(), p.end(), rng);
	p.insert(p.end(), p.begin(), p.end());
	int perm[512];
	for (int i = 0; i < 512; i++) perm[i] = p[i];

	map.assign(height, std::vector<char>(width));

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			float nx = x * scale;
			float ny = y * scale;
			float val = (Perlin(nx, ny, perm) + 1.0f) / 2.0f;

			float cursor = 0.0f;
			char chosen = tiles.back().tile;
			for (auto& t : tiles)
			{
				cursor += t.weight / 100.0f;
				if (val < cursor) { chosen = t.tile; break; }
			}
			map[y][x] = chosen;
		}
	}
	return map;
}

DNAPI std::string TileColor(char tile, const std::unordered_map<char, std::string>& colorMap)
{
	auto it = colorMap.find(tile);
	if (it != colorMap.end())
	{
		return it->second;
	}
	return "";
}

DNAPI void DisplayMap(int rows, int spritex, int spritey, const std::unordered_map<char, std::string>& tileColors)
{
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole == INVALID_HANDLE_VALUE) return;

	auto consoleSize = GetConsoleSize();
	int consoleW = consoleSize.first;
	int consoleH = consoleSize.second;

	const int reservedRows = rows;
	int viewH = std::max(reservedRows, consoleH - reservedRows);
	int viewW = consoleW;

	if (map.empty()) return;

	int mapH = static_cast<int>(map.size());
	int mapW = static_cast<int>(map[0].size());

	viewH = std::min(viewH, mapH);
	viewW = std::min(viewW, mapW);

	int sx = spritex;
	int sy = spritey;

	int top = std::clamp(sy - viewH / 2, 0, mapH - viewH);
	int left = std::clamp(sx - viewW / 2, 0, mapW - viewW);

	std::ostringstream out;

	for (int y = top; y < top + viewH; ++y)
	{
		for (int x = left; x < left + viewW; ++x)
		{
			if (x == sx && y == sy)
			{
				out << TILE_RED << "@" << TILE_RESET;
			}
			else
			{
				char tile = map[y][x];
				out << TileColor(tile, tileColors) << tile;
			}
		}
		out << TILE_RESET << '\n';
	}

	SetConsoleCursorPosition(hConsole, { 0, 0 });
	std::cout << out.str() << std::flush;
#else 
	struct winsize w;
	ioctl(fileno(stdout), TIOCGWINSZ, &w);

	int consoleH = (int)(w.ws_row);
	int consoleW = (int)(w.ws_col);

	const int reservedRows = rows;
	int viewH = std::max(reservedRows, consoleH - reservedRows);
	int viewW = consoleW;

	if (map.empty()) return;

	int mapH = static_cast<int>(map.size());
	int mapW = static_cast<int>(map[0].size());

	viewH = std::min(viewH, mapH);
	viewW = std::min(viewW, mapW);

	int sx = spritex;
	int sy = spritey;

	int maxTop = std::max(0, mapH - viewH);
	int maxLeft = std::max(0, mapW - viewW);

	int top = std::clamp(sy - viewH / 2, 0, maxTop);
	int left = std::clamp(sx - viewW / 2, 0, maxLeft);

	std::ostringstream out;

	for (int y = top; y < top + viewH; ++y)
	{
		for (int x = left; x < left + viewW; ++x)
		{
			if (x == sx && y == sy)
			{
				out << TILE_RED << '@' << TILE_RESET;
			}
			else
			{
				char tile = map[y][x];
				out << TileColor(tile, tileColors) << tile << TILE_RESET;
			}
		}
		out << '\n';
	}

	std::cout << "\033[H" << out.str() << std::flush;
#endif
}

DNAPI void CreateSprite(int w, int h)
{
	Create(w, h);
}

DNAPI void DestroySprite()
{
	if (core != nullptr)
	{
		delete[] core->s_Sprites;
		core->s_Sprites = nullptr;
		delete[] core->s_Colors;
		core->s_Colors = nullptr;
		core->spriteWidth = 0;
		core->spriteHeight = 0;
	}
}

DNAPI void Create(int w, int h)
{
	core->spriteWidth = w;
	core->spriteHeight = h;
	core->s_Sprites = new short[w * h];
	core->s_Colors = new unsigned short[w * h];

	for (int i = 0; i < w * h; i++)
	{
		core->s_Sprites[i] = L' ';
		core->s_Colors[i] = BLACK;
	}
}

DNAPI void SetSprite(int x, int y, short p)
{
	if (x >= 0 && x < core->spriteWidth && y >= 0 && y < core->spriteHeight)
		core->s_Sprites[y * core->spriteWidth + x] = p;
}

DNAPI void SetColor(int x, int y, unsigned short color)
{
	if (x >= 0 && x < core->spriteWidth && y >= 0 && y < core->spriteHeight)
		core->s_Colors[y * core->spriteWidth + x] = color;
}

DNAPI short GetSprite(int x, int y)
{
	if (x < 0 || x >= core->spriteWidth || y < 0 || y >= core->spriteHeight)
		return L' ';
	else
		return core->s_Sprites[y * core->spriteWidth + x];
}

DNAPI unsigned short GetColor(int x, int y)
{
	if (x < 0 || x >= core->spriteWidth || y < 0 || y >= core->spriteHeight)
		return WHITE;
	else
		return core->s_Colors[y * core->spriteWidth + x];
}

DNAPI short SampleSprite(float x, float y)
{
	int sx = (int)(x * (float)core->spriteWidth);	
	int sy = (int)(y * (float)core->spriteHeight);
	if (sx < 0 || sx >= core->spriteWidth || sy < 0 || sy >= core->spriteHeight)
		return L' ';
	else
		return core->s_Sprites[sy * core->spriteWidth + sx];
}

DNAPI unsigned short SampleColor(float x, float y)
{
	int sx = (int)(x * (float)core->spriteWidth);
	int sy = (int)(y * (float)core->spriteHeight);
	if (sx < 0 || sx >= core->spriteWidth || sy < 0 || sy >= core->spriteHeight)
		return WHITE;
	else
		return core->s_Colors[sy * core->spriteWidth + sx];
}

DNAPI bool SaveSprite(std::wstring sFile)
{
	FILE* f = nullptr;
#if defined(_WIN32)
	_wfopen_s(&f, sFile.c_str(), L"wb");
#else
	f = fopen(std::string(sFile.begin(), sFile.end()).c_str(), "wb");
#endif
	if (f == nullptr)
		return false;

	fwrite(&core->spriteWidth, sizeof(int), 1, f);
	fwrite(&core->spriteHeight, sizeof(int), 1, f);
	fwrite(core->s_Colors, sizeof(short), core->spriteWidth * core->spriteHeight, f);
	fwrite(core->s_Sprites, sizeof(short), core->spriteWidth * core->spriteHeight, f);

	fclose(f);

	return true;
}

DNAPI bool LoadSprite(const std::wstring& sFile)
{
	FILE* f = nullptr;

#if defined(_WIN32)
	_wfopen_s(&f, sFile.c_str(), L"rb");
#else
	f = fopen(std::string(sFile.begin(), sFile.end()).c_str(), "rb");
#endif

	if (f == nullptr)
		return false;

	int newWidth = 0, newHeight = 0;
	if (std::fread(&newWidth, sizeof(int), 1, f) != 1 ||
		std::fread(&newHeight, sizeof(int), 1, f) != 1 ||
		newWidth <= 0 || newHeight <= 0)
	{
		std::fclose(f);
		return false;
	}

	size_t count = static_cast<size_t>(newWidth) * newHeight;
	unsigned short* newColors = new unsigned short[count];
	short* newSprites = new short[count];

	if (std::fread(newColors, sizeof(unsigned short), count, f) != count || std::fread(newSprites, sizeof(short), count, f) != count)
	{
		delete[] newColors;
		delete[] newSprites;
		std::fclose(f);
		return false;
	}

	std::fclose(f);

	delete[] core->s_Colors;
	delete[] core->s_Sprites;
	core->spriteWidth = newWidth;
	core->spriteHeight = newHeight;
	core->s_Colors = newColors;
	core->s_Sprites = newSprites;

	return true;
}


// Pixel Drawing functions and KeyStates


// Unix helpers 

#if !defined(_WIN32)

static void UnixShutdownSignalHandler(int /*sig*/)
{
	if (g_gameActive) *g_gameActive = false;
}

static void ResizeScreenBuffer(int newW, int newH)
{
	if (!core) return;
	if (newW <= 0 || newH <= 0) return;
	if (newW == core->screenWidth && newH == core->screenHeight) return;

	core->screenWidth = newW;
	core->screenHeight = newH;

	delete[] core->screenBuffer;
	core->screenBuffer = new CHAR_INFO[newW * newH]();
	std::memset(core->screenBuffer, 0, sizeof(CHAR_INFO) * newW * newH);

	if (core->graphicsFd >= 0)
	{
		const char* clear = "\033[2J\033[H";
		write(core->graphicsFd, clear, strlen(clear));
	}

	InternalLog("ResizeScreenBuffer: " + std::to_string(newW) + "x" + std::to_string(newH));
}

static std::chrono::steady_clock::time_point lastResize = {};

static void ResizeSignalHandler(int sig)
{
	if (!core) return;

	auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastResize).count() < 100)
		return;

	lastResize = now;

	struct winsize ws = {};
	int fd = (core->graphicsFd >= 0) ? core->graphicsFd : STDOUT_FILENO;

	if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0)
	{
		ResizeScreenBuffer(ws.ws_col, ws.ws_row);
	}
}

#endif // !_WIN32

DNAPI int InitWindow(int width, int height, int fontw, int fonth, TerminalMode mode)
{
	if (core == nullptr) core = new DONUTAPI();
	core->gameActive = true;
	g_gameActive = &core->gameActive;

	core->screenWidth = width;
	core->screenHeight = height;

	if (core->screenBuffer != nullptr) delete[] core->screenBuffer;
	core->screenBuffer = new CHAR_INFO[core->screenWidth * core->screenHeight];
	std::memset(core->screenBuffer, 0, sizeof(CHAR_INFO) * core->screenWidth * core->screenHeight);

#if !defined(_WIN32)
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	struct sigaction sa_unix = {};
	sa_unix.sa_handler = UnixShutdownSignalHandler;
	sigemptyset(&sa_unix.sa_mask);
	sa_unix.sa_flags = SA_RESTART;
	sigaction(SIGTERM, &sa_unix, nullptr);
#endif

#if defined(_WIN32)
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	core->originalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (core->originalConsole == INVALID_HANDLE_VALUE)
	{
		InternalLog("FAILED: GetStdHandle"); return false;
	}

	if (!GetConsoleScreenBufferInfo(core->originalConsole, &core->originalConsoleInfo))
	{
		InternalLog("FAILED: GetConsoleScreenBufferInfo (original)"); return false;
	}

	core->d_Console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	if (core->d_Console == INVALID_HANDLE_VALUE)
	{
		InternalLog("FAILED: CreateConsoleScreenBuffer"); return false;
	}

	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	SetConsoleMode(hInput, ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);

	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Consolas");

	int actualFontW = fontw;
	int actualFontH = fonth;
	bool fits = false;

	while (actualFontH >= 2)
	{
		cfi.dwFontSize.X = (short)actualFontW;
		cfi.dwFontSize.Y = (short)actualFontH;

		if (!SetCurrentConsoleFontEx(core->d_Console, FALSE, &cfi))
		{
			InternalLog("FAILED: SetCurrentConsoleFontEx"); return false;
		}

		COORD maxSize = GetLargestConsoleWindowSize(core->d_Console);

		InternalLog("Font " + std::to_string(actualFontW) + "x" + std::to_string(actualFontH) +
			" Max " + std::to_string(maxSize.X) + "x" + std::to_string(maxSize.Y) +
			" Requested " + std::to_string(width) + "x" + std::to_string(height));

		if (width <= maxSize.X && height <= maxSize.Y)
		{
			fits = true;
			break;
		}

		actualFontH--;
		actualFontW = std::max(2, actualFontW - 1);
	}

	if (!fits)
	{
		InternalLog("FAILED: Cannot fit " + std::to_string(width) + "x" +
			std::to_string(height) + " even at minimum font size.");
		return false;
	}

	if (actualFontW != fontw || actualFontH != fonth)
	{
		InternalLog("Font auto-scaled from " + std::to_string(fontw) + "x" + std::to_string(fonth) +
			" to " + std::to_string(actualFontW) + "x" + std::to_string(actualFontH) +
			" to fit " + std::to_string(width) + "x" + std::to_string(height));
	}

	SMALL_RECT minRect = { 0, 0, 1, 1 };
	if (!SetConsoleWindowInfo(core->d_Console, TRUE, &minRect))
	{
		InternalLog("FAILED: SetConsoleWindowInfo (shrink)"); return false;
	}

	COORD coord = { (short)core->screenWidth, (short)core->screenHeight };
	if (!SetConsoleScreenBufferSize(core->d_Console, coord))
	{
		InternalLog("FAILED: SetConsoleScreenBufferSize"); return false;
	}

	SMALL_RECT rectWindow = { 0, 0, (short)core->screenWidth - 1, (short)core->screenHeight - 1 };
	if (!SetConsoleWindowInfo(core->d_Console, TRUE, &rectWindow))
	{
		InternalLog("FAILED: SetConsoleWindowInfo (final size)"); return false;
	}

	if (!SetConsoleActiveScreenBuffer(core->d_Console))
	{
		InternalLog("FAILED: SetConsoleActiveScreenBuffer"); return false;
	}

	InternalLog("InitWindow SUCCESS");
	return true;

#elif defined(__APPLE__)
	bool useExisting = (mode == TerminalMode::Existing) || (mode == TerminalMode::Auto && isatty(STDOUT_FILENO));

	if (useExisting)
	{
		core->useExistingTerminal = true;
		core->graphicsFd = STDOUT_FILENO;

		tcgetattr(STDIN_FILENO, &core->originalTermios);

		struct termios raw = core->originalTermios;
		raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
		raw.c_oflag &= ~OPOST;
		raw.c_cflag |= CS8;
		raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
		raw.c_cc[VMIN] = 0;
		raw.c_cc[VTIME] = 0;

		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
		{
			InternalLog("FAILED: tcsetattr raw mode on macOS");
			return false;
		}
		core->rawModeEnabled = true;

		std::string setup =
			"\033%G"
			"\033[2J\033[H"
			"\033[?25l"
			"\033[?1002h\033[?1006h"
			"\033[?1004h";

		struct winsize ws = {};
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0)
		{
			if (ws.ws_col != width || ws.ws_row != height)
			{
				setup += "\033[8;" + std::to_string(height) + ";" +
					std::to_string(width) + "t";
			}
			else
			{
				core->screenWidth = ws.ws_col;
				core->screenHeight = ws.ws_row;
			}
		}

		std::string name_utf8(core->appName.begin(), core->appName.end());
		setup += "\033]0;" + name_utf8 + "\007";

		write(STDOUT_FILENO, setup.c_str(), setup.size());
		fflush(stdout);

		signal(SIGWINCH, ResizeSignalHandler);

		InternalLog("InitWindow SUCCESS - macOS Existing Terminal");
		return true;
	}
	else
	{
		std::string fp = "/tmp/donut_" + std::to_string(getpid());
		core->fifoPath = fp;
		unlink(fp.c_str());
		if (mkfifo(fp.c_str(), 0600) != 0)
		{
			InternalLog("FAILED: mkfifo (output)"); return false;
		}

		std::string fp_in = fp + "_in";
		core->inputFifoPath = fp_in;
		unlink(fp_in.c_str());
		if (mkfifo(fp_in.c_str(), 0600) != 0)
		{
			InternalLog("FAILED: mkfifo (input)"); return false;
		}

		std::string bridge_path = fp + "_bridge.py";
		FILE* bf = fopen(bridge_path.c_str(), "w");
		if (!bf) { InternalLog("FAILED: write bridge script"); return false; }
		fprintf(bf,
			"import sys,os,tty,termios,subprocess,select,signal\n"
			"out_fifo='%s'\n"
			"in_fifo='%s'\n"
			"fd=sys.stdin.fileno()\n"
			"old=termios.tcgetattr(fd)\n"
			"tty.setraw(fd)\n"
			"cat=subprocess.Popen(['cat',out_fifo],stdout=sys.stdout,close_fds=True)\n"
			"f=open(in_fifo,'wb',0)\n"
			"def on_resize(sig,frame):\n"
			" try:\n"
			"  sz=os.get_terminal_size(fd)\n"
			"  f.write(('\\033[8;'+str(sz.lines)+';'+str(sz.columns)+'t').encode())\n"
			" except:pass\n"
			"signal.signal(signal.SIGWINCH,on_resize)\n"
			"try:\n"
			" while cat.poll() is None:\n"
			"  r,_,_=select.select([sys.stdin],[],[],0.05)\n"
			"  if r:\n"
			"   c=os.read(fd,8)\n"
			"   f.write(c)\n"
			"except Exception:pass\n"
			"finally:\n"
			" f.close()\n"
			" cat.wait()\n"
			" termios.tcsetattr(fd,termios.TCSADRAIN,old)\n",
			fp.c_str(), fp_in.c_str()
		);
		fclose(bf);
		chmod(bridge_path.c_str(), 0755);

		std::string cmd = "osascript -e 'tell application \"Terminal\" to do script "
			"\"python3 " + bridge_path + "\"' &";
		system(cmd.c_str());

		core->graphicsFd = open(fp.c_str(), O_WRONLY);
		if (core->graphicsFd < 0)
		{
			InternalLog("FAILED: open output FIFO for writing"); return false;
		}

		core->inputFd = open(fp_in.c_str(), O_RDONLY);
		if (core->inputFd < 0)
		{
			InternalLog("FAILED: open input FIFO for reading"); return false;
		}

		fcntl(core->inputFd, F_SETFL, O_NONBLOCK);

		std::string name_utf8(core->appName.begin(), core->appName.end());
		std::string setup;
		setup += "\033%G";
		setup += "\033[2J";
		setup += "\033[H";
		setup += "\033[?25l";
		setup += "\033[8;" + std::to_string(core->screenHeight) + ";" + std::to_string(core->screenWidth) + "t";
		setup += "\033]0;" + name_utf8 + "\007";

		write(core->graphicsFd, setup.c_str(), setup.size());

		InternalLog("InitWindow SUCCESS (macOS — New terminal)");
		return true;
	}

#elif defined(__linux__)
	core->useExistingTerminal = (mode == TerminalMode::Existing) ||
		(mode == TerminalMode::Auto && (getenv("DONUT_EXISTING_TERM") || isatty(STDOUT_FILENO)));

	if (core->useExistingTerminal)
	{
		core->graphicsFd = STDOUT_FILENO;

		tcgetattr(STDIN_FILENO, &core->originalTermios);

		struct termios raw = core->originalTermios;
		raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
		raw.c_oflag &= ~OPOST;
		raw.c_cflag |= CS8;
		raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
		raw.c_cc[VMIN] = 0;
		raw.c_cc[VTIME] = 0;

		tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
		core->rawModeEnabled = true;

		std::string setup =
			"\033%G"
			"\033[?1049h"
			"\033[H"
			"\033[?25l"
			"\033[8;" + std::to_string(core->screenHeight) + ";" + std::to_string(core->screenWidth) + "t"
			"\033[?1002h\033[?1006h";

		struct winsize ws = {};
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
		{
			core->screenWidth = ws.ws_col;
			core->screenHeight = ws.ws_row;
		}

		std::string name_utf8(core->appName.begin(), core->appName.end());
		setup += "\033]0;" + name_utf8 + "\007";

		write(STDOUT_FILENO, setup.c_str(), setup.size());
		fflush(stdout);

		signal(SIGWINCH, ResizeSignalHandler);

		InternalLog("InitWindow SUCCESS - Existing terminal (" +
			std::to_string(core->screenWidth) + "x" + std::to_string(core->screenHeight) + ")");
		return true;
	}
	else
	{
		int master_fd = -1, slave_fd = -1;
		if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) < 0)
		{
			InternalLog("FAILED: openpty"); return false;
		}

		char* slave_name_ptr = ttyname(slave_fd);
		if (!slave_name_ptr)
		{
			InternalLog("FAILED: ttyname on slave PTY");
			close(master_fd); close(slave_fd); return false;
		}
		std::string slave_name = slave_name_ptr;

		struct termios tio;
		tcgetattr(slave_fd, &tio);
		tio.c_lflag &= ~(ICANON | ECHO | ISIG);
		tio.c_cc[VMIN] = 1;
		tio.c_cc[VTIME] = 0;
		tcsetattr(slave_fd, TCSANOW, &tio);

		std::string relay_path = "/tmp/donut_relay_" + std::to_string(getpid()) + ".sh";
		FILE* rf = fopen(relay_path.c_str(), "w");
		if (!rf)
		{
			InternalLog("FAILED: write relay script");
			close(master_fd); close(slave_fd);
			return false;
		}

		fprintf(rf,
			"#!/bin/sh\n"
			"SLAVE='%s'\n"
			"exec 2>/tmp/donut_relay_%d.log\n"
			"echo '[DonutAPI] Relay started' >&2\n"
			"\n"
			"stty raw -echo -icanon iutf8\n"
			"trap 'echo \"[DonutAPI] Relay exiting\" >&2' EXIT\n"
			"\n"
			"# Output pipe\n"
			"cat \"$SLAVE\" &\n"
			"OUT_PID=$!\n"
			"\n"
			"# Input pipe\n"
			"cat > \"$SLAVE\" &\n"
			"IN_PID=$!\n"
			"\n"
			"wait $OUT_PID $IN_PID\n"
			"echo '[DonutAPI] Relay pipes died' >&2\n",
			slave_name.c_str(), (int)getpid()
		);
		fclose(rf);
		chmod(relay_path.c_str(), 0755);

		std::string geom = std::to_string(core->screenWidth) + "x" + std::to_string(core->screenHeight) + "+50+50";
		std::string cmd = "xterm -geometry " + geom +
			" -title \"DonutAPI\" -hold -e '" + relay_path + "' &";

		if (system(cmd.c_str()) != 0)
		{
			InternalLog("FAILED: could not launch xterm");
			close(master_fd); close(slave_fd);
			return false;
		}

		core->graphicsFd = master_fd;
		core->fifoPath = relay_path;

		std::string setup =
			"\033%G"
			"\033[?1049h"
			"\033[2J\033[H"
			"\033[?25l"
			"\033[?1002h\033[?1006h";

		std::string name_utf8(core->appName.begin(), core->appName.end());
		setup += "\033]0;" + name_utf8 + "\007";

		write(core->graphicsFd, setup.c_str(), setup.size());

		fcntl(core->graphicsFd, F_SETFL, fcntl(core->graphicsFd, F_GETFL, 0) | O_NONBLOCK);

		struct winsize ws = {};
		ws.ws_row = core->screenHeight;
		ws.ws_col = core->screenWidth;
		ioctl(core->graphicsFd, TIOCSWINSZ, &ws);

		close(slave_fd);

		InternalLog("InitWindow SUCCESS (Linux - spawned xterm) ( " +
			std::to_string(core->screenWidth) + "x" + std::to_string(core->screenHeight) + ")");
		return true;
	}
#endif
}

DNAPI void DestroyWindow()
{
	if (core == nullptr) return;

	core->gameActive = false;

#if defined(_WIN32)
	SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
	SetConsoleActiveScreenBuffer(core->originalConsole);
	if (core->d_Console != INVALID_HANDLE_VALUE)
	{
		CloseHandle(core->d_Console);
		core->d_Console = INVALID_HANDLE_VALUE;
	}

#else
	if (core->rawModeEnabled)
	{
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &core->originalTermios);
		const char* restore = "\033[?25h\033[?1002l\033[?1006l\033[?1004l\033[?1049l\033[0m\n";
		write(STDOUT_FILENO, restore, strlen(restore));
		fflush(stdout);
	}
	else if (core->graphicsFd >= 0 && core->graphicsFd != STDOUT_FILENO)
	{
		const char* restore = "\033[?1049l\033[?1002l\033[?1006l\033[?25h\033[0m\n";
		write(core->graphicsFd, restore, strlen(restore));
		fsync(core->graphicsFd);
		close(core->graphicsFd);
		core->graphicsFd = -1;
	}

	if (!core->fifoPath.empty())
	{
		unlink(core->fifoPath.c_str());
		core->fifoPath.clear();
	}

#if defined(__APPLE__)
	if (!core->inputFifoPath.empty())
	{
		unlink(core->inputFifoPath.c_str());
		std::string bridge = core->fifoPath + "_bridge.py";
		unlink(bridge.c_str());
		core->inputFifoPath.clear();
	}
#endif
#endif

	delete core;
	core = nullptr;
	exit(0);
}

DNAPI void SetWindowName(const std::wstring& name)
{
	core->appName = name;
#if !defined(_WIN32)
	if (core && core->graphicsFd >= 0)
	{
		std::string name_utf8(name.begin(), name.end());
		std::string seq = "\033]0;" + name_utf8 + "\007";
		write(core->graphicsFd, seq.c_str(), seq.size());
		if (core->graphicsFd == STDOUT_FILENO)
			fflush(stdout);
	}
#endif
}

DNAPI void SetFPS(int fps)
{
	g_targetFrameTime = (fps > 0) ? (1.0f / fps) : 0.0f;
}

DNAPI bool WindowShouldClose()
{
	if (core == nullptr || !core->gameActive) return true;
	return false;
}

DNAPI void DrawPixel(int x, int y, short pixel, unsigned short color)
{
	if (x >= 0 && x < core->screenWidth && y >= 0 && y < core->screenHeight)
	{
		core->screenBuffer[y * core->screenWidth + x].Char.UnicodeChar = pixel;
		core->screenBuffer[y * core->screenWidth + x].Attributes = color;
	}
}

DNAPI void Clip(int& x, int& y)
{
	if (x < 0) x = 0;
	if (x >= core->screenWidth) x = core->screenWidth - 1;
	if (y < 0) y = 0;
	if (y >= core->screenHeight) y = core->screenHeight - 1;
}

DNAPI void Fill(int x1, int y1, int x2, int y2, short pixel, unsigned short color)
{
	Clip(x1, y1);
	Clip(x2, y2);
	for (int x = x1; x < x2; x++)
		for (int y = y1; y < y2; y++)
			DrawPixel(x, y, pixel, color);
}

DNAPI void DrawString(int x, int y, std::wstring c, unsigned short color)
{
	for (size_t i = 0; i < c.size(); i++)
	{
		int cx = x + (int)i;
		if (cx >= core->screenWidth) 
			break; // stop at right edge — no row wrap

		int index = y * core->screenWidth + cx;
		if (index < 0 || index >= core->screenWidth * core->screenHeight)
			break;
		
		core->screenBuffer[index].Char.UnicodeChar = c[i];
		core->screenBuffer[index].Attributes = color;
	}
}

DNAPI void DrawStringAlpha(int x, int y, std::wstring c, unsigned short color)
{
	for (size_t i = 0; i < c.size(); i++)
	{
		int cx = x + (int)i;
		if (cx >= core->screenWidth) 
			break; // stop at right edge — no row wrap

		int index = y * core->screenWidth + cx;
		if (index < 0 || index >= core->screenWidth * core->screenHeight) break;
		if (c[i] != L' ')
		{
			core->screenBuffer[index].Char.UnicodeChar = c[i];
			core->screenBuffer[index].Attributes = color;
		}
	}
}

DNAPI void DrawSprite(int x, int y)
{
	for (int i = 0; i < core->spriteWidth; i++)
	{
		for (int j = 0; j < core->spriteHeight; j++)
		{
			if (GetSprite(i, j) != L' ')
				DrawPixel(x + i, y + j, GetSprite(i, j), GetColor(i, j));
		}
	}
}

DNAPI void DrawLine(int x1, int y1, int x2, int y2, short pixel, unsigned short color)
{
	int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;

	dx = x2 - x1;
	dy = y2 - y1;

	dx1 = abs(dx);
	dy1 = abs(dy);

	px = 2 * dy1 - dx1;
	py = 2 * dx1 - dy1;

	if (dy1 <= dx1)
	{
		if (dx >= 0)
		{
			x = x1; y = y1; xe = x2;
		}
		else
		{
			x = x2; y = y2; xe = x1;
		}

		DrawPixel(x, y, pixel, color);

		for (i = 0; x < xe; i++)
		{
			x = x + 1;
			if (px < 0)
				px = px + 2 * dy1;
			else
			{
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) y = y + 1; else y = y - 1;
				px = px + 2 * (dy1 - dx1);
			}
			DrawPixel(x, y, pixel, color);
		}
	}
	else
	{
		if (dy >= 0)
		{
			x = x1; y = y1; ye = y2;
		}
		else
		{
			x = x2; y = y2; ye = y1;
		}

		DrawPixel(x, y, pixel, color);

		for (i = 0; y < ye; i++)
		{
			y = y + 1;
			if (py <= 0)
				py = py + 2 * dx1;
			else
			{
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) x = x + 1; else x = x - 1;
				py = py + 2 * (dx1 - dy1);
			}
			DrawPixel(x, y, pixel, color);
		}
	}
}

DNAPI void DrawRectangle(int x, int y, int sidelength, short pixel, unsigned short color)
{
	int half = sidelength / 2;
	int left = x - half;
	int right = x + half;
	int top = y - half;
	int bottom = y + half;

	DrawLine(left, top, right, top, pixel, color);
	DrawLine(right, top, right, bottom, pixel, color);
	DrawLine(right, bottom, left, bottom, pixel, color);
	DrawLine(left, bottom, left, top, pixel, color);
}

DNAPI void FillRectangle(int x, int y, int sidelength, short pixel, unsigned short color)
{
	int half = sidelength / 2;
	Fill(x - half, y - half, x + half + 1, y + half + 1, pixel, color);
}

DNAPI void DrawRotableRectangle(int x, int y, int sidelength, float rotation, short pixel, unsigned short color)
{
	float h = sidelength / 2.0f;

	std::vector<std::pair<float, float>> square =
	{
		{ -h, -h },
		{  h, -h },
		{  h,  h },
		{ -h,  h },
	};

	DrawWireFrameModel(square, (float)x, (float)y, rotation, 1.0f, color, pixel);
}

DNAPI void FillRotableRectangle(int x, int y, int sidelength, float rotation, short pixel, unsigned short color)
{
	float h = (float)sidelength / 2.0f;
	float c = cosf(rotation);
	float s = sinf(rotation);

	std::pair<float, float> corners[4] =
	{
			{ -h, -h },
			{  h, -h },
			{  h,  h },
			{ -h,  h },
	};

	int rx[4], ry[4];
	for (int i = 0; i < 4; i++)
	{
		rx[i] = (int)(corners[i].first * c - corners[i].second * s + x);
		ry[i] = (int)(corners[i].first * s + corners[i].second * c + y);
	}

	FillTriangle(rx[0], ry[0], rx[1], ry[1], rx[2], ry[2], pixel, color);
	FillTriangle(rx[0], ry[0], rx[2], ry[2], rx[3], ry[3], pixel, color);
}

DNAPI void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short pixel, unsigned short color)
{
	DrawLine(x1, y1, x2, y2, pixel, color);
	DrawLine(x2, y2, x3, y3, pixel, color);
	DrawLine(x3, y3, x1, y1, pixel, color);
}

DNAPI void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short pixel, unsigned short color)
{
	auto SWAP = [](int& x, int& y) { int t = x; x = y; y = t; };
	auto drawline = [&](int sx, int ex, int ny) { for (int i = sx; i <= ex; i++) DrawPixel(i, ny, pixel, color); };

	int t1x, t2x, y, minx, maxx, t1xp, t2xp;
	bool changed1 = false, changed2 = false;
	int signx1, signx2, dx1, dy1, dx2, dy2, e1, e2;

	if (y1 > y2) { SWAP(y1, y2); SWAP(x1, x2); }
	if (y1 > y3) { SWAP(y1, y3); SWAP(x1, x3); }
	if (y2 > y3) { SWAP(y2, y3); SWAP(x2, x3); }

	t1x = t2x = x1; y = y1;
	dx1 = (int)(x2 - x1);

	if (dx1 < 0) { dx1 = -dx1; signx1 = -1; }
	else signx1 = 1;
	dy1 = (int)(y2 - y1);

	dx2 = (int)(x3 - x1);
	if (dx2 < 0) { dx2 = -dx2; signx2 = -1; }
	else signx2 = 1;
	dy2 = (int)(y3 - y1);

	if (dy1 > dx1) { SWAP(dx1, dy1); changed1 = true; }
	if (dy2 > dx2) { SWAP(dy2, dx2); changed2 = true; }

	e2 = (int)(dx2 >> 1);
	if (y1 == y2) goto next;
	e1 = (int)(dx1 >> 1);

	for (int i = 0; i < dx1;) {
		t1xp = 0; t2xp = 0;
		if (t1x < t2x) { minx = t1x; maxx = t2x; }
		else { minx = t2x; maxx = t1x; }

		while (i < dx1)
		{
			i++;
			e1 += dy1;
			while (e1 >= dx1)
			{
				e1 -= dx1;
				if (changed1) t1xp = signx1;
				else goto next1;
			}
			if (changed1) break;
			else t1x += signx1;
		}
	next1:
		while (1)
		{
			e2 += dy2;
			while (e2 >= dx2)
			{
				e2 -= dx2;
				if (changed2) t2xp = signx2;
				else goto next2;
			}
			if (changed2) break;
			else t2x += signx2;
		}
	next2:
		if (minx > t1x) minx = t1x;
		if (minx > t2x) minx = t2x;
		if (maxx < t1x) maxx = t1x;
		if (maxx < t2x) maxx = t2x;

		drawline(minx, maxx, y);
		if (!changed1) t1x += signx1;
		t1x += t1xp;
		if (!changed2) t2x += signx2;
		t2x += t2xp;
		y += 1;
		if (y == y2) break;
	}
next:
	dx1 = (int)(x3 - x2);
	if (dx1 < 0) { dx1 = -dx1; signx1 = -1; }
	else signx1 = 1;
	dy1 = (int)(y3 - y2);
	t1x = x2;

	if (dy1 > dx1) { SWAP(dy1, dx1); changed1 = true; }
	else changed1 = false;

	e1 = (int)(dx1 >> 1);

	for (int i = 0; i <= dx1; i++)
	{
		t1xp = 0; t2xp = 0;
		if (t1x < t2x) { minx = t1x; maxx = t2x; }
		else { minx = t2x; maxx = t1x; }

		while (i < dx1)
		{
			e1 += dy1;
			while (e1 >= dx1)
			{
				e1 -= dx1;
				if (changed1) { t1xp = signx1; break; }
				else goto next3;
			}
			if (changed1) break;
			else t1x += signx1;
			if (i < dx1) i++;
		}
	next3:
		while (t2x != x3)
		{
			e2 += dy2;
			while (e2 >= dx2)
			{
				e2 -= dx2;
				if (changed2) t2xp = signx2;
				else goto next4;
			}
			if (changed2) break;
			else t2x += signx2;
		}
	next4:
		if (minx > t1x) minx = t1x;
		if (minx > t2x) minx = t2x;
		if (maxx < t1x) maxx = t1x;
		if (maxx < t2x) maxx = t2x;

		drawline(minx, maxx, y);
		if (!changed1) t1x += signx1;
		t1x += t1xp;
		if (!changed2) t2x += signx2;
		t2x += t2xp;
		y += 1;
		if (y > y3) return;
	}
}

DNAPI void DrawTriangleStrip(const Vector2* points, int pointCount, short pixel, unsigned short color)
{
	// A triangle strip requires at least 3 points
	if (pointCount < 3) return;

	for (int i = 0; i < pointCount - 2; i++)
	{
		// Draw the outline of each triangle in the strip
		DrawTriangle(
			(int)points[i].x, (int)points[i].y,
			(int)points[i + 1].x, (int)points[i + 1].y,
			(int)points[i + 2].x, (int)points[i + 2].y,
			pixel, color
		);
	}
}

DNAPI void FillTriangleStrip(const Vector2* points, int pointCount, short pixel, unsigned short color)
{
	// A triangle strip requires at least 3 points
	if (pointCount < 3) return;

	for (int i = 0; i < pointCount - 2; i++)
	{
		// Fill a triangle strip
		FillTriangle((int)points[i].x, (int)points[i].y,
			(int)points[i + 1].x, (int)points[i + 1].y,
			(int)points[i + 2].x, (int)points[i + 2].y,
			pixel, color
		);
	}
}

DNAPI void DrawTriangleFan(const Vector2* points, int pointCount, short pixel, unsigned short color)
{
	if (pointCount < 3) return;

	for (int i = 0; i < pointCount - 2; i++)
	{
		DrawTriangle(
			(int)points[0].x, (int)points[0].y,
			(int)points[i + 1].x, (int)points[i + 1].y,
			(int)points[i + 2].x, (int)points[i + 2].y,
			pixel, color
		);
	}
}

DNAPI void FillTriangleFan(const Vector2* points, int pointCount, short pixel, unsigned short color)
{
	if (pointCount < 3) return;

	for (int i = 0; i < pointCount - 2; i++)
	{
		FillTriangle(
			(int)points[0].x, (int)points[0].y,
			(int)points[i + 1].x, (int)points[i + 1].y,
			(int)points[i + 2].x, (int)points[i + 2].y,
			pixel, color
		);
	}
}

DNAPI void DrawCircle(int xc, int yc, int r, short pixel, unsigned short color)
{
	int x = 0;
	int y = r;
	int pr = 3 - 2 * r;

	if (!r) return;

	while (y >= x)
	{
		DrawPixel(xc - x, yc - y, pixel, color);
		DrawPixel(xc - y, yc - x, pixel, color);
		DrawPixel(xc + y, yc - x, pixel, color);
		DrawPixel(xc + x, yc - y, pixel, color);
		DrawPixel(xc - x, yc + y, pixel, color);
		DrawPixel(xc - y, yc + x, pixel, color);
		DrawPixel(xc + y, yc + x, pixel, color);
		DrawPixel(xc + x, yc + y, pixel, color);

		if (pr < 0) pr += 4 * x++ + 6;
		else pr += 4 * (x++ - y--) + 10;
	}
}

DNAPI void FillCircle(int xc, int yc, int r, short pixel, unsigned short color)
{
	int x = 0;
	int y = r;
	int p = 3 - 2 * r;
	if (!r) return;

	auto drawline = [&](int sx, int ex, int ny)
		{
			for (int i = sx; i <= ex; i++)
				DrawPixel(i, ny, pixel, color);
		};

	while (y >= x)
	{
		drawline(xc - x, xc + x, yc - y);
		drawline(xc - y, xc + y, yc - x);
		drawline(xc - x, xc + x, yc + y);
		drawline(xc - y, xc + y, yc + x);

		if (p < 0) p += 4 * x++ + 6;
		else p += 4 * (x++ - y--) + 10;
	}
}

DNAPI void DrawCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, short pixel, unsigned short color)
{
	float start = startAngle * 3.14159f / 180.0f;
	float end = endAngle * 3.14159f / 180.0f;
	float angleStep = (end - start) / segments;

	for (int i = 0; i < segments; i++)
	{
		float angle1 = start + i * angleStep;
		float angle2 = start + (i + 1) * angleStep;

		int x1 = (int)center.x + (int)(radius * cosf(angle1));
		int y1 = (int)center.y + (int)(radius * sinf(angle1));
		int x2 = (int)center.x + (int)(radius * cosf(angle2));
		int y2 = (int)center.y + (int)(radius * sinf(angle2));

		DrawLine(x1, y1, x2, y2, pixel, color);
	}
}

DNAPI void FillCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, short pixel, unsigned short color)
{
	float angleStep = 2.0f * 3.14159f / segments;

	for (int i = 0; i < segments; i++)
	{
		float angle1 = i * angleStep;
		float angle2 = (i + 1) * angleStep;

		// Perimeter point 1
		int x1 = (int)center.x + (int)(radius * cosf(angle1));
		int y1 = (int)center.y + (int)(radius * sinf(angle1));

		// Perimeter point 2
		int x2 = (int)center.x + (int)(radius * cosf(angle2));
		int y2 = (int)center.y + (int)(radius * sinf(angle2));

		// Fill triangle from center to the two perimeter points
		FillTriangle(center.x, center.y, x1, y1, x2, y2, pixel, color);
	}
}

DNAPI void DrawEllipse(int xc, int yc, int a, int b, int angle, short pixel, unsigned short color)
{
	float t = 3.14f / 180.0f; 
	angle = 360 - (float)angle;
	float theta;

	for (int i = 0; i < 360; i += 1)
	{
		theta = i;
		int x = a * cos(t * theta) * cos(t * angle) + b * sin(t * theta) * sin(t * angle);

		int y = b * sin(t * theta) * cos(t * angle) - a * cos(t * theta) * sin(t * angle);

		DrawPixel(xc + x, yc - y, pixel, color);
	}
}

DNAPI void FillEllipse(int xc, int yc, int a, int b, int angle, short pixel, unsigned short color)
{
	float t = 3.14f / 180.0f;
	angle = 360 - angle;
	float theta;

	for (int i = 0; i < 360; i += 1)
	{
		theta = i;
		int x = a * cos(t * theta) * cos(t * angle)
			+ b * sin(t * theta) * sin(t * angle);

		int y = b * sin(t * theta) * cos(t * angle) - a * cos(t * theta) * sin(t * angle);

		DrawLine(xc + x, yc - y, xc - x - 1, yc + y, pixel, color);
	}
}

DNAPI void DrawPoly(const Vector2* vertices, int count, short pixel, unsigned short color)
{
	if (count < 2) return;

	// Draw a line between each consecutive vertex, closing back to the first
	for (int i = 0; i < count; i++)
	{
		int j = (i + 1) % count;
		DrawLine(
			(int)vertices[i].x, (int)vertices[i].y,
			(int)vertices[j].x, (int)vertices[j].y,
			pixel, color
		);
	}
}

DNAPI void FillPoly(const Vector2* vertices, int count, short pixel, unsigned short color)
{
	if (count < 3) return;

	// Find the Y extents of the polygon so we know which scanlines to process
	int yMin = (int)vertices[0].y;
	int yMax = (int)vertices[0].y;
	for (int i = 1; i < count; i++)
	{
		if ((int)vertices[i].y < yMin) yMin = (int)vertices[i].y;
		if ((int)vertices[i].y > yMax) yMax = (int)vertices[i].y;
	}

	// Clamp to screen bounds
	if (yMin < 0)                   yMin = 0;
	if (yMax >= core->screenHeight) yMax = core->screenHeight - 1;

	// Scanline fill — for each horizontal row find where polygon edges cross it,
	// sort those X intersections, then fill between each pair (even-odd rule)
	std::vector<int> xIntersections;
	xIntersections.reserve(count);

	for (int y = yMin; y <= yMax; y++)
	{
		xIntersections.clear();

		for (int i = 0; i < count; i++)
		{
			int j = (i + 1) % count;

			float ay = vertices[i].y;
			float by = vertices[j].y;

			// Skip horizontal edges — they contribute no intersection
			if ((int)ay == (int)by) continue;

			// Only process edges that cross this scanline (top-left fill convention:
			// include the top, exclude the bottom to avoid double-drawing shared edges)
			float yLo = (ay < by) ? ay : by;
			float yHi = (ay < by) ? by : ay;

			if ((float)y < yLo || (float)y >= yHi) continue;

			// Linear interpolation: find the X where this edge crosses scanline y
			float t = ((float)y - ay) / (by - ay);
			int x = (int)(vertices[i].x + t * (vertices[j].x - vertices[i].x));

			xIntersections.push_back(x);
		}

		// Sort intersections left to right
		std::sort(xIntersections.begin(), xIntersections.end());

		// Fill between each pair of intersections
		for (int k = 0; k + 1 < (int)xIntersections.size(); k += 2)
		{
			int xStart = xIntersections[k];
			int xEnd = xIntersections[k + 1];

			// Clamp to screen bounds
			if (xStart < 0) xStart = 0;
			if (xEnd >= core->screenWidth)   xEnd = core->screenWidth - 1;

			for (int x = xStart; x <= xEnd; x++)
				DrawPixel(x, y, pixel, color);
		}
	}
}

DNAPI void DrawWireFrameModel(const std::vector<std::pair<float, float>>& vecModelCoordinates, float x, float y, float r, float s, short pixel, unsigned short color)
{
	std::vector<std::pair<float, float>> vecTransformedCoordinates;
	int verts = vecModelCoordinates.size();
	vecTransformedCoordinates.resize(verts);

	for (int i = 0; i < verts; i++)
	{
		vecTransformedCoordinates[i].first = vecModelCoordinates[i].first * cosf(r) - vecModelCoordinates[i].second * sinf(r);
		vecTransformedCoordinates[i].second = vecModelCoordinates[i].first * sinf(r) + vecModelCoordinates[i].second * cosf(r);
	}

	for (int i = 0; i < verts; i++)
	{
		vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first * s;
		vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second * s;
	}

	for (int i = 0; i < verts; i++)
	{
		vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first + x;
		vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second + y;
	}

	for (int i = 0; i < verts + 1; i++)
	{
		int j = (i + 1);
		DrawLine((int)vecTransformedCoordinates[i % verts].first, (int)vecTransformedCoordinates[i % verts].second,
			(int)vecTransformedCoordinates[j % verts].first, (int)vecTransformedCoordinates[j % verts].second, pixel, color);
	}
}

DNAPI KeyState GetKey(int keycode)
{
	if (core == nullptr || keycode < 0 || keycode > 255)
		return { false, false, false };

	return core->k_keys[keycode];
}

DNAPI void FlushKeys()
{
	if (core == nullptr) return;

	memset(core->k_keys, 0, sizeof(core->k_keys));
	memset(core->k_keyOldState, 0, sizeof(core->k_keyOldState));
	memset(core->k_keyNewState, 0, sizeof(core->k_keyNewState));
}

DNAPI std::string PollInput()
{
#if defined(_WIN32)
	static const std::pair<int, char> keyMap[] =
	{
		{'A','a'},{'B','b'},{'C','c'},{'D','d'},{'E','e'},
		{'F','f'},{'G','g'},{'H','h'},{'I','i'},{'J','j'},
		{'K','k'},{'L','l'},{'M','m'},{'N','n'},{'O','o'},
		{'P','p'},{'Q','q'},{'R','r'},{'S','s'},{'T','t'},
		{'U','u'},{'V','v'},{'W','w'},{'X','x'},{'Y','y'},
		{'Z','z'},
		{'1','1'},{'2','2'},{'3','3'},{'4','4'},{'5','5'},
		{'6','6'},{'7','7'},{'8','8'},{'9','9'},{'0','0'},
	};

	{
		bool anyHeld = true;
		while (anyHeld)
		{
			anyHeld = false;
			for (auto& [vk, ch] : keyMap)
				if (GetAsyncKeyState(vk) & 0x8000) { anyHeld = true; break; }
			if (GetAsyncKeyState(VK_RETURN) & 0x8000) anyHeld = true;
			if (GetAsyncKeyState(VK_BACK) & 0x8000) anyHeld = true;
			if (anyHeld) Delay(16);
		}
		GetKeyState();
	}

	std::string result;

	auto renderInput = [&]()
		{
			std::cout << "\r> " << result << " " << std::flush;
		};

	renderInput();

	while (true)
	{
		GetKeyState();
		bool changed = false;

		if (GetKey(VK_BACK).k_Pressed && !result.empty())
		{
			result.pop_back();
			changed = true;
		}
		if (GetKey(VK_RETURN).k_Pressed)
		{
			std::cout << "\n" << std::flush;
			break;
		}
		for (auto& [vk, ch] : keyMap)
		{
			if (GetKey(vk).k_Pressed)
			{
				result += ch;
				changed = true;
				break;
			}
		}
		if (changed) renderInput();
		Delay(16);
	}

	FlushKeys();
	return result;

#else
	// On Linux we read from the PTY master (graphicsFd)
	// On macOS we read from the keyboard input FIFO (inputFd)
	// In both cases we echo each character back to graphicsFd so it appears in the
	// game window rather than disappearing
#if defined(__APPLE__)
	int poll_in = (core && core->inputFd >= 0) ? core->inputFd : STDIN_FILENO;
#else
	int poll_in = (core && core->graphicsFd >= 0) ? core->graphicsFd : STDIN_FILENO;
#endif
	int poll_out = (core && core->graphicsFd >= 0) ? core->graphicsFd : STDOUT_FILENO;

	// Make the input fd blocking for the duration of PollInput
	int saved_flags = fcntl(poll_in, F_GETFL, 0);
	fcntl(poll_in, F_SETFL, saved_flags & ~O_NONBLOCK);

	std::string result;
	const char* prompt = "> ";
	write(poll_out, prompt, 2);

	while (true)
	{
		char c;
		if (read(poll_in, &c, 1) == 1)
		{
			if (c == '\n' || c == '\r')
			{
				write(poll_out, "\n", 1);
				break;
			}
			else if (c == 127 || c == '\b') // backspace / DEL
			{
				if (!result.empty())
				{
					result.pop_back();
					const char* bs = "\b \b";
					write(poll_out, bs, 3);
				}
			}
			else if (c >= 32 && c < 127)
			{
				result += c;
				write(poll_out, &c, 1);
			}
		}
	}

	// Restore non-blocking mode that GetKeyState expects.
	fcntl(poll_in, F_SETFL, saved_flags);

	FlushKeys();
	return result;
#endif
}

DNAPI void UpdateScreen()
{
	if (core == nullptr || core->screenBuffer == nullptr)
		return;

	static auto tp1 = std::chrono::steady_clock::now();
	auto tp2 = std::chrono::steady_clock::now();
	core->elapsedTime = std::chrono::duration<float>(tp2 - tp1).count();

	if (g_targetFrameTime > 0.0f && core->elapsedTime < g_targetFrameTime)
	{
		float sleepSec = g_targetFrameTime - core->elapsedTime;
		std::this_thread::sleep_for(
			std::chrono::microseconds((long long)(sleepSec * 1e6f))
		);
		tp2 = std::chrono::steady_clock::now();
		core->elapsedTime = std::chrono::duration<float>(tp2 - tp1).count();
	}

	tp1 = tp2;
	if (core->elapsedTime == 0.0f) core->elapsedTime = 0.0001f;

#if defined(_WIN32)
	wchar_t s[256];
	swprintf_s(s, 256, L"%ls - FPS: %3.2f", core->appName.c_str(), 1.0f / core->elapsedTime);
	SetConsoleTitle(s);

	for (int row = 0; row < core->screenHeight; row++)
	{
		SMALL_RECT rowRect = { 0, (SHORT)row, (SHORT)(core->screenWidth - 1), (SHORT)row };
		COORD bufSize = { (SHORT)core->screenWidth, 1 };
		COORD bufCoord = { 0, 0 };
		WriteConsoleOutput(
			core->d_Console,
			&core->screenBuffer[row * core->screenWidth],
			bufSize, bufCoord, &rowRect
		);
	}

#else
	std::string out;
	out.reserve(core->screenWidth * core->screenHeight * 16 + 64);

	wchar_t title[256];
	swprintf(title, 256, L"%ls - FPS: %3.1f", core->appName.c_str(),
		(core->elapsedTime > 0.0f) ? 1.0f / core->elapsedTime : 0.0f);

	std::string title_utf8(title, title + wcslen(title));
	out += "\033]0;" + title_utf8 + "\007";
	out += "\033[H";

	static const char* ansiColor[16] =
	{
		"\033[30m", "\033[34m", "\033[32m", "\033[36m",
		"\033[31m", "\033[35m", "\033[33m", "\033[37m",
		"\033[90m", "\033[94m", "\033[92m", "\033[96m",
		"\033[91m", "\033[95m", "\033[93m", "\033[97m",
	};

	unsigned short lastColor = 0xFFFF;

	for (int y = 0; y < core->screenHeight; ++y)
	{
		for (int x = 0; x < core->screenWidth; ++x)
		{
			const CHAR_INFO& cell = core->screenBuffer[y * core->screenWidth + x];
			wchar_t ch = cell.Char.UnicodeChar;
			unsigned short attr = cell.Attributes & 0x000F;

			if (ch == 0) ch = L' ';

			if (attr != lastColor)
			{
				out += ansiColor[attr];
				lastColor = attr;
			}

			if (ch < 0x80)
			{
				out += static_cast<char>(ch);
			}

			else if (ch < 0x800)
			{
				out += static_cast<char>(0xC0 | (ch >> 6));
				out += static_cast<char>(0x80 | (ch & 0x3F));
			}

			else
			{
				out += static_cast<char>(0xE0 | (ch >> 12));
				out += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
				out += static_cast<char>(0x80 | (ch & 0x3F));
			}
		}

		if (y < core->screenHeight - 1)
			out += '\n';
		else
			lastColor = 0xFFFF;
	}

	out += "\033[0m";

	if (core->graphicsFd >= 0)
	{
		write(core->graphicsFd, out.c_str(), out.size());
	}
	else
	{
		fwrite(out.c_str(), 1, out.size(), stdout);
		fflush(stdout);
	}
#endif
}

DNAPI void GetKeyState()
{
	if (core == nullptr || !core->gameActive) return;

#ifdef _WIN32
	for (int i = 0; i < 256; i++)
	{
		core->k_keyOldState[i] = core->k_keyNewState[i];
		core->k_keyNewState[i] = GetAsyncKeyState(i);

		core->k_keys[i].k_Pressed = false;
		core->k_keys[i].k_Released = false;

		if (core->k_keyNewState[i] & 0x8000)
		{
			if (!(core->k_keyOldState[i] & 0x8000))
				core->k_keys[i].k_Pressed = true;
			core->k_keys[i].k_Held = true;
		}
		else
		{
			if (core->k_keyOldState[i] & 0x8000)
				core->k_keys[i].k_Released = true;
			core->k_keys[i].k_Held = false;
		}
	}

	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	DWORD numEvents = 0;
	GetNumberOfConsoleInputEvents(hInput, &numEvents);

	while (numEvents > 0)
	{
		INPUT_RECORD record;
		DWORD read = 0;
		ReadConsoleInput(hInput, &record, 1, &read);

		if (record.EventType == MOUSE_EVENT)
		{
			core->m_mousePosX = record.Event.MouseEvent.dwMousePosition.X;
			core->m_mousePosY = record.Event.MouseEvent.dwMousePosition.Y;

			DWORD buttons = record.Event.MouseEvent.dwButtonState;

			bool newLeft = (buttons & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
			bool newMiddle = (buttons & FROM_LEFT_2ND_BUTTON_PRESSED) != 0;
			bool newRight = (buttons & RIGHTMOST_BUTTON_PRESSED) != 0;

			core->m_mousePressed[0] = newLeft && !core->m_mouseButtons[0];
			core->m_mouseReleased[0] = !newLeft && core->m_mouseButtons[0];
			core->m_mouseButtons[0] = newLeft;

			core->m_mousePressed[1] = newMiddle && !core->m_mouseButtons[1];
			core->m_mouseReleased[1] = !newMiddle && core->m_mouseButtons[1];
			core->m_mouseButtons[1] = newMiddle;

			core->m_mousePressed[2] = newRight && !core->m_mouseButtons[2];
			core->m_mouseReleased[2] = !newRight && core->m_mouseButtons[2];
			core->m_mouseButtons[2] = newRight;

			if (record.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED)
			{
				short delta = HIWORD(record.Event.MouseEvent.dwButtonState);
				core->m_mouseWheelDelta += (delta > 0) ? 1 : -1;
			}
		}
		numEvents--;
	}

#else // Linux or macOS
#if defined(__APPLE__)
	int input_fd = core->useExistingTerminal ? STDIN_FILENO :
		(core->inputFd >= 0 ? core->inputFd : STDIN_FILENO);
#else
	int input_fd = core->useExistingTerminal ? STDIN_FILENO : core->graphicsFd;
#endif

	static std::chrono::steady_clock::time_point keyLastSeen[256] = {};
	auto now = std::chrono::steady_clock::now();

	for (int i = 0; i < 256; i++)
	{
		core->k_keys[i].k_Pressed = false;
		core->k_keys[i].k_Released = false;
	}

	// Reset per-frame mouse transient states independently of key state.
	// m_mouseButtons[] (held) is NOT reset here — it persists until a release event.
	for (int b = 0; b < 3; ++b)
	{
		core->m_mousePressed[b] = false;
		core->m_mouseReleased[b] = false;
	}
	core->m_mouseWheelDelta = 0;

	if (input_fd < 0) return;

	struct pollfd pfd = { input_fd, POLLIN | POLLHUP | POLLERR, 0 };

	if (poll(&pfd, 1, 0) > 0)
	{
		// Check POLLIN *first* — Linux can set both POLLIN and POLLHUP simultaneously
		// during a resize. Processing data first prevents the probe read in the POLLHUP
		// branch from silently consuming the leading byte of the resize escape sequence.
		if (pfd.revents & POLLIN)
		{
			char buf[256] = {};
			int n = read(input_fd, buf, sizeof(buf));

			if (n < 0 && errno == EIO)
			{
				InternalLog("GetKeyState: CLOSE — read EIO (slave closed)");
				core->gameActive = false;
				goto release_expired;
			}
			if (n == 0)
			{
				InternalLog("GetKeyState: CLOSE — read returned 0 (EOF)");
				core->gameActive = false;
				goto release_expired;
			}
			if (n < 0)
				goto release_expired; // EAGAIN / EWOULDBLOCK — no data right now

			auto markKey = [&](int k)
				{
					if (k < 0 || k > 255) return;
					if (!core->k_keys[k].k_Held) core->k_keys[k].k_Pressed = true;
					core->k_keys[k].k_Held = true;
					keyLastSeen[k] = now;
				};

			for (int pos = 0; pos < n; )
			{
				if (buf[pos] == '\033' && pos + 1 < n)
				{
					++pos; // consume ESC

					if (buf[pos] == '[')
					{
						++pos; // consume '['

						// ── SGR mouse sequence:  ESC [ < btn ; col ; row M/m ──
						if (pos < n && buf[pos] == '<')
						{
							++pos; // consume '<'

							int params[3] = { 0, 0, 0 };
							int pi = 0;
							char finalByte = 0;

							while (pos < n)
							{
								char c = buf[pos++];
								if (c >= '0' && c <= '9')
									params[pi] = params[pi] * 10 + (c - '0');
								else if (c == ';')
								{
									if (pi < 2) ++pi;
								}
								else { finalByte = c; break; }
							}

							// params: [0]=button-code  [1]=col(1-based)  [2]=row(1-based)
							int  btn = params[0];
							int  mx = params[1] - 1; // convert to 0-based
							int  my = params[2] - 1;
							bool isMotion = (btn & 32) != 0;

							// Always update position — covers drags and motion events.
							core->m_mousePosX = mx;
							core->m_mousePosY = my;

							if (!isMotion)
							{
								int physBtn = btn & 3; // 0=left  1=middle  2=right
								if (physBtn <= 2)
								{
									if (finalByte == 'M')       // press
									{
										core->m_mouseButtons[physBtn] = true;
										core->m_mousePressed[physBtn] = true;
										core->m_mouseReleased[physBtn] = false;
									}
									else if (finalByte == 'm')  // release
									{
										core->m_mouseButtons[physBtn] = false;
										core->m_mousePressed[physBtn] = false;
										core->m_mouseReleased[physBtn] = true;
									}
								}
								else if (btn == 64) core->m_mouseWheelDelta += 1; // scroll up
								else if (btn == 65) core->m_mouseWheelDelta -= 1; // scroll down
							}
						}
						else
						{
							// Collect everything up to the CSI final byte (A-Z, a-z, ~).
							int paramStart = pos;
							while (pos < n &&
								!(buf[pos] >= 'A' && buf[pos] <= 'Z') &&
								!(buf[pos] >= 'a' && buf[pos] <= 'z') &&
								buf[pos] != '~')
								++pos;
							char finalByte = (pos < n) ? buf[pos++] : 0;

							// Resize notification:  ESC [ 8 ; rows ; cols t
							if (finalByte == 't')
							{
								int nums[3] = { 0, 0, 0 };
								int ni = 0;
								for (int ip = paramStart; ip < pos - 1 && ni < 3; ++ip)
								{
									if (buf[ip] >= '0' && buf[ip] <= '9')
										nums[ni] = nums[ni] * 10 + (buf[ip] - '0');
									else if (buf[ip] == ';')
										++ni;
								}
								if (nums[0] == 8 && nums[1] > 0 && nums[2] > 0)
									ResizeScreenBuffer(nums[2], nums[1]);
							}
							else
							{
								int mapped = -1;
								switch (finalByte)
								{
								case 'A': mapped = KEY_UP;    break;
								case 'B': mapped = KEY_DOWN;  break;
								case 'C': mapped = KEY_RIGHT; break;
								case 'D': mapped = KEY_LEFT;  break;
								case 'H': mapped = KEY_HOME;  break;
								case 'F': mapped = KEY_END;   break;
								case '~':
								{
									int num = 0;
									for (int ip = paramStart; ip < pos - 1; ++ip)
										if (buf[ip] >= '0' && buf[ip] <= '9')
											num = num * 10 + (buf[ip] - '0');
									switch (num)
									{
									case 2:  mapped = KEY_INSERT;  break;
									case 3:  mapped = KEY_DELETE;  break;
									case 5:  mapped = KEY_PG_UP;   break;
									case 6:  mapped = KEY_PG_DOWN; break;
									case 15: mapped = KEY_F5;      break;
									case 17: mapped = KEY_F6;      break;
									case 18: mapped = KEY_F7;      break;
									case 19: mapped = KEY_F8;      break;
									case 20: mapped = KEY_F9;      break;
									case 21: mapped = KEY_F10;     break;
									case 23: mapped = KEY_F11;     break;
									case 24: mapped = KEY_F12;     break;
									}
									break;
								}
								}
								if (mapped >= 0) markKey(mapped);
								else             markKey(KEY_ESC);
							}
						}
					}
					else if (buf[pos] == 'O')
					{
						++pos; // consume 'O'
						if (pos < n)
						{
							int mapped = -1;
							switch (buf[pos++])
							{
							case 'P': mapped = KEY_F1; break;
							case 'Q': mapped = KEY_F2; break;
							case 'R': mapped = KEY_F3; break;
							case 'S': mapped = KEY_F4; break;
							}
							if (mapped >= 0) markKey(mapped);
							else             markKey(KEY_ESC);
						}
						else markKey(KEY_ESC);
					}
					else
					{
						markKey(KEY_ESC); // lone ESC or unrecognised two-byte sequence
					}
				}
				else
				{
					unsigned char k = (unsigned char)buf[pos++];
					if (k == 127) k = 8; // DEL -> Backspace
					markKey(k);
					if (k >= 'a' && k <= 'z') markKey(k - 32); // also mark uppercase
				}
			}
		}
		else if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL))
		{
			// No data (POLLIN not set). Confirm hangup — transient POLLHUP on a live
			// PTY master returns EAGAIN; genuine slave-close returns EIO.
			char probe;
			ssize_t r = read(input_fd, &probe, 1);
			if (r == 0 || (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
			{
				InternalLog("GetKeyState: CLOSE — POLLHUP confirmed errno=" + std::to_string(errno));
				core->gameActive = false;
			}
			goto release_expired;
		}
	}

release_expired:
	for (int i = 0; i < 256; i++)
	{
		if (core->k_keys[i].k_Held)
		{
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
				now - keyLastSeen[i]).count();
			if (ms > 100)
			{
				core->k_keys[i].k_Released = true;
				core->k_keys[i].k_Held = false;
			}
		}
	}
#endif
}

// Mouse position

DNAPI int GetMouseX()
{
	if (core == nullptr) return { 0 };
	return core->m_mousePosX;
}

DNAPI int GetMouseY()
{
	if (core == nullptr) return { 0 };
	return core->m_mousePosY;
}

DNAPI std::pair<int, int> GetMousePos()
{
	if (core == nullptr) return { 0, 0 };
	return { core->m_mousePosX, core->m_mousePosY };
}

DNAPI MouseState GetMouseState()
{
	if (core == nullptr) return {};

	MouseState ms;
	ms.x = core->m_mousePosX;
	ms.y = core->m_mousePosY;

	ms.leftHeld = core->m_mouseButtons[0];
	ms.rightHeld = core->m_mouseButtons[1];
	ms.middleHeld = core->m_mouseButtons[2];

	ms.leftPressed = core->m_mousePressed[0];
	ms.rightPressed = core->m_mousePressed[1];
	ms.middlePressed = core->m_mousePressed[2];

	ms.leftReleased = core->m_mouseReleased[0];
	ms.rightReleased = core->m_mouseReleased[1];
	ms.middleReleased = core->m_mouseReleased[2];

	ms.wheelDelta = core->m_mouseWheelDelta;

	return ms;
}

DNAPI void ShowConsoleCursor(bool visible)
{
#if defined(_WIN32)
	HANDLE h = (core && core->d_Console != INVALID_HANDLE_VALUE)
		? core->d_Console
		: GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO cci;
	GetConsoleCursorInfo(h, &cci);
	cci.bVisible = visible ? TRUE : FALSE;
	SetConsoleCursorInfo(h, &cci);
#else
	std::printf(visible ? "\e[?25h" : "\e[?25l");
	std::fflush(stdout);
#endif
}

// Utility functions 

DNAPI float GetElapsedTime()
{
	if (core == nullptr) return 0.0f;
	return core->elapsedTime;
}

DNAPI int GetScreenWidth()
{
	if (core == nullptr) return 0;
	return core->screenWidth;
}

DNAPI int GetScreenHeight()
{
	if (core == nullptr) return 0;
	return core->screenHeight;
}

DNAPI int GetRandomValue(int min, int max)
{
	if (max <= min) return min;
	return min + std::rand() % (max - min + 1);
}

DNAPI int SetRandomSeed(unsigned int seed)
{
	if (seed <= 0) return seed;
	return rand() % seed;
}

// Texture 


DNAPI Texture CreateTexture(int width, int height)
{
	Texture tex;
	tex.width = width;
	tex.height = height;
	tex.pixels = new unsigned short[width * height]();
	return tex;
}

DNAPI void DestroyTexture(Texture& tex)
{
	delete[] tex.pixels;
	tex.pixels = nullptr;
	tex.width = 0;
	tex.height = 0;
}

DNAPI void SetTexPixel(Texture& tex, int x, int y, unsigned short color)
{
	if (x < 0 || x >= tex.width || y < 0 || y >= tex.height) return;
	tex.pixels[y * tex.width + x] = color;
}

DNAPI unsigned short GetTexPixel(const Texture& tex, int x, int y)
{
	if (x < 0 || x >= tex.width || y < 0 || y >= tex.height) return 0;
	return tex.pixels[y * tex.width + x];
}

DNAPI unsigned short SampleTexture(const Texture& tex, float u, float v)
{
	u -= floorf(u);
	v -= floorf(v);

	int tx = (int)(u * tex.width);
	int ty = (int)(v * tex.height);

	if (tx < 0) tx = 0; else if (tx >= tex.width)  tx = tex.width - 1;
	if (ty < 0) ty = 0; else if (ty >= tex.height) ty = tex.height - 1;

	return tex.pixels[ty * tex.width + tx];
}

DNAPI bool SaveTexture(const Texture& tex, const std::wstring& path)
{
	FILE* f = nullptr;
#if defined(_WIN32)
	_wfopen_s(&f, path.c_str(), L"wb");
#else
	f = fopen(std::string(path.begin(), path.end()).c_str(), "wb");
#endif
	if (!f) return false;

	const char magic[4] = { 'D', 'N', 'T', 'X' };
	fwrite(magic, 1, 4, f);
	fwrite(&tex.width, sizeof(int), 1, f);
	fwrite(&tex.height, sizeof(int), 1, f);
	fwrite(tex.pixels, sizeof(unsigned short), tex.width * tex.height, f);
	fclose(f);
	return true;
}

DNAPI bool LoadTexture(const std::wstring& path, Texture& outTex)
{
	FILE* f = nullptr;
#if defined(_WIN32)
	_wfopen_s(&f, path.c_str(), L"rb");
#else
	f = fopen(std::string(path.begin(), path.end()).c_str(), "rb");
#endif
	if (!f) return false;

	char magic[4] = {};
	fread(magic, 1, 4, f);
	if (magic[0] != 'D' || magic[1] != 'N' || magic[2] != 'T' || magic[3] != 'X')
	{
		fclose(f); return false;
	}

	int w = 0, h = 0;
	fread(&w, sizeof(int), 1, f);
	fread(&h, sizeof(int), 1, f);

	if (w <= 0 || h <= 0 || w > 4096 || h > 4096) { fclose(f); return false; }

	delete[] outTex.pixels;

	outTex.width = w;
	outTex.height = h;
	outTex.pixels = new unsigned short[w * h];
	fread(outTex.pixels, sizeof(unsigned short), w * h, f);
	fclose(f);
	return true;
}

//  Audio

static void WavLE(std::ofstream& f, uint32_t v, int bytes)
{
	f.write(reinterpret_cast<const char*>(&v), bytes);
}

DNAPI void CreateMusicFile(const char* filename, double bpm, double beat, int sr, const Music* notes, int noteCount, float volume)
{
	double BEAT = beat / bpm;
	int SR = sr;

	float  vol = (volume < 0.0f) ? 0.0f : (volume > 1.0f) ? 1.0f : volume;
	double amplitude = 15000.0 * vol;

	std::vector<int16_t> smp;
	smp.reserve(SR * 32 * 2);

	for (int j = 0; j < noteCount; j++)
	{
		const Music& n = notes[j];
		int N = (int)(SR * n.beats * BEAT);
		for (int i = 0; i < N; i++)
		{
			double t = (double)i / SR;
			double fade = (i > N * 0.88) ? (double)(N - i) / (N * 0.12) : 1.0;
			int16_t v = (n.hz > 0.0)
				? (int16_t)(amplitude * fade * std::sin(6.28318530717 * n.hz * t))
				: 0;
			smp.push_back(v);
			smp.push_back(v);
		}
	}

	uint32_t dsize = (uint32_t)(smp.size() * 2);
	std::ofstream f(filename, std::ios::binary);
	f.write("RIFF", 4);  WavLE(f, 36 + dsize, 4);
	f.write("WAVE", 4);  f.write("fmt ", 4);  WavLE(f, 16, 4);
	WavLE(f, 1, 2);      WavLE(f, 2, 2);      WavLE(f, SR, 4);
	WavLE(f, SR * 4, 4); WavLE(f, 4, 2);      WavLE(f, 16, 2);
	f.write("data", 4);  WavLE(f, dsize, 4);
	for (int16_t s : smp)
		f.write(reinterpret_cast<const char*>(&s), 2);
}

// Internal WAV loader 

static void* LoadWavBytes(const char* filename, uint32_t& fileSize)
{
#if defined(_WIN32)
	HANDLE file = CreateFileA(filename, GENERIC_READ, 0, nullptr,
		OPEN_EXISTING, 0, nullptr);
	if (file == INVALID_HANDLE_VALUE) return nullptr;
	DWORD sz = GetFileSize(file, nullptr);
	void* buf = HeapAlloc(GetProcessHeap(), 0, sz + 1);
	DWORD br = 0;
	ReadFile(file, buf, sz, &br, nullptr);
	CloseHandle(file);
	((uint8_t*)buf)[sz] = 0;
	fileSize = sz;
	return buf;
#else
	FILE* f = fopen(filename, "rb");
	if (!f) return nullptr;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	rewind(f);
	void* buf = malloc(sz + 1);
	fread(buf, 1, sz, f);
	fclose(f);
	((uint8_t*)buf)[sz] = 0;
	fileSize = (uint32_t)sz;
	return buf;
#endif
}

static void FreeWavBytes(void* buf)
{
#if defined(_WIN32)
	HeapFree(GetProcessHeap(), 0, buf);
#else
	free(buf);
#endif
}

static void ReapSFXThreads()
{
	std::lock_guard<std::mutex> lk(g_sfxMutex);

	for (auto& t : g_sfxThreads)
		if (t.joinable()) t.detach();
	g_sfxThreads.clear();
}

#if defined(_WIN32)

static void WASAPIPlay(const char* filename,
	const std::atomic<bool>* stopFlag,
	std::atomic<bool>* playingFlag)
{
	uint32_t fileSize = 0;
	void* fileBytes = LoadWavBytes(filename, fileSize);
	if (!fileBytes)
	{
		fprintf(stderr, "[DonutAPI] WASAPI: cannot open '%s'\n", filename);
		return;
	}

	Audio* wav = (Audio*)fileBytes;
	if (wav->riffId != 0x46464952u || wav->waveId != 0x45564157u ||
		wav->formatCode != 1 || wav->numChannels != 2 ||
		wav->sampleRate != 44100 || wav->bitsPerSample != 16)
	{
		fprintf(stderr, "[DonutAPI] WASAPI: unsupported WAV format in '%s'\n", filename);
		FreeWavBytes(fileBytes);
		return;
	}

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr)) { FreeWavBytes(fileBytes); return; }

	IMMDeviceEnumerator* devEnum = nullptr;
	CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator), (LPVOID*)&devEnum);

	IMMDevice* audioDev = nullptr;
	devEnum->GetDefaultAudioEndpoint(eRender, eConsole, &audioDev);
	devEnum->Release();

	IAudioClient* audioClient = nullptr;
	audioDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (LPVOID*)&audioClient);
	audioDev->Release();

	WAVEFORMATEX fmt = {};
	fmt.wFormatTag = WAVE_FORMAT_PCM;
	fmt.nChannels = 2;
	fmt.nSamplesPerSec = 44100;
	fmt.wBitsPerSample = 16;
	fmt.nBlockAlign = 4;
	fmt.nAvgBytesPerSec = 44100 * 4;

	REFERENCE_TIME bufDur = 10000000LL; // 1 second buffer
	DWORD flags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
	hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, bufDur, 0, &fmt, nullptr);
	if (FAILED(hr)) { audioClient->Release(); FreeWavBytes(fileBytes); CoUninitialize(); return; }

	IAudioRenderClient* renderClient = nullptr;
	audioClient->GetService(__uuidof(IAudioRenderClient), (LPVOID*)&renderClient);

	UINT32 bufFrames = 0;
	audioClient->GetBufferSize(&bufFrames);
	audioClient->Start();

	if (playingFlag) *playingFlag = true;

	uint32_t  totalSamples = wav->dataChunkSize / sizeof(uint16_t);
	uint16_t* wavSamples = wav->samples;
	int       samplePos = 0;

	// Write in chunks so stopFlag is checked regularly (~every 10 ms)
	const UINT32 CHUNK = 441; // 10 ms worth of frames at 44100 Hz

	while (samplePos < (int)totalSamples)
	{
		if (stopFlag && *stopFlag) break;

		UINT32 padding = 0;
		hr = audioClient->GetCurrentPadding(&padding);
		if (FAILED(hr)) break;

		UINT32 available = bufFrames - padding;
		if (available == 0) { Sleep(1); continue; }

		UINT32 samplesLeft = totalSamples - (UINT32)samplePos;
		UINT32 framesLeft = samplesLeft / 2;
		UINT32 framesToWrite = available < CHUNK ? available : CHUNK;
		framesToWrite = framesToWrite < framesLeft ? framesToWrite : framesLeft;
		if (framesToWrite == 0) break;

		int16_t* buf = nullptr;
		hr = renderClient->GetBuffer(framesToWrite, (BYTE**)&buf);
		if (FAILED(hr)) break;

		for (UINT32 i = 0; i < framesToWrite; ++i)
		{
			*buf++ = (int16_t)wavSamples[samplePos++]; // L
			*buf++ = (int16_t)wavSamples[samplePos++]; // R
		}
		renderClient->ReleaseBuffer(framesToWrite, 0);
	}

	// Let the hardware buffer drain before stopping 
	if (!stopFlag || !(*stopFlag)) Sleep(200);

	audioClient->Stop();
	renderClient->Release();
	audioClient->Release();
	FreeWavBytes(fileBytes);
	CoUninitialize();

	if (playingFlag) *playingFlag = false;
}

DNAPI void StopMusic()
{
	g_musicStop = true;
	if (g_musicThread.joinable()) g_musicThread.join();
	g_musicStop = false;
	g_musicPlaying = false;
}

DNAPI bool IsMusicPlaying()
{
	return g_musicPlaying.load();
}

DNAPI void PlayMusicFile(const char* filename, void** /*data*/, uint32_t* /*numBytesRead*/)
{
	StopMusic(); // stop any prior track
	g_musicStop = false;
	g_musicThread = std::thread([fn = std::string(filename)]()
		{
			WASAPIPlay(fn.c_str(), &g_musicStop, &g_musicPlaying);
		});
}

DNAPI void PlaySFX(const char* filename, float /*volume*/)
{
	// Reap finished threads first so the vector stays small
	ReapSFXThreads();

	std::lock_guard<std::mutex> lk(g_sfxMutex);
	g_sfxThreads.emplace_back([fn = std::string(filename)]()
		{
			WASAPIPlay(fn.c_str(), nullptr, nullptr); // no stop flag — plays to completion
		});
}

#elif defined(__linux__)

static void ALSAPlay(const char* filename,
	const std::atomic<bool>* stopFlag,
	std::atomic<bool>* playingFlag)
{
	uint32_t fileSize = 0;
	void* fileBytes = LoadWavBytes(filename, fileSize);
	if (!fileBytes)
	{
		fprintf(stderr, "[DonutAPI] ALSAPlay: cannot open '%s'\n", filename);
		return;
	}

	Audio* wav = (Audio*)fileBytes;
	if (wav->formatCode != 1 || wav->numChannels != 2 ||
		wav->sampleRate != 44100 || wav->bitsPerSample != 16)
	{
		fprintf(stderr, "[DonutAPI] ALSAPlay: unsupported WAV in '%s'\n", filename);
		free(fileBytes); return;
	}

	snd_pcm_t* handle = nullptr;
	if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		fprintf(stderr, "[DonutAPI] ALSAPlay: snd_pcm_open failed\n");
		free(fileBytes); return;
	}

	int err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED, wav->numChannels,
		wav->sampleRate, 1, 50000);
	if (err < 0)
	{
		snd_pcm_close(handle); free(fileBytes); return;
	}

	if (playingFlag) *playingFlag = true;

	uint32_t    totalFrames = wav->dataChunkSize / wav->blockAlign;
	uint32_t    written = 0;
	const int16_t* src = (const int16_t*)wav->samples;
	const uint32_t CHUNK = 1024; // frames per write (~23 ms)

	while (written < totalFrames)
	{
		if (stopFlag && *stopFlag) break;
		uint32_t rem = totalFrames - written;
		uint32_t chunk = rem < CHUNK ? rem : CHUNK;
		snd_pcm_sframes_t rc = snd_pcm_writei(handle, src + written * wav->numChannels, chunk);
		if (rc < 0) rc = snd_pcm_recover(handle, (int)rc, 0);
		if (rc > 0) written += (uint32_t)rc;
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	FreeWavBytes(fileBytes);
	if (playingFlag) *playingFlag = false;
}

DNAPI void StopMusic()
{
	g_musicStop = true;
	if (g_musicThread.joinable()) g_musicThread.join();
	g_musicStop = false;
	g_musicPlaying = false;
}

DNAPI bool IsMusicPlaying() { return g_musicPlaying.load(); }

DNAPI void PlayMusicFile(const char* filename, void** /*data*/, uint32_t* /*numBytesRead*/)
{
	StopMusic();
	g_musicStop = false;
	g_musicThread = std::thread([fn = std::string(filename)]()
		{
			ALSAPlay(fn.c_str(), &g_musicStop, &g_musicPlaying);
		});
}

DNAPI void PlaySFX(const char* filename, float /*volume*/)
{
	ReapSFXThreads();
	std::lock_guard<std::mutex> lk(g_sfxMutex);
	g_sfxThreads.emplace_back([fn = std::string(filename)]()
		{
			ALSAPlay(fn.c_str(), nullptr, nullptr);
		});
}

#elif defined(__APPLE__)

struct AQPlayerState
{
	uint16_t* samples;
	uint32_t            numSamples;
	uint32_t            readPos;
	AudioQueueRef       queue;
	std::atomic<bool>   done{ false };
	const std::atomic<bool>* stopFlag;
};

static void AQCallback(void* ud, AudioQueueRef inAQ, AudioQueueBufferRef inBuf)
{
	AQPlayerState* s = (AQPlayerState*)ud;

	if (s->done || (s->stopFlag && *(s->stopFlag)))
	{
		s->done = true;
		AudioQueueStop(inAQ, false);
		return;
	}

	const uint32_t BPF = 4;
	uint32_t maxF = inBuf->mAudioDataBytesCapacity / BPF;
	int16_t* dst = (int16_t*)inBuf->mAudioData;
	uint32_t sampLeft = s->numSamples - s->readPos;
	uint32_t framesLeft = sampLeft / 2;
	uint32_t framesToFill = maxF < framesLeft ? maxF : framesLeft;

	if (framesToFill == 0) { s->done = true; AudioQueueStop(inAQ, false); return; }

	for (uint32_t i = 0; i < framesToFill; ++i)
	{
		*dst++ = (int16_t)s->samples[s->readPos++];
		*dst++ = (int16_t)s->samples[s->readPos++];
	}
	inBuf->mAudioDataByteSize = framesToFill * BPF;
	AudioQueueEnqueueBuffer(inAQ, inBuf, 0, nullptr);
}

static void AQPlay(const char* filename,
	const std::atomic<bool>* stopFlag,
	std::atomic<bool>* playingFlag)
{
	uint32_t fileSize = 0;
	void* fileBytes = LoadWavBytes(filename, fileSize);
	if (!fileBytes) return;

	Audio* wav = (Audio*)fileBytes;

	AudioStreamBasicDescription fmt = {};
	fmt.mFormatID = kAudioFormatLinearPCM;
	fmt.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
	fmt.mSampleRate = wav->sampleRate;
	fmt.mChannelsPerFrame = wav->numChannels;
	fmt.mBitsPerChannel = wav->bitsPerSample;
	fmt.mFramesPerPacket = 1;
	fmt.mBytesPerFrame = wav->blockAlign;
	fmt.mBytesPerPacket = wav->blockAlign;

	AQPlayerState state;
	state.samples = wav->samples;
	state.numSamples = wav->dataChunkSize / sizeof(uint16_t);
	state.readPos = 0;
	state.stopFlag = stopFlag;

	AudioQueueNewOutput(&fmt, AQCallback, &state, nullptr, nullptr, 0, &state.queue);

	const uint32_t FRAMES = 4096;
	const uint32_t BUFSZ = FRAMES * wav->blockAlign;
	AudioQueueBufferRef bufs[3];
	for (int i = 0; i < 3; ++i)
	{
		AudioQueueAllocateBuffer(state.queue, BUFSZ, &bufs[i]);
		AQCallback(&state, state.queue, bufs[i]);
	}

	if (playingFlag) *playingFlag = true;
	AudioQueueStart(state.queue, nullptr);

	while (!state.done) usleep(5000);

	usleep(100000);
	AudioQueueDispose(state.queue, true);
	FreeWavBytes(fileBytes);
	if (playingFlag) *playingFlag = false;
}

DNAPI void StopMusic()
{
	g_musicStop = true;
	if (g_musicThread.joinable()) g_musicThread.join();
	g_musicStop = false;
	g_musicPlaying = false;
}

DNAPI bool IsMusicPlaying() { return g_musicPlaying.load(); }

DNAPI void PlayMusicFile(const char* filename, void** /*data*/, uint32_t* /*numBytesRead*/)
{
	StopMusic();
	g_musicStop = false;
	g_musicThread = std::thread([fn = std::string(filename)]()
		{
			AQPlay(fn.c_str(), &g_musicStop, &g_musicPlaying);
		});
}

DNAPI void PlaySFX(const char* filename, float /*volume*/)
{
	ReapSFXThreads();
	std::lock_guard<std::mutex> lk(g_sfxMutex);
	g_sfxThreads.emplace_back([fn = std::string(filename)]()
		{
			AQPlay(fn.c_str(), nullptr, nullptr);
		});
}

#else

DNAPI void StopMusic() {}
DNAPI bool IsMusicPlaying() { return false; }
DNAPI void PlayMusicFile(const char* filename, void** /*data*/, uint32_t* /*numBytesRead*/)
{
	fprintf(stderr, "[DonutAPI] APIPlay: audio not supported on this platform.\n");
	(void)filename;
}
DNAPI void PlaySFX(const char* filename, float /*volume*/)
{
	fprintf(stderr, "[DonutAPI] PlaySFX: audio not supported on this platform.\n");
	(void)filename;
}

#endif


//  Sprite Billboard Rendering


DNAPI void DrawBillboard
(
	const double* zBuffer,
	double worldX, double worldY,
	double dirX, double dirY,
	double planeX, double planeY,
	double posX, double posY,
	const Texture& sprite,
	float scale
)
{
	if (core == nullptr || zBuffer == nullptr || sprite.pixels == nullptr) return;

	int sw = core->screenWidth;
	int sh = core->screenHeight;

	double sprX = worldX - posX;
	double sprY = worldY - posY;

	double invDet = 1.0 / (planeX * dirY - dirX * planeY);

	double transformX = invDet * (dirY * sprX - dirX * sprY);
	double transformY = invDet * (-planeY * sprX + planeX * sprY);

	if (transformY <= 0.0) return;

	int sprScreenX = (int)((sw / 2) * (1.0 + transformX / transformY));

	int sprH = std::abs((int)(sh / transformY * scale));
	int sprW = sprH;

	int drawStartY = std::max(0, sh / 2 - sprH / 2);
	int drawEndY = std::min(sh - 1, sh / 2 + sprH / 2);

	int drawStartX = std::max(0, sprScreenX - sprW / 2);
	int drawEndX = std::min(sw - 1, sprScreenX + sprW / 2);

	for (int stripe = drawStartX; stripe <= drawEndX; stripe++)
	{
		if (transformY >= zBuffer[stripe]) continue;

		float u = (float)(stripe - (sprScreenX - sprW / 2)) / (float)sprW;

		for (int y = drawStartY; y <= drawEndY; y++)
		{
			float v = (float)(y - drawStartY) / (float)(drawEndY - drawStartY + 1);

			unsigned short col = SampleTexture(sprite, u, v);

			if (col == 0) continue;

			if (transformY > 9.0)       col = 0;
			else if (transformY > 5.0)  col = (col >= 8) ? (unsigned short)(col - 8) : 0;

			if (col != 0)
			{
				int idx = y * sw + stripe;
				core->screenBuffer[idx].Char.UnicodeChar = PIXEL_SOLID;
				core->screenBuffer[idx].Attributes = col;
			}
		}
	}
}