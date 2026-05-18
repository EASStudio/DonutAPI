#include "DonutAPI.h"

const int WIDTH = 210;
const int HEIGHT = 200;
const int COLOR_SELECT_SIZE = 5; // How big the color select box is 
const int COLOR_SIZE = 15; // How many colors are in the colorPalette
const int BRUSH_SIZE = 2;
COLOR color = WHITE;

COLOR colorPalette[] = { BLACK, DARK_BLUE, DARK_GREEN, DARK_CYAN, DARK_RED, DARK_MAGENTA, BROWN, GRAY, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE };

struct PaintedCircle 
{
	int x, y, radius;
	COLOR colors;
};

void drawPalette(COLOR *colors, int size) 
{
	for (int i = 0; i < size; i++)
	{
		FillRectangle(i * COLOR_SELECT_SIZE + 82, 0, COLOR_SELECT_SIZE, PIXEL_SOLID, colors[i]);
	}
}

void checkPaletteXY(int x, int y)
{
	int paletteStartX = 80; 
	if (x >= paletteStartX && x < paletteStartX + COLOR_SIZE * COLOR_SELECT_SIZE && y < COLOR_SELECT_SIZE)
	{
		int i = (x - paletteStartX) / COLOR_SELECT_SIZE;
		if (i >= 0 && i < COLOR_SIZE)
			color = colorPalette[i];
	}
}

int main() 
{
	InitWindow(WIDTH, HEIGHT, 8, 8);
	SetWindowName(L"Paint");
	ShowConsoleCursor(false);
	SetFPS(60);

	std::vector<PaintedCircle> strokes;

	while(!WindowShouldClose()) 
	{
		Fill(0, 0, WIDTH, HEIGHT, PIXEL_SOLID, DARK_GRAY);

		GetKeyState();

		int mouseX = GetMouseX();
		int mouseY = GetMouseY();

		MouseState mouse = GetMouseState(); 

		if (mouse.leftHeld)
		{
			checkPaletteXY(mouseX, mouseY);
			strokes.push_back({ mouseX, mouseY, BRUSH_SIZE, color });
		}

		// Redraw all recorded circles every frame
		for (auto& c : strokes)
		    FillCircle(c.x, c.y, c.radius, PIXEL_SOLID, c.colors);

		drawPalette(colorPalette, COLOR_SIZE);

		UpdateScreen();
	}

	DestroyWindow();
	return 0;
}