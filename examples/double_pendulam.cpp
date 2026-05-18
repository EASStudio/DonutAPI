#include "DonutAPI.h"
#include "DonutMath.h"

const int WIDTH = 210;
const int HEIGHT = 200; 
const int RADIUS = 5;
const int L1 = 20; 
const int L2 = 15; 
const int G = 100;
const int TRACE_LEN = 100;
const int TRACE_THICKNESS = 1;
int currentTraceLen = 0;

Vector2 startPos = { WIDTH / 2, 0 };
Vector2 trace[TRACE_LEN];

float l1, l2, phi1, phi2, phi1_d, phi2_d, phi1_dd, phi2_dd, m1, m2;

int GetRandomValue(int min, int max) 
{
    if (max <= min) return min;
    return min + std::rand() % (max - min + 1);
}

Vector2 getEnd(Vector2 start, float l, float phi)
{
    return { start.x + l * sinf(phi), start.y + l * cosf(phi) };
}

void step(float dt) 
{
    // angular acceleration
    phi1_dd = (-G * (2 * m1 + m2) * sinf(phi1) - m2 * G * sinf(phi1 - 2 * phi2) - 2 * sinf(phi1 - phi2) * m2 * (phi2_d * phi2_d * l2 + phi1_d * phi1_d * l1 * cosf(phi1 - phi2))) / (l1 * (2 * m1 + m2 - m2 * cosf(2 * phi1 - 2 * phi2)));

    phi2_dd = 2 * sinf(phi1 - phi2) * (phi1_d * phi1_d * l1 * (m1 + m2) + G * (m1 + m2) * cosf(phi1) + phi2_d * phi2_d * l1 * m2 * cosf(phi1 - phi2)) / (l2 * (2 * m1 + m2 - m2 * cosf(2 * phi1 - 2 * phi2)));

    //angular velocity
    phi1_d += phi1_dd * dt;
    phi2_d += phi2_dd * dt;

    // angle itself
    phi1 += phi1_d * dt;
    phi2 += phi2_d * dt;
}

void initSolver() 
{
    l1 = L1;
    l2 = L2;
    phi1 = GetRandomValue(-90, 90) * DEG2RAD; 
    phi2 = GetRandomValue(-90, 90) * DEG2RAD;
    phi1_d = 0;
    phi2_d = 0;
    m1 = 1;
    m2 = 1;
}

void drawPendulum(Vector2 start, float l, float phi)
{
    Vector2 end = getEnd(start, l, phi);
    DrawLine(start.x, start.y, end.x, end.y, PIXEL_SOLID, WHITE);
    FillCircle(end.x, end.y, RADIUS, PIXEL_SOLID, RED);
}

void drawDoublePendulum(Vector2 start, float l1, float l2, float phi1, float phi2) 
{
    // Draw the pendulum
    Vector2 endL1 = getEnd(start, l1, phi1);
    Vector2 endL2 = getEnd(endL1, l2, phi2);

    // Draw second pendulum first
    drawPendulum(endL1, l2, phi2);

    drawPendulum(start, l1, phi1);

    // Draw the pendulum trace
    Vector2 traceCopy[TRACE_LEN];
    memcpy(traceCopy, trace, TRACE_LEN * sizeof(Vector2));

    if (currentTraceLen < TRACE_LEN)
        currentTraceLen++;

    for (int i = 1; i < currentTraceLen; ++i)
        trace[i] = traceCopy[i - 1];

    trace[0] = endL2;

    for (int i = 0; i < TRACE_LEN; ++i)
        DrawRectangle(trace[i].x, trace[i].y, TRACE_THICKNESS, PIXEL_SOLID, DARK_RED);
}

int main()
{
    InitWindow(WIDTH, HEIGHT, 8, 8);
    SetWindowName(L"A Double Pendulum");
    ShowConsoleCursor(false);

    initSolver();
    while (!WindowShouldClose())
    {
        step(GetElapsedTime());

        Fill(0, 0, WIDTH, HEIGHT, PIXEL_SOLID, BLACK);

        drawDoublePendulum(startPos, l1, l2, phi1, phi2);

        UpdateScreen();
    }

    DestroyWindow();
    return 0;
}