#include "DonutAPI.h"

const int WIDTH = 210; 
const int HEIGHT = 200;
const int FONT_W = 8;
const int FONT_H = 8;

const int OFFSET = 2;
const int START_X = 20; 
const int DIGIT_DISTANCE = 25; 

const float SEGMENT_WIDTH = 6; 
const float SEGMENT_THICKNESS = 2; 

int digits[10][7] = { { 1, 1, 1, 0, 1, 1, 1 },  // 0
				      { 0, 0, 1, 0, 0, 1, 0 },  // 1
				      { 1, 0, 1, 1, 1, 0, 1 },  // 2
				      { 1, 0, 1, 1, 0, 1, 1 },  // 3
				      { 0, 1, 1, 1, 0, 1, 0 },  // 4
				      { 1, 1, 0, 1, 0, 1, 1 },  // 5
				      { 1, 1, 0, 1, 1, 1, 1 },  // 6 
				      { 1, 0, 1, 0, 0, 1, 0 },  // 7
				      { 1, 1, 1, 1, 1, 1, 1 },  // 8
				      { 1, 1, 1, 1, 0, 1, 1 },  // 9
};

void drawSegment(Vector2 center, bool horizontal, COLOR color) 
{
	int count = 6;
	Vector2 a, b, c, d, e, f;

	if (horizontal) 
	{
		a = { center.x - SEGMENT_WIDTH / 2 - SEGMENT_THICKNESS / 2, center.y };
		b = { center.x - SEGMENT_WIDTH / 2, center.y + SEGMENT_THICKNESS / 2 };
		c = { center.x - SEGMENT_WIDTH / 2, center.y - SEGMENT_THICKNESS / 2 };
		d = { center.x + SEGMENT_WIDTH / 2, center.y + SEGMENT_THICKNESS / 2 };
		e = { center.x + SEGMENT_WIDTH / 2, center.y - SEGMENT_THICKNESS / 2 };
		f = { center.x + SEGMENT_WIDTH / 2 + SEGMENT_THICKNESS / 2, center.y };
	}

	else 
	{
		a = { center.x, center.y - SEGMENT_WIDTH / 2 - SEGMENT_THICKNESS / 2 };
		b = { center.x - SEGMENT_THICKNESS / 2, center.y - SEGMENT_WIDTH / 2 };
		c = { center.x + SEGMENT_THICKNESS / 2, center.y - SEGMENT_WIDTH / 2 }; 
		d = { center.x - SEGMENT_THICKNESS / 2, center.y + SEGMENT_WIDTH / 2 }; 
		e = { center.x + SEGMENT_THICKNESS / 2, center.y + SEGMENT_WIDTH / 2 };
		f = { center.x, center.y + SEGMENT_WIDTH / 2 + SEGMENT_THICKNESS / 2 }; 
	}

	Vector2 points[] = { a, b, c, d, e, f };
	FillTriangleStrip(points, count, PIXEL_SOLID, color);
}

void drawDigit(Vector2 center, int digit) 
{
	int *digitSegments = &digits[digit][0];

	Vector2 first = { center.x, center.y - SEGMENT_WIDTH - OFFSET};
	drawSegment(first, true, digitSegments[0] ? RED : DARK_GRAY);

	Vector2 second = { center.x - SEGMENT_WIDTH / 2 - OFFSET / 2, center.y - SEGMENT_WIDTH / 2 - OFFSET / 2};
	drawSegment(second, false, digitSegments[1] ? RED : DARK_GRAY);

	Vector2 third = { center.x + SEGMENT_WIDTH / 2 + OFFSET / 2, center.y - SEGMENT_WIDTH / 2 };
	drawSegment(third, false, digitSegments[2] ? RED : DARK_GRAY);

	Vector2 fourth = { center.x, center.y };
	drawSegment(fourth, true, digitSegments[3] ? RED : DARK_GRAY);

	Vector2 fifth = { center.x - SEGMENT_WIDTH / 2 - OFFSET / 2, center.y + SEGMENT_WIDTH / 2 + OFFSET / 2};
	drawSegment(fifth, false, digitSegments[4] ? RED : DARK_GRAY);

	Vector2 sixth = { center.x + SEGMENT_WIDTH / 2 + OFFSET / 2, center.y + SEGMENT_WIDTH / 2 + OFFSET / 2};
	drawSegment(sixth, false, digitSegments[5] ? RED : DARK_GRAY);

	Vector2 seventh = { center.x, center.y + SEGMENT_WIDTH + OFFSET};
	drawSegment(seventh, true, digitSegments[6] ? RED : DARK_GRAY);

}

void drawColon(Vector2 center) 
{
	FillCircle(center.x, center.y - 5, 2, PIXEL_SOLID, RED);
}

void drawTime(int hours, int minutes, int seconds) 
{
	
	// Hour
	drawDigit({ START_X, 20 }, hours / 10);
	drawDigit({ START_X + DIGIT_DISTANCE, 20 }, hours % 10);

	drawColon(Vector2{ 58, 23 });
	drawColon(Vector2{ 58, 30 });

	// Minute
	drawDigit({ START_X + 2 * DIGIT_DISTANCE, 20 }, minutes / 10);
	drawDigit({ START_X + 3 * DIGIT_DISTANCE, 20 }, minutes % 10);

	drawColon(Vector2{ 108, 23 });
	drawColon(Vector2{ 108, 30 });

	// Second
	drawDigit({ START_X + 4 * DIGIT_DISTANCE, 20 }, seconds / 10);
	drawDigit({ START_X + 5 * DIGIT_DISTANCE, 20 }, seconds % 10);
}

int main() 
{
	InitWindow(WIDTH, HEIGHT, FONT_W, FONT_H);
	SetWindowName(L"Clock");
	ShowConsoleCursor(false);
	SetFPS(60);

	while (!WindowShouldClose())
	{
		Fill(0, 0, WIDTH, HEIGHT, PIXEL_SOLID, BLACK);

		time_t currentTime = time(NULL);
		struct tm currentLocalTime; 
		localtime_s(&currentLocalTime, &currentTime);

		drawTime(currentLocalTime.tm_hour, currentLocalTime.tm_min, currentLocalTime.tm_sec);

		UpdateScreen();
	}

	DestroyWindow();
	return 0;
}