#include "DonutAPI.h"

const int screenWidth = 210;
const int screenHeight = 100;

int main() 
{
	InitWindow(screenWidth, screenHeight, 8, 8);
	SetWindowName(L"Rects");
	ShowConsoleCursor(false);
	SetFPS(60);

	RectangleDef rectA = { 10, screenHeight / 2 - 50, 50, 50 };
	float rectASpeed = 0.8;

	RectangleDef rectB = { screenWidth / 2 - 30, screenHeight / 2 - 30, 20, 20}; 

	RectangleDef rectCollision = { 0 }; 

	int screenUpperLimit = 40;      

	bool pause = false;             
	bool collision = false;

	while (!WindowShouldClose()) 
	{
		GetKeyState();

		if (!pause) rectA.x += rectASpeed;

		if (((rectA.x + rectA.width) >= screenWidth) || (rectA.x <= 0)) rectASpeed *= -1;

		rectB.x = GetMouseX();
		rectB.y = GetMouseY();

		if ((rectB.x + rectB.width) >= screenWidth) rectB.x = screenWidth - rectB.width;
		else if (rectB.x <= 0) rectB.x = 0;

		if ((rectB.y + rectB.height) >= screenHeight) rectB.y = screenHeight - rectB.height;
		else if (rectB.y <= screenUpperLimit) rectB.y = (float)screenUpperLimit;

		collision = CheckCollisionRects(rectA, rectB);

		if (collision) rectCollision = GetCollisionRects(rectA, rectB);

		if (GetKey(KEY_SPACE).k_Pressed) pause = !pause;

		Fill(0, 0, screenWidth, screenHeight, PIXEL_SOLID, BROWN);

		DrawRectangle(0, 0, screenUpperLimit, PIXEL_SOLID, collision ? RED : BLACK);

		DrawRectangleRect(rectA, PIXEL_SOLID, YELLOW);
		DrawRectangleRect(rectB, PIXEL_SOLID, BLUE);

		if (collision)
		{
			// Draw collision area
			DrawRectangleRect(rectCollision, PIXEL_SOLID, GREEN);
		}

		UpdateScreen();
	}

	DestroyWindow();
}