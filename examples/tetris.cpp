#include "DonutAPI.h"

#include <vector>
#include <cstring>
#include <algorithm>

const int WIDTH = 78; 
const int HEIGHT = 42; 
const int FONT_W = 10;
const int FONT_H = 18;

const int COLS = 10;  // board columns
const int ROWS = 20;  // board rows
const int CW = 3;     // pixels per cell (width)
const int CH = 2;     // pixels per cell (height)

// Board top-left pixel
const int BOFF_X = 24;
const int BOFF_Y = 1;

// Derived layout
const int BOARD_PX_W = COLS * CW;          // 30
const int BOARD_PX_H = ROWS * CH;          // 40
const int BORDER_L = BOFF_X - 1;           // 23
const int BORDER_R = BOFF_X + BOARD_PX_W;  // 54
const int BORDER_B = BOFF_Y + BOARD_PX_H;  // 41
const int RIGHT_X = BORDER_R + 3;          // 57 — right panel

enum PieceType : int { I = 0, O = 1, T = 2, S = 3, Z = 4, J = 5, L = 6, NUM_PIECES = 7 };

//  [piece][rotation][row][col]  — each piece lives in a 4×4 grid
const int SHAPES[NUM_PIECES][4][4][4] =
{
  { // I — cyan
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
    {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
    {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}},
  },
  { // O — yellow
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
  },
  { // T — magenta
    {{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,0,0},{0,1,1,0},{0,1,0,0}},
    {{0,0,0,0},{0,0,0,0},{1,1,1,0},{0,1,0,0}},
    {{0,0,0,0},{0,1,0,0},{1,1,0,0},{0,1,0,0}},
  },
  { // S — green
    {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,0,0},{0,1,1,0},{0,0,1,0}},
    {{0,0,0,0},{0,0,0,0},{0,1,1,0},{1,1,0,0}},
    {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
  },
  { // Z — red
    {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,0,1,0},{0,1,1,0},{0,1,0,0}},
    {{0,0,0,0},{0,0,0,0},{1,1,0,0},{0,1,1,0}},
    {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}},
  },
  { // J — blue
    {{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,1,0},{0,1,0,0},{0,1,0,0}},
    {{0,0,0,0},{0,0,0,0},{1,1,1,0},{0,0,1,0}},
    {{0,0,0,0},{0,1,0,0},{0,1,0,0},{1,1,0,0}},
  },
  { // L — white (closest available to orange in 4-bit console palette)
    {{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,0,0},{0,1,0,0},{0,1,1,0}},
    {{0,0,0,0},{0,0,0,0},{1,1,1,0},{1,0,0,0}},
    {{0,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,0,0}},
  },
};

//  [piece][0]=face, [1]=highlight, [2]=shadow
const unsigned short PCOLORS[NUM_PIECES][3] =
{
  { CYAN,    WHITE, DARK_CYAN    },   // I
  { YELLOW,  WHITE, BROWN        },   // O
  { MAGENTA, WHITE, DARK_MAGENTA },   // T
  { GREEN,   WHITE, DARK_GREEN   },   // S
  { RED,     WHITE, DARK_RED     },   // Z
  { BLUE,    WHITE, DARK_BLUE    },   // J
  { WHITE,   WHITE, GRAY         },   // L
};

Texture g_tex[NUM_PIECES];

void BuildTextures()
{
    for (int p = 0; p < NUM_PIECES; p++)
    {
        g_tex[p] = CreateTexture(CW, CH);

        unsigned short face = PCOLORS[p][0];
        unsigned short hi = PCOLORS[p][1];
        unsigned short shade = PCOLORS[p][2];

        for (int ty = 0; ty < CH; ty++)
            for (int tx = 0; tx < CW; tx++)
            {
                unsigned short c;
                if (tx == 0 && ty == 0)            c = hi;
                else if (tx == CW - 1 || ty == CH - 1)      c = shade;
                else                                     c = face;
                SetTexPixel(g_tex[p], tx, ty, c);
            }
    }
}

void FreeTextures()
{
    for (int p = 0; p < NUM_PIECES; p++)
        DestroyTexture(g_tex[p]);
}

int  g_board[ROWS][COLS];   // 0=empty, (PieceType+1)=placed block

int  g_score = 0;
int  g_level = 1;
int  g_lines = 0;
int  g_hiScore = 0;

