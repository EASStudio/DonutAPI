// ASTEROIDS  --  DonutAPI Edition 

#include "DonutAPI.h"
#include "DonutMath.h"

#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

const int WIDTH = 200;  
const int HEIGHT = 100; 
const int FONT_W = 8;
const int FONT_H = 8; 

const float TURN_SPEED = 2.8f;     // rad/sec
const float THRUST_POWER = 22.0f;  // units/sec^2
const float MAX_SPEED = 28.0f;
const float DRAG_PER_SEC = 0.985f; // applied 60x / sec
const float BULLET_SPEED = 55.0f;
const float BULLET_LIFE = 1.4f;
const int   MAX_BULLETS = 5;
const float INV_TIME = 3.0f;
const float RESPAWN_WAIT = 2.0f;

const short PX = PIXEL_SOLID;

const Music theme[] =
{
    { 110.0, 1.0 },  // low thump  (A2)
    {  82.5, 1.0 },  // lower thump (E2)
    { 110.0, 1.0 },
    {  82.5, 1.0 },
    { 110.0, 1.0 },
    {  82.5, 1.0 },

    { 110.0, 1.0 },  // low thump  (A2)
    {  82.5, 1.0 },  // lower thump (E2)
    { 110.0, 1.0 },
    {  82.5, 1.0 },
    { 110.0, 1.0 },
    {  82.5, 1.0 },

    { 110.0, 1.0 },  // low thump  (A2)
    {  82.5, 1.0 },  // lower thump (E2)
    { 110.0, 1.0 },
    {  82.5, 1.0 },
    { 110.0, 1.0 },
    {  82.5, 1.0 },
};
const int themeCount = (int)(sizeof(theme) / sizeof(Music));

const Music shot[] =
{
    { 880.0, 0.3 },  // A5 – sharp attack
    { 440.0, 0.2 },  // A4 – tail off
};
const int shotCount = (int)(sizeof(shot) / sizeof(Music));

const Music thrust[] =
{
    { 75.0, 2.0 },   // low growl (~75 Hz)
};
const int thrustCount = (int)(sizeof(thrust) / sizeof(Music));

const Music colosion[] =
{
    { 180.0, 0.4 },  // mid crack
    {  90.0, 0.5 },  // low body
    {  40.0, 1.0 },  // deep rumble tail
};
const int colosionCount = (int)(sizeof(colosion) / sizeof(Music));


void buildAudio()
{
    // Repeat the 12-note heartbeat 10 times for a long loop
    const int reps = 10;
    std::vector<Music> loop;
    loop.reserve(themeCount * reps);
    for (int i = 0; i < reps; i++)
        for (int j = 0; j < themeCount; j++)
            loop.push_back(theme[j]);

    CreateMusicFile("astroids_theme.wav", 160.0, 60.0, 44100,
        loop.data(), (int)loop.size(), 0.55f);

    CreateMusicFile("shot.wav", 160.0, 60.0, 44100, shot, shotCount, 0.70f);
    CreateMusicFile("thrust.wav", 160.0, 60.0, 44100, thrust, thrustCount, 0.40f);
    CreateMusicFile("collision.wav", 160.0, 60.0, 44100, colosion, colosionCount, 0.80f);
}

enum class State { MENU, PLAYING, DEAD_WAIT, GAME_OVER };

struct Poly
{
    std::vector<Vector2> verts;   // model-space vertices

    // Rotate every vertex by 'angle', translate to 'pos', then draw with DrawLine
    void draw(Vector2 pos, float angle, unsigned short col) const
    {
        if (verts.size() < 2) return;

        int n = (int)verts.size();
        for (int i = 0; i < n; i++) {
            Vector2 a = Vector2Add(Vector2Rotate(verts[i], angle), pos);
            Vector2 b = Vector2Add(Vector2Rotate(verts[(i + 1) % n], angle), pos);
            DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, PX, col);
        }
    }
};

struct Ship
{
    Vector2 pos;
    Vector2 vel;
    float   angle;
    float   invTimer;
    bool    alive;
    bool    thrusting;

    // Ship shape: a triangle pointing up (-Y)
    static Poly makePoly()
    {
        Poly p;
        p.verts = {
            {  0.0f, -5.0f },   // nose
            {  3.5f,  4.5f },   // bottom-right
            { -3.5f,  4.5f },   // bottom-left
        };
        return p;
    }

    // Engine flame triangle (drawn behind the ship when thrusting)
    static Poly makeFlame()
    {
        Poly p;
        p.verts = {
            { -2.0f,  4.5f },   // left of exhaust
            {  0.0f,  9.5f },   // flame tip
            {  2.0f,  4.5f },   // right of exhaust
        };
        return p;
    }
};

struct Asteroid
{
    Vector2 pos;
    Vector2 vel;
    float   angle;
    float   spin;
    int     size;    // 3=large  2=medium  1=small
    bool    alive;
    Poly    poly;

    float radius() const { return size * 4.2f; }
};

struct Bullet
{
    Vector2 pos;
    Vector2 vel;
    float   life;
    bool    alive;
};

struct Spark
{
    Vector2 pos;
    Vector2 vel;
    float   life;
    float   maxLife;
};

static State   g_state = State::MENU;
static int     g_score = 0;
static int     g_hiscore = 0;
static int     g_lives = 3;
static int     g_level = 1;
static float   g_timer = 0.0f;
static float   g_gameTime = 0.0f;

static Ship                  g_ship;
static Poly                  g_shipPoly;
static Poly                  g_flamePoly;
static std::vector<Asteroid> g_asteroids;
static std::vector<Bullet>   g_bullets;
static std::vector<Spark>    g_sparks;

static bool g_wasThrustingLastFrame = false;

float fwrap(float v, float lo, float hi)
{
    float range = hi - lo;
    while (v < lo)  v += range;
    while (v >= hi) v -= range;
    return v;
}

Vector2 wrapPos(Vector2 p)
{
    return
    {
        fwrap(p.x, 0.f, (float)WIDTH),
        fwrap(p.y, 0.f, (float)HEIGHT)
    };
}

// Circle collision using Vector2Distance from DonutMath
bool circleHit(Vector2 a, float ra, Vector2 b, float rb)
{
    return Vector2Distance(a, b) < ra + rb;
}

float randf() { return (float)rand() / ((float)RAND_MAX + 1.0f); }

Poly makeAsteroidPoly(int size)
{
    float r = (float)size * 4.0f;
    int   verts = 10 + rand() % 5;
    Poly  p;
    for (int i = 0; i < verts; i++) {
        float a = TUA * i / (float)verts;
        float rad = r * (0.65f + 0.35f * randf());
        p.verts.push_back({ cosf(a) * rad, sinf(a) * rad });
    }
    return p;
}

void addAsteroid(Vector2 pos, int size, Vector2 parentVel = { 0.f, 0.f })
{
    float dir = TUA * randf();
    float spd = (4.0f - (float)size) * 3.0f + 2.0f + 2.0f * randf();

    Asteroid a;
    a.pos = pos;
    // Velocity = random direction + 30% of parent's velocity 
    a.vel = Vector2Add({ cosf(dir) * spd, sinf(dir) * spd },
        Vector2Scale(parentVel, 0.3f));
    a.angle = 0.0f;
    a.spin = (0.6f + 1.5f * randf()) * (rand() % 2 ? 1.f : -1.f);
    a.size = size;
    a.alive = true;
    a.poly = makeAsteroidPoly(size);
    g_asteroids.push_back(a);
}

void spawnSparks(Vector2 pos, int n)
{
    for (int i = 0; i < n; i++) {
        float dir = TUA * randf();
        float spd = 5.0f + 25.0f * randf();
        Spark s;
        s.pos = pos;
        s.vel = { cosf(dir) * spd, sinf(dir) * spd };
        s.life = 0.35f + 0.45f * randf();
        s.maxLife = 0.8f;
        g_sparks.push_back(s);
    }
}

void spawnLevel(int level)
{
    g_asteroids.clear();
    g_bullets.clear();
    g_sparks.clear();

    Vector2 centre = { (float)WIDTH / 2.0f, (float)HEIGHT / 2.0f };
    int n = 2 + level;

    for (int i = 0; i < n; i++) {
        Vector2 pos;
        int tries = 0;
        do {
            pos = { randf() * WIDTH, randf() * HEIGHT };
            tries++;
        } while (tries < 50 && circleHit(pos, 18.0f, centre, 18.0f));
        addAsteroid(pos, 3);
    }
}

void resetShip()
{
    g_ship.pos = { (float)WIDTH / 2.0f, (float)HEIGHT / 2.0f };
    g_ship.vel = Vector2Zero();
    g_ship.angle = 0.0f;
    g_ship.invTimer = INV_TIME;
    g_ship.alive = true;
    g_ship.thrusting = false;
}

void startGame()
{
    g_score = 0;
    g_lives = 3;
    g_level = 1;
    g_gameTime = 0.0f;
    resetShip();
    spawnLevel(1);
    g_state = State::PLAYING;

    // Restart background music when a new game begins
    PlayMusicFile("astroids_theme.wav", nullptr, nullptr);
}

void update(float dt)
{
    g_gameTime += dt;
    GetKeyState();

    // Menu
    if (g_state == State::MENU)
    {
        if (GetKey(KEY_SPACE).k_Pressed || GetKey(KEY_ENTER).k_Pressed)
            startGame();
        return;
    }

    // Game over
    if (g_state == State::GAME_OVER)
    {
        g_timer -= dt;
        for (auto& s : g_sparks)
        {
            s.pos = Vector2Add(s.pos, Vector2Scale(s.vel, dt));
            s.life -= dt;
        }

        g_sparks.erase(std::remove_if(g_sparks.begin(), g_sparks.end(),
            [](const Spark& s) { return s.life <= 0.f; }), g_sparks.end());
        if (g_timer <= 0.0f && (GetKey(KEY_SPACE).k_Pressed || GetKey(KEY_ENTER).k_Pressed))
        {
            // Return to menu — stop music so the menu is silent until next game
            StopMusic();
            g_state = State::MENU;
        }
        return;
    }

    // Dead wait
    if (g_state == State::DEAD_WAIT)
    {
        g_timer -= dt;
        for (auto& a : g_asteroids)
        {
            a.pos = wrapPos(Vector2Add(a.pos, Vector2Scale(a.vel, dt)));
            a.angle += a.spin * dt;
        }

        for (auto& s : g_sparks)
        {
            s.pos = Vector2Add(s.pos, Vector2Scale(s.vel, dt));
            s.life -= dt;
        }

        g_sparks.erase(std::remove_if(g_sparks.begin(), g_sparks.end(),
            [](const Spark& s) { return s.life <= 0.f; }), g_sparks.end());
        if (g_timer <= 0.0f) { resetShip(); g_state = State::PLAYING; }
        return;
    }

    // Playing
    if (GetKey(KEY_ESC).k_Pressed)
    {
        StopMusic();
        g_state = State::MENU;
        return;
    }

    if (g_ship.alive)
    {

        // Rotation
        if (GetKey(KEY_LEFT).k_Held)  g_ship.angle -= TURN_SPEED * dt;
        if (GetKey(KEY_RIGHT).k_Held) g_ship.angle += TURN_SPEED * dt;

        // Thrust 
        g_ship.thrusting = GetKey(KEY_UP).k_Held;
        if (g_ship.thrusting)
        {
            Vector2 thrustDir = Vector2Rotate({ 0.0f, -1.0f }, g_ship.angle);
            g_ship.vel = Vector2Add(g_ship.vel,
                Vector2Scale(thrustDir, THRUST_POWER * dt));

            if (!g_wasThrustingLastFrame)
                PlaySFX("thrust.wav", 0.40f);
        }
        g_wasThrustingLastFrame = g_ship.thrusting;

        // Clamp speed using Vector2Length / Vector2Normalize
        float spd = Vector2Length(g_ship.vel);
        if (spd > MAX_SPEED)
            g_ship.vel = Vector2Scale(Vector2Normalize(g_ship.vel), MAX_SPEED);

        // Frame-rate-independent drag (applied 60 times/sec worth per second)
        float drag = powf(DRAG_PER_SEC, dt * 60.0f);
        g_ship.vel = Vector2Scale(g_ship.vel, drag);

        // Fire bullet
        if (GetKey(KEY_SPACE).k_Pressed) {
            int cnt = 0;
            for (auto& b : g_bullets) if (b.alive) cnt++;
            if (cnt < MAX_BULLETS) {
                Vector2 nose = Vector2Rotate({ 0.0f, -1.0f }, g_ship.angle);
                Bullet b;
                b.pos = Vector2Add(g_ship.pos, Vector2Scale(nose, 6.0f));
                b.vel = Vector2Add(Vector2Scale(nose, BULLET_SPEED), g_ship.vel);
                b.life = BULLET_LIFE;
                b.alive = true;
                g_bullets.push_back(b);

                PlaySFX("shot.wav", 0.70f);  // laser pew
            }
        }

        // Move & wrap ship using Vector2Add + Vector2Scale
        g_ship.pos = wrapPos(Vector2Add(g_ship.pos, Vector2Scale(g_ship.vel, dt)));
        if (g_ship.invTimer > 0.0f) g_ship.invTimer -= dt;
    }

    // Move bullets
    for (auto& b : g_bullets)
    {
        if (!b.alive) continue;
        b.pos = wrapPos(Vector2Add(b.pos, Vector2Scale(b.vel, dt)));
        b.life -= dt;
        if (b.life <= 0.f) b.alive = false;
    }

    // Move asteroids
    for (auto& a : g_asteroids)
    {
        a.pos = wrapPos(Vector2Add(a.pos, Vector2Scale(a.vel, dt)));
        a.angle += a.spin * dt;
    }

    // Move sparks
    for (auto& s : g_sparks)
    {
        s.pos = Vector2Add(s.pos, Vector2Scale(s.vel, dt));
        s.life -= dt;
    }

    g_sparks.erase(std::remove_if(g_sparks.begin(), g_sparks.end(),
        [](const Spark& s) { return s.life <= 0.f; }), g_sparks.end());

    // Bullet vs Asteroid
    std::vector<Asteroid> newAsteroids;
    for (auto& a : g_asteroids) {
        for (auto& b : g_bullets) {
            if (!b.alive) continue;
            if (!circleHit(b.pos, 1.0f, a.pos, a.radius())) continue;

            b.alive = false;
            a.alive = false;
            spawnSparks(a.pos, 5 + a.size * 4);

            PlaySFX("collision.wav", 0.80f);  // asteroid explosion

            if (a.size == 3)      g_score += 100;
            else if (a.size == 2) g_score += 150;
            else                  g_score += 200;
            if (g_score > g_hiscore) g_hiscore = g_score;

            // Split into 2 smaller pieces
            if (a.size > 1) {
                float d1 = TUA * randf();
                float d2 = d1 + PI + 0.4f * (randf() - 0.5f);
                float spd = (4.0f - (float)(a.size - 1)) * 3.0f + 2.0f;
                for (int k = 0; k < 2; k++) {
                    float dir = (k == 0) ? d1 : d2;
                    Asteroid na;
                    na.pos = a.pos;
                    na.vel = Vector2Add(
                        { cosf(dir) * spd, sinf(dir) * spd },
                        Vector2Scale(a.vel, 0.3f));
                    na.angle = 0.0f;
                    na.spin = (0.6f + 1.5f * randf()) * (k == 0 ? 1.f : -1.f);
                    na.size = a.size - 1;
                    na.alive = true;
                    na.poly = makeAsteroidPoly(na.size);
                    newAsteroids.push_back(na);
                }
            }
            break;
        }
    }

    g_asteroids.erase(std::remove_if(g_asteroids.begin(), g_asteroids.end(),
        [](const Asteroid& a) { return !a.alive; }), g_asteroids.end());
    for (auto& na : newAsteroids) g_asteroids.push_back(na);
    g_bullets.erase(std::remove_if(g_bullets.begin(), g_bullets.end(),
        [](const Bullet& b) { return !b.alive; }), g_bullets.end());

    // Ship vs Asteroid
    if (g_ship.alive && g_ship.invTimer <= 0.0f) {
        for (auto& a : g_asteroids) {
            if (!circleHit(g_ship.pos, 2.5f, a.pos, a.radius())) continue;
            g_ship.alive = false;
            spawnSparks(g_ship.pos, 22);

            PlaySFX("collision.wav", 0.80f);  // ship explosion

            g_lives--;
            if (g_lives > 0) { g_state = State::DEAD_WAIT; g_timer = RESPAWN_WAIT; }
            else { g_state = State::GAME_OVER;  g_timer = 1.2f; }
            break;
        }
    }

    // Level complete
    if (g_asteroids.empty())
    {
        g_level++;
        spawnLevel(g_level);
        resetShip();
    }
}

// Draw a segmented circle 
void drawCircleLines(int cx, int cy, int r, int segs, unsigned short col)
{
    for (int i = 0; i < segs; i++) {
        float a1 = TUA * i / (float)segs;
        float a2 = TUA * (i + 1) / (float)segs;
        DrawLine(cx + (int)(cosf(a1) * r), cy + (int)(sinf(a1) * r),
            cx + (int)(cosf(a2) * r), cy + (int)(sinf(a2) * r),
            PX, col);
    }
}

// Draw a filled rectangle border with DrawLine (for Game Over box)
void drawRectLine(int x1, int y1, int x2, int y2, unsigned short col)
{
    DrawLine(x1, y1, x2, y1, PX, col);
    DrawLine(x2, y1, x2, y2, PX, col);
    DrawLine(x2, y2, x1, y2, PX, col);
    DrawLine(x1, y2, x1, y1, PX, col);
}

void draw()
{
    Fill(0, 0, WIDTH, HEIGHT, ' ', BLACK);

    // Menu
    if (g_state == State::MENU) {
        DrawString(WIDTH / 2 - 9, HEIGHT / 2 - 8, L"A S T E R O I D S", CYAN);
        DrawString(WIDTH / 2 - 10, HEIGHT / 2 - 6, L"- DonutAPI Edition -", WHITE);

        DrawString(WIDTH / 2 - 14, HEIGHT / 2 - 2, L"[LEFT / RIGHT]  Rotate", GRAY);
        DrawString(WIDTH / 2 - 14, HEIGHT / 2 - 1, L"[UP]            Thrust", GRAY);
        DrawString(WIDTH / 2 - 14, HEIGHT / 2 + 0, L"[SPACE]         Fire", GRAY);
        DrawString(WIDTH / 2 - 14, HEIGHT / 2 + 1, L"[ESC]           Quit", GRAY);

        DrawString(WIDTH / 2 - 10, HEIGHT / 2 + 4, L"Press SPACE to Play", GREEN);

        if (g_hiscore > 0) {
            std::wstring hs = L"HI-SCORE: " + std::to_wstring(g_hiscore);
            DrawString(WIDTH / 2 - (int)hs.size() / 2, HEIGHT / 2 + 6, hs, YELLOW);
        }

        // Small ship outlines drawn with DrawLine in corners
        DrawLine(14, 5, 10, 13, PX, DARK_GRAY);
        DrawLine(10, 13, 18, 13, PX, DARK_GRAY);
        DrawLine(18, 13, 14, 5, PX, DARK_GRAY);

        DrawLine(WIDTH - 15, 5, WIDTH - 11, 13, PX, GRAY);
        DrawLine(WIDTH - 11, 13, WIDTH - 19, 13, PX, GRAY);
        DrawLine(WIDTH - 19, 13, WIDTH - 15, 5, PX, GRAY);

        // Asteroid outlines using segmented DrawLine circles
        drawCircleLines(12, HEIGHT - 8, 5, 12, DARK_GRAY);
        drawCircleLines(WIDTH - 14, HEIGHT - 8, 5, 12, GRAY);
        return;
    }

    // Sparks drawn as short lines in their travel direction 
    for (auto& s : g_sparks)
    {
        if (s.life <= 0.0f) continue;
        float t = s.life / s.maxLife;
        unsigned short col = (t > 0.6f) ? WHITE : (t > 0.3f) ? WHITE : WHITE;
        // End point = pos + normalised vel * 1.5 pixels
        Vector2 norm = Vector2Normalize(s.vel);
        int x1 = (int)s.pos.x;
        int y1 = (int)s.pos.y;
        int x2 = (int)(s.pos.x + norm.x * 1.5f);
        int y2 = (int)(s.pos.y + norm.y * 1.5f);
        if (x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT)
            DrawLine(x1, y1, x2, y2, PX, col);
    }

    // Asteroids | Poly::draw uses DrawLine for each edge 
    for (auto& a : g_asteroids) {
        if (!a.alive) continue;
        unsigned short col = (a.size == 3) ? GRAY
            : (a.size == 2) ? WHITE
            : CYAN;
        a.poly.draw(a.pos, a.angle, col);
    }

    // Bullets | small '+' cross with two DrawLine
    for (auto& b : g_bullets) {
        if (!b.alive) continue;
        int px = (int)b.pos.x, py = (int)b.pos.y;
        if (px >= 1 && px < WIDTH - 1 && py >= 1 && py < HEIGHT - 1) {
            DrawLine(px - 1, py, px + 1, py, PX, WHITE);
            DrawLine(px, py - 1, px, py + 1, PX, WHITE);
        }
    }

    // Ship | Poly::draw (DrawLine per edge) with blink when invincible 
    bool drawShip = g_ship.alive &&
        (g_ship.invTimer <= 0.0f || (int)(g_gameTime * 8.0f) % 2 == 0);
    if (drawShip) {
        unsigned short shipCol = (g_ship.invTimer > 0.0f) ? CYAN : WHITE;
        g_shipPoly.draw(g_ship.pos, g_ship.angle, shipCol);

        if (g_ship.thrusting)
        {
            unsigned short flameCol = ((int)(g_gameTime * 20.0f) % 2 == 0) ? WHITE : WHITE;
            g_flamePoly.draw(g_ship.pos, g_ship.angle, flameCol);
        }
    }

    // HUD
    std::wstring scoreTxt = L"SCORE: " + std::to_wstring(g_score);
    DrawString(2, 0, scoreTxt, WHITE);

    std::wstring lvlTxt = L"LEVEL " + std::to_wstring(g_level);
    DrawString(WIDTH / 2 - (int)lvlTxt.size() / 2, 0, lvlTxt, CYAN);

    // Each life = a mini ship triangle using DrawLine
    for (int i = 0; i < g_lives; i++)
    {
        int lx = WIDTH - 14 + i * 5;  // center x, spaced 5px apart
        int ty = 1;                   // nose y
        int by = 4;                   // base y

        DrawLine(lx, ty, lx - 2, by, PX, WHITE);      // nose to bottom-left
        DrawLine(lx - 2, by, lx + 2, by, PX, WHITE);  // bottom-left to bottom-right
        DrawLine(lx + 2, by, lx, ty, PX, WHITE);      // bottom-right to nose
    }

    // Overlay messages
    if (g_state == State::DEAD_WAIT)
    {
        DrawString(WIDTH / 2 - 6, HEIGHT / 2 + 2, L"Respawning...", YELLOW);
    }

    if (g_state == State::GAME_OVER)
    {
        Fill(WIDTH / 2 - 14, HEIGHT / 2 - 3, WIDTH / 2 + 14, HEIGHT / 2 + 5, ' ', BLACK);
        drawRectLine(WIDTH / 2 - 13, HEIGHT / 2 - 2, WIDTH / 2 + 13, HEIGHT / 2 + 4, RED);

        DrawString(WIDTH / 2 - 4, HEIGHT / 2 - 1, L"GAME OVER", RED);

        std::wstring stxt = L"SCORE: " + std::to_wstring(g_score);
        DrawString(WIDTH / 2 - (int)stxt.size() / 2, HEIGHT / 2 + 1, stxt, WHITE);

        std::wstring htxt = L"HI-SCORE: " + std::to_wstring(g_hiscore);
        DrawString(WIDTH / 2 - (int)htxt.size() / 2, HEIGHT / 2 + 2, htxt, YELLOW);

        if (g_timer <= 0.0f)
            DrawString(WIDTH / 2 - 10, HEIGHT / 2 + 4, L"SPACE / ENTER  Menu", CYAN);
    }
}

int main()
{
    srand((unsigned int)time(nullptr));

    InitWindow(WIDTH, HEIGHT, FONT_W, FONT_H);
    SetWindowName(L"ASTEROIDS");
    SetFPS(0);
    ShowConsoleCursor(false);

    g_shipPoly = Ship::makePoly();
    g_flamePoly = Ship::makeFlame();

    buildAudio();
    PlayMusicFile("astroids_theme.wav", nullptr, nullptr);

    while (!WindowShouldClose())
    {
        float dt = GetElapsedTime();
        if (dt < 0.0001f) dt = 0.016f;
        if (dt > 0.05f)   dt = 0.05f;

        update(dt);
        draw();
        UpdateScreen();
    }

    StopMusic();  
    DestroyWindow();
    return 0;
}