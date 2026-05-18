#include "DonutAPI.h"

const int WIDTH = 210; 
const int HEIGHT = 200;
const int FONT_W = 8;
const int FONT_H = 8;

const int MAX_WAVES = 50;
const int WAVE_SPEED = 25; 
const float CAR_SPEED = 1.0f; 
const float WAVE_EMISSION_FREQUENCY = 6.0f;

int currentWaves = 0;

struct Car
{
	float x;
	float y;
};
struct Car car;

struct SoundWave 
{
	float x;
	float y;
	float r;
};
struct SoundWave waves[MAX_WAVES];

void drawCar() 
{
	FillCircle(car.x, car.y, 2, PIXEL_SOLID, WHITE);
}

void drawWaves()
{
	for (int i = 0; i < currentWaves; i++)
	{
		DrawCircle(waves[i].x, waves[i].y, waves[i].r, PIXEL_SOLID, BLUE);
	}
}

void propagateWaves(float deltaTime) 
{
	for (int i = 0; i < currentWaves; i++)
	{
		waves[i].r += WAVE_SPEED * deltaTime;
	}
}

void emitNewWaves() 
{ 
	struct SoundWave copy[MAX_WAVES];
	memset(copy, 0, MAX_WAVES * sizeof(struct SoundWave));
	for (int i = 0; i < MAX_WAVES; i++) 
	{
		copy[i] = waves[i];
	}

	for (int i = 0; i < MAX_WAVES; ++i) 
	{
		if (i < MAX_WAVES - 1)
			waves[i + 1] = copy[i];
	}

	waves[0] = { car.x, car.y, 0.0f };
	if (currentWaves < MAX_WAVES)
		currentWaves++;
}

int main() 
{
	InitWindow(WIDTH, HEIGHT, FONT_W, FONT_H);
	SetWindowName(L"Doppler Effect");
	ShowConsoleCursor(false);
	SetFPS(60);

	car = { 100.0f, 25.0f };

	float interval = 0;

	while(!WindowShouldClose()) 
	{
		float delta = GetElapsedTime();

		interval += delta;
		if (interval > 1.0f / WAVE_EMISSION_FREQUENCY) 
		{
			emitNewWaves();
			interval = 0.0f;
		}

		propagateWaves(delta);

		GetKeyState();

		if (GetKey(KEY_RIGHT).k_Held) car.x += CAR_SPEED;
		if (GetKey(KEY_LEFT).k_Held) car.x -= CAR_SPEED;
		if (GetKey(KEY_UP).k_Held) car.y -= CAR_SPEED;
		if (GetKey(KEY_DOWN).k_Held) car.y += CAR_SPEED;

		Fill(0, 0, WIDTH, HEIGHT, PIXEL_SOLID, BLACK);

		drawCar();
		drawWaves();

		UpdateScreen();
	}
}