bool g_started = false;
bool g_paused = false;
bool g_gameOver = false;
bool g_musicOn = true;
bool g_playedGameOverSFX = false;

// Active piece
PieceType g_type;
int g_rot;
int g_px;    // board column  (may be 0-indexed negative briefly)
int g_py;    // board row

// Hold
int g_holdType = -1;    // -1 = empty
bool g_holdUsed = false;

// Next queue (3 visible)
PieceType g_next[3];

// Timers  (all in seconds)

float g_dropT = 0.f;    // gravity accumulator
float g_lockT = 0.f;    // lock-delay accumulator
float g_dasT = 0.f;    // Delayed Auto Shift
float g_arrT = 0.f;    // Auto-Repeat Rate
float g_sdT = 0.f;    // soft-drop accumulator

bool g_moveL = false;  // DAS direction flags
bool g_moveR = false;

const float DAS = 0.170f;   // seconds before auto-repeat starts
const float ARR = 0.050f;   // auto-repeat interval
const float LOCK_DELAY = 0.500f;   // seconds before locking
const float SOFT_RATE = 0.050f;   // soft-drop step interval

// Music
const Music g_theme[] =
{
    // ── Section A 
    {659.25,1.0},{493.88,0.5},{523.25,0.5},
    {587.33,1.0},{523.25,0.5},{493.88,0.5},
    {440.00,1.0},{440.00,0.5},{523.25,0.5},
    {659.25,1.0},{587.33,0.5},{523.25,0.5},
    {493.88,1.5},{523.25,0.5},{587.33,1.0},{659.25,1.0},
    {523.25,1.0},{440.00,1.0},{440.00,2.0},
    // ── Section B 
    {0,0.5},
    {587.33,1.5},{698.46,0.5},
    {880.00,1.0},{783.99,0.5},{698.46,0.5},
    {659.25,1.5},{523.25,0.5},{659.25,1.0},
    {587.33,0.5},{523.25,0.5},
    {493.88,1.5},{523.25,0.5},{587.33,1.0},{659.25,1.0},
    {523.25,1.0},{440.00,1.0},{440.00,1.0},{0,1.0},
    // ── Section A (repeat)
    {659.25,1.0},{493.88,0.5},{523.25,0.5},
    {587.33,1.0},{523.25,0.5},{493.88,0.5},
    {440.00,1.0},{440.00,0.5},{523.25,0.5},
    {659.25,1.0},{587.33,0.5},{523.25,0.5},
    {493.88,1.5},{523.25,0.5},{587.33,1.0},{659.25,1.0},
    {523.25,1.0},{440.00,1.0},{440.00,2.0},
    // ── Section C (bridge) 
    {0,0.5},
    {329.63,1.5},{392.00,0.5},
    {523.25,1.0},{493.88,0.5},{440.00,0.5},
    {392.00,2.0},{261.63,0.5},{329.63,0.5},
    {392.00,1.0},{440.00,1.0},{523.25,1.0},{0,1.0},
    {392.00,1.5},{440.00,0.5},{493.88,1.0},
    {523.25,0.5},{440.00,0.5},{392.00,1.0},{329.63,2.0},
};
const int g_themeCount = (int)(sizeof(g_theme) / sizeof(Music));

// SFX note tables
const Music g_sfxClear[] = { {523.25,0.10},{659.25,0.10},{783.99,0.10},{1046.50,0.18} };
const Music g_sfxTetris[] = { {523.25,0.08},{659.25,0.08},{783.99,0.08},{1046.50,0.08},{1318.51,0.28} };
const Music g_sfxLock[] = { {200.00,0.06} };
const Music g_sfxHold[] = { {440.00,0.05},{880.00,0.05} };
const Music g_sfxOver[] = { {440.00,0.35},{370.00,0.35},{311.13,0.35},{261.63,0.70} };
const Music g_sfxRotate[] = { {660.00,0.04} };

void BuildAudio()
{
    // Build a long looped WAV (~10 passes ≈ several minutes of music)
    const int reps = 10;
    std::vector<Music> loop;
    loop.reserve(g_themeCount * reps);
    for (int i = 0; i < reps; i++)
        for (int j = 0; j < g_themeCount; j++)
            loop.push_back(g_theme[j]);

    CreateMusicFile("tetris_theme.wav", 160.0, 60.0, 44100,
        loop.data(), (int)loop.size(), 0.55f);

    CreateMusicFile("sfx_clear.wav", 200.0, 60.0, 44100, g_sfxClear, 4, 0.50f);
    CreateMusicFile("sfx_tetris.wav", 200.0, 60.0, 44100, g_sfxTetris, 5, 0.60f);
    CreateMusicFile("sfx_lock.wav", 300.0, 60.0, 44100, g_sfxLock, 1, 0.30f);
    CreateMusicFile("sfx_hold.wav", 200.0, 60.0, 44100, g_sfxHold, 2, 0.35f);
    CreateMusicFile("sfx_over.wav", 80.0, 60.0, 44100, g_sfxOver, 4, 0.50f);
    CreateMusicFile("sfx_rotate.wav", 400.0, 60.0, 44100, g_sfxRotate, 1, 0.20f);
}

unsigned g_rng = 0xBEEFCAFEu;
int RandPiece()
{
    g_rng = g_rng * 1664525u + 1013904223u;
    return (int)((g_rng >> 16) & 0x7FFF) % NUM_PIECES;
}

bool Collides(PieceType t, int rot, int px, int py)
{
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
        {
            if (!SHAPES[t][rot][r][c]) continue;
            int bx = px + c, by = py + r;
            if (bx < 0 || bx >= COLS || by >= ROWS) return true;
            if (by >= 0 && g_board[by][bx])          return true;
        }
    return false;
}

int GhostY()
{
    int gy = g_py;
    while (!Collides(g_type, g_rot, g_px, gy + 1)) gy++;
    return gy;
}

float DropInterval()
{
    // NES-style gravity table (seconds per row)
    const float spd[] =
    {
        0.800f,0.717f,0.633f,0.550f,0.467f,
        0.383f,0.300f,0.217f,0.133f,0.100f,
        0.083f,0.083f,0.083f,0.067f,0.067f,
        0.067f,0.050f,0.050f,0.050f,0.033f,
    };
    return spd[std::min(g_level - 1, 19)];
}

PieceType PopNext()
{
    PieceType top = g_next[0];
    g_next[0] = g_next[1];
    g_next[1] = g_next[2];
    g_next[2] = (PieceType)RandPiece();
    return top;
}

// Forward declaration
void LockPiece();

bool Spawn(PieceType t)
{
    g_type = t;
    g_rot = 0;
    g_px = 3;
    g_py = 0;
    g_holdUsed = false;
    g_lockT = 0.f;
    g_dropT = 0.f;

    if (Collides(g_type, g_rot, g_px, g_py))
    {
        g_gameOver = true;
        return false;
    }
    return true;
}

void InitGame()
{
    memset(g_board, 0, sizeof(g_board));
    g_score = 0; g_level = 1; g_lines = 0;
    g_gameOver = false; g_paused = false;
    g_holdType = -1; g_holdUsed = false;
    g_playedGameOverSFX = false;
    g_dropT = g_lockT = g_dasT = g_arrT = g_sdT = 0.f;
    g_moveL = g_moveR = false;

    for (int i = 0; i < 3; i++)
        g_next[i] = (PieceType)RandPiece();

    Spawn(PopNext());
}

bool Rotate(int dir)
{
    int nr = (g_rot + dir + 4) % 4;
    // SRS wall kick attempts: 0,0  -1,0  +1,0  0,-1  -2,0  +2,0
    const int kicks[6][2] = { {0,0},{-1,0},{1,0},{0,-1},{-2,0},{2,0} };
    for (auto& k : kicks)
    {
        if (!Collides(g_type, nr, g_px + k[0], g_py + k[1]))
        {
            g_px += k[0];
            g_py += k[1];
            g_rot = nr;
            g_lockT = 0.f;
            PlaySFX("sfx_rotate.wav", 0.2f);
            return true;
        }
    }
    return false;
}

void HardDrop()
{
    int gy = GhostY();
    g_score += (gy - g_py) * 2;
    g_py = gy;
    LockPiece();
}

void DoHold()
{
    if (g_holdUsed) return;
    g_holdUsed = true;
    PlaySFX("sfx_hold.wav", 0.35f);

    if (g_holdType < 0)
    {
        g_holdType = (int)g_type;
        Spawn(PopNext());
    }
    else
    {
        int tmp = g_holdType;
        g_holdType = (int)g_type;
        Spawn((PieceType)tmp);
    }
}

void LockPiece()
{
    // Write piece to board
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
        {
            if (!SHAPES[g_type][g_rot][r][c]) continue;
            int bx = g_px + c, by = g_py + r;
            if (by >= 0 && by < ROWS && bx >= 0 && bx < COLS)
                g_board[by][bx] = (int)g_type + 1;
        }

    PlaySFX("sfx_lock.wav", 0.3f);

    // Clear completed rows (scan bottom-up, re-check after each shift)
    int cleared = 0;
    for (int r = ROWS - 1; r >= 0; r--)
    {
        bool full = true;
        for (int c = 0; c < COLS; c++) { if (!g_board[r][c]) { full = false; break; } }
        if (!full) continue;

        for (int rr = r; rr > 0; rr--)
            memcpy(g_board[rr], g_board[rr - 1], sizeof(int) * COLS);
        memset(g_board[0], 0, sizeof(int) * COLS);
        cleared++;
        r++;  // re-check same index after shift
    }

    if (cleared > 0)
    {
        const int pts[] = { 0, 100, 300, 500, 800 };
        g_score += pts[std::min(cleared, 4)] * g_level;
        g_lines += cleared;
        g_level = std::max(1, g_lines / 10 + 1);
        if (g_score > g_hiScore) g_hiScore = g_score;
        PlaySFX(cleared == 4 ? "sfx_tetris.wav" : "sfx_clear.wav",
            cleared == 4 ? 0.6f : 0.5f);
    }

    Spawn(PopNext());
}

//  Draw one Tetris cell at board column/row using its per-piece texture
void DrawCell(int col, int row, PieceType type)
{
    int sx = BOFF_X + col * CW;
    int sy = BOFF_Y + row * CH;
    for (int ty = 0; ty < CH; ty++)
        for (int tx = 0; tx < CW; tx++)
        {
            unsigned short color = GetTexPixel(g_tex[type], tx, ty);
            //  Bottom row and right column rendered as three-quarters for depth
            short pix = (ty == CH - 1 || tx == CW - 1) ? PIXEL_THREEQUARTERS : PIXEL_SOLID;
            DrawPixel(sx + tx, sy + ty, pix, color);
        }
}

//  Ghost piece: quarter-filled using the piece's shadow colour
void DrawGhostCell(int col, int row, PieceType type)
{
    int sx = BOFF_X + col * CW;
    int sy = BOFF_Y + row * CH;
    for (int ty = 0; ty < CH; ty++)
        for (int tx = 0; tx < CW; tx++)
            DrawPixel(sx + tx, sy + ty, PIXEL_QUARTER, PCOLORS[type][2]);
}

//  Mini-piece preview (2-wide × 1-tall per cell), placed at absolute screen coords
void DrawMiniPiece(int scrX, int scrY, PieceType type, bool dimmed = false)
{
    const auto& shape = SHAPES[type][0];
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
        {
            if (!shape[r][c]) continue;
            unsigned short face = dimmed ? DARK_GRAY : PCOLORS[type][0];
            unsigned short shade = dimmed ? DARK_GRAY : PCOLORS[type][2];
            DrawPixel(scrX + c * 2, scrY + r, PIXEL_SOLID, face);
            DrawPixel(scrX + c * 2 + 1, scrY + r, PIXEL_THREEQUARTERS, shade);
        }
}

void DrawBoard()
{
    // Black background
    Fill(BOFF_X, BOFF_Y, BOFF_X + BOARD_PX_W, BOFF_Y + BOARD_PX_H, PIXEL_SOLID, BLACK);

    // Subtle grid overlay: dotted horizontal lines every 4 rows
    for (int r = 4; r < ROWS; r += 4)
        DrawLine(BOFF_X, BOFF_Y + r * CH, BOFF_X + BOARD_PX_W - 1, BOFF_Y + r * CH,
            PIXEL_QUARTER, DARK_GRAY);

    // Placed blocks
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (g_board[r][c])
                DrawCell(c, r, (PieceType)(g_board[r][c] - 1));

    if (g_gameOver || g_paused) return;

    // Ghost piece (only when it differs from actual position)
    int gy = GhostY();
    if (gy != g_py)
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
            {
                if (!SHAPES[g_type][g_rot][r][c]) continue;
                int brow = gy + r;
                if (brow >= 0 && brow < ROWS)
                    DrawGhostCell(g_px + c, brow, g_type);
            }

    // Active piece
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
        {
            if (!SHAPES[g_type][g_rot][r][c]) continue;
            int brow = g_py + r;
            if (brow >= 0 && brow < ROWS)
                DrawCell(g_px + c, brow, g_type);
        }
}

void DrawBorders()
{
    // Left and right walls
    for (int r = BOFF_Y; r <= BORDER_B; r++)
    {
        DrawPixel(BORDER_L, r, PIXEL_SOLID, GRAY);
        DrawPixel(BORDER_R, r, PIXEL_SOLID, GRAY);
    }
    // Floor
    for (int c = BORDER_L; c <= BORDER_R; c++)
        DrawPixel(c, BORDER_B, PIXEL_SOLID, GRAY);
}

// Helpers to avoid swprintf repetition
void DrawNum(int x, int y, int val, unsigned short col)
{
    wchar_t buf[24];
    swprintf(buf, 24, L"%d", val);
    DrawString(x, y, buf, col);
}

void DrawLeftPanel()
{
    const int lx = 1;

    DrawString(lx, 1, L"SCORE", DARK_GRAY);
    DrawNum(lx, 2, g_score, YELLOW);

    DrawString(lx, 4, L"HI-SCORE", DARK_GRAY);
    DrawNum(lx, 5, g_hiScore, CYAN);

    DrawString(lx, 7, L"LEVEL", DARK_GRAY);
    DrawNum(lx, 8, g_level, GREEN);

    DrawString(lx, 10, L"LINES", DARK_GRAY);
    DrawNum(lx, 11, g_lines, MAGENTA);

    DrawString(lx, 14, L"HOLD", DARK_GRAY);
    DrawLine(lx, 15, lx + 11, 15, PIXEL_SOLID, DARK_GRAY);
    DrawLine(lx, 21, lx + 11, 21, PIXEL_SOLID, DARK_GRAY);
    DrawLine(lx, 15, lx, 21, PIXEL_SOLID, DARK_GRAY);
    DrawLine(lx + 11, 15, lx + 11, 21, PIXEL_SOLID, DARK_GRAY);

    if (g_holdType >= 0)
        DrawMiniPiece(lx + 1, 16, (PieceType)g_holdType, g_holdUsed);
    else
        DrawString(lx + 2, 18, L"----", DARK_GRAY);

    if (!g_gameOver && !g_paused && g_lockT > 0.f)
    {
        DrawString(lx, 23, L"LOCK", DARK_GRAY);
        int filled = (int)(10.f * std::min(g_lockT / LOCK_DELAY, 1.0f));
        for (int i = 0; i < 10; i++)
            DrawPixel(lx + i, 24, PIXEL_SOLID, i < filled ? RED : DARK_GRAY);
    }
}

void DrawRightPanel()
{
    const int rx = RIGHT_X;

    DrawString(rx, 1, L"NEXT", DARK_GRAY);

    for (int qi = 0; qi < 3; qi++)
    {
        int oy = 3 + qi * 7;
        DrawMiniPiece(rx, oy, g_next[qi], qi > 0);
        if (qi < 2)
            DrawLine(rx, oy + 5, rx + 10, oy + 5, PIXEL_QUARTER, DARK_GRAY);
    }

    DrawString(rx, 26, L"CONTROLS", DARK_GRAY);
#ifdef _WIN32
    DrawString(rx, 27, L"\x2190\x2192 MOVE", DARK_GRAY);
    DrawString(rx, 28, L"\x2191  ROTATE", DARK_GRAY);
    DrawString(rx, 29, L"\x2193  SOFT DROP", DARK_GRAY);
#else
    DrawString(rx, 27, L"A/D  MOVE", DARK_GRAY);
    DrawString(rx, 28, L"W    ROTATE", DARK_GRAY);
    DrawString(rx, 29, L"S    SOFT DROP", DARK_GRAY);
#endif
    DrawString(rx, 30, L"SPC  HARD DROP", DARK_GRAY);
    DrawString(rx, 31, L"C    HOLD", DARK_GRAY);
    DrawString(rx, 32, L"P    PAUSE", DARK_GRAY);
    DrawString(rx, 33, L"M    MUSIC", DARK_GRAY);
#ifdef _WIN32
    DrawString(rx, 34, L"ESC  QUIT", DARK_GRAY);
#else
    DrawString(rx, 34, L"Q    QUIT", DARK_GRAY);
#endif

    // Music on/off indicator
    unsigned short mc = g_musicOn ? CYAN : DARK_GRAY;
    DrawString(rx, 36, g_musicOn ? L"\x266B MUSIC ON " : L"\x266A MUSIC OFF", mc);
}

// Block-art title letters — each drawn as coloured filled rectangles
void DrawTitle()
{
    //  6-wide × 5-tall letters, 2-char gap between them
    int tx = 8, ty = 4;
    const int LW = 6, LH = 5, GAP = 2;

    auto B = [&](int dx, int dy, int w, int h, unsigned short c)
        {
            Fill(tx + dx, ty + dy, tx + dx + w, ty + dy + h, PIXEL_SOLID, c);
        };

    // T
    B(0, 0, LW, 1, CYAN); B(2, 1, 2, 4, CYAN);
    tx += LW + GAP;

    // E
    B(0, 0, LW, 1, YELLOW); B(0, 1, 1, 2, YELLOW);
    B(0, 2, LW - 1, 1, YELLOW); B(0, 4, LW, 1, YELLOW); B(0, 3, 1, 1, YELLOW);
    tx += LW + GAP;

    // T
    B(0, 0, LW, 1, GREEN); B(2, 1, 2, 4, GREEN);
    tx += LW + GAP;

    // R
    B(0, 0, LW, 1, MAGENTA); B(0, 0, 1, LH, MAGENTA);
    B(1, 1, LW - 2, 1, MAGENTA); B(0, 2, LW - 1, 1, MAGENTA);
    B(2, 3, LW - 2, 2, MAGENTA);
    tx += LW + GAP;

    // I
    B(0, 0, LW, 1, RED); B(2, 1, 2, 3, RED); B(0, 4, LW, 1, RED);
    tx += LW + GAP;

    // S
    B(0, 0, LW, 1, BLUE); B(0, 1, 1, 1, BLUE); B(0, 2, LW, 1, BLUE);
    B(LW - 1, 3, 1, 1, BLUE); B(0, 4, LW, 1, BLUE);
}

void DrawTitleScreen()
{
    Fill(0, 0, WIDTH, HEIGHT, PIXEL_SOLID, BLACK);

    DrawTitle();

    // Decorative pieces around the title
    DrawMiniPiece(2, 2, I);
    DrawMiniPiece(2, 3, O);
    DrawMiniPiece(62, 2, T);
    DrawMiniPiece(62, 4, S);
    DrawMiniPiece(2, 5, Z);

    DrawString(24, 12, L"PRESS ENTER TO START", WHITE);
    DrawString(27, 14, L"ESC TO QUIT", GRAY);

    if (g_hiScore > 0)
    {
        wchar_t buf[32];
        swprintf(buf, 32, L"HI-SCORE: %d", g_hiScore);
        DrawString(28, 21, buf, CYAN);
    }
}

void DrawPauseOverlay()
{
    // Checkerboard dimmer over the board area
    for (int r = 0; r < BOARD_PX_H; r++)
        for (int c = 0; c < BOARD_PX_W; c++)
            if ((r + c) % 2 == 0)
                DrawPixel(BOFF_X + c, BOFF_Y + r, PIXEL_HALF, DARK_GRAY);

    int px = BOFF_X + 4, py = BOFF_Y + 9;
    DrawString(px, py, L"PAUSED", YELLOW);
    DrawString(px - 2, py + 2, L"P to resume", GRAY);
}

void DrawGameOverOverlay()
{
    // Solid dim over the board
    for (int r = 0; r < BOARD_PX_H; r++)
        for (int c = 0; c < BOARD_PX_W; c++)
            DrawPixel(BOFF_X + c, BOFF_Y + r, PIXEL_THREEQUARTERS, DARK_GRAY);

    int px = BOFF_X + 2, py = BOFF_Y + 6;
    DrawString(px, py, L"GAME OVER", RED);
    DrawString(px, py + 2, L"SCORE:", WHITE);
    DrawNum(px + 8, py + 2, g_score, YELLOW);
    DrawString(px, py + 4, L"BEST: ", WHITE);
    DrawNum(px + 8, py + 4, g_hiScore, CYAN);
    DrawString(px - 1, py + 7, L"ENTER=RETRY", WHITE);
}

int main()
{
    InitWindow(WIDTH, HEIGHT, FONT_W, FONT_H);

    SetWindowName(L"TETRIS");
    SetFPS(60);
    ShowConsoleCursor(false);

    BuildTextures();
    BuildAudio();     // Generates all WAV files to disk on first run

    while (!WindowShouldClose())
    {
        float dt = GetElapsedTime();
        if (dt <= 0.f || dt > 0.25f) dt = 0.016f;  // clamp wild deltas
        GetKeyState();

        if (!g_started)
        {
            DrawTitleScreen();

            if (GetKey(KEY_ENTER).k_Pressed)
            {
                g_started = true;
                InitGame();
                if (g_musicOn) PlayMusicFile("tetris_theme.wav", nullptr, nullptr);
            }

            UpdateScreen();
            continue;
        }

        if (g_gameOver)
        {
            if (GetKey(KEY_ENTER).k_Pressed)
            {
                g_playedGameOverSFX = false;
                InitGame();
                if (g_musicOn) PlayMusicFile("tetris_theme.wav", nullptr, nullptr);
            }
        }
        else if (!g_paused)
        {
            // Rotate
            if (GetKey(KEY_UP).k_Pressed || GetKey(KEY_Z).k_Pressed)
                Rotate(1);

            // Hard drop
            if (GetKey(KEY_SPACE).k_Pressed)
                HardDrop();

            // Hold
            if (GetKey(KEY_C).k_Pressed)
                DoHold();

            if (GetKey(KEY_LEFT).k_Pressed)
            {
                if (!Collides(g_type, g_rot, g_px - 1, g_py))
                {
                    g_px--; g_lockT = 0.f;
                }
                g_moveL = true;  g_moveR = false;
                g_dasT = 0.f;   g_arrT = 0.f;
            }
            if (GetKey(KEY_RIGHT).k_Pressed)
            {
                if (!Collides(g_type, g_rot, g_px + 1, g_py))
                {
                    g_px++; g_lockT = 0.f;
                }
                g_moveR = true;  g_moveL = false;
                g_dasT = 0.f;   g_arrT = 0.f;
            }
            if (GetKey(KEY_LEFT).k_Released)  g_moveL = false;
            if (GetKey(KEY_RIGHT).k_Released) g_moveR = false;

            if (g_moveL || g_moveR)
            {
                g_dasT += dt;
                if (g_dasT >= DAS)
                {
                    g_arrT += dt;
                    while (g_arrT >= ARR)
                    {
                        g_arrT -= ARR;
                        int dx = g_moveL ? -1 : 1;
                        if (!Collides(g_type, g_rot, g_px + dx, g_py))
                        {
                            g_px += dx; g_lockT = 0.f;
                        }
                    }
                }
            }

            if (GetKey(KEY_DOWN).k_Held)
            {
                g_sdT += dt;
                while (g_sdT >= SOFT_RATE)
                {
                    g_sdT -= SOFT_RATE;
                    if (!Collides(g_type, g_rot, g_px, g_py + 1))
                    {
                        g_py++; g_score++; g_dropT = 0.f; g_lockT = 0.f;
                    }
                }
            }
            else
            {
                g_sdT = 0.f;
            }

            g_dropT += dt;
            if (g_dropT >= DropInterval())
            {
                g_dropT = 0.f;
                if (!Collides(g_type, g_rot, g_px, g_py + 1))
                {
                    g_py++; g_lockT = 0.f;
                }
            }

            if (Collides(g_type, g_rot, g_px, g_py + 1))
            {
                g_lockT += dt;
                if (g_lockT >= LOCK_DELAY) LockPiece();
            }
            else
            {
                g_lockT = 0.f;
            }
        }

        if (!g_gameOver && GetKey(KEY_P).k_Pressed)
            g_paused = !g_paused;

        if (GetKey(KEY_M).k_Pressed)  // K_M
        {
            g_musicOn = !g_musicOn;
            if (!g_musicOn)
                StopMusic();
            else if (!g_gameOver)
                PlayMusicFile("tetris_theme.wav", nullptr, nullptr);
        }

        if (g_gameOver && !g_playedGameOverSFX)
        {
            g_playedGameOverSFX = true;
            StopMusic();
            PlaySFX("sfx_over.wav", 0.5f);
        }

        Fill(0, 0, WIDTH, HEIGHT, PIXEL_SOLID, BLACK);
        DrawBorders();
        DrawBoard();
        DrawLeftPanel();
        DrawRightPanel();
        if (g_paused)   DrawPauseOverlay();
        if (g_gameOver) DrawGameOverOverlay();

        UpdateScreen();
    }

    StopMusic();
    FreeTextures();
    DestroyWindow();
    return 0;
}