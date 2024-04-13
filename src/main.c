#include <SDL2/SDL.h>
#undef main

#include "main.h"
#include "tinyFPS.h"

#include "main3DO.h"

static SDL_Surface *surface;
static SDL_Window *window;
static ScreenBuffer screenSDL;

static bool fpsDisplay = false;
static bool vsync = false;
static bool wasInputUpdateCalled = false;

#define MAP_3DO_JOY_BUTTON_UP			SDLK_UP
#define MAP_3DO_JOY_BUTTON_UP_ALT		SDLK_KP_8
#define MAP_3DO_JOY_BUTTON_DOWN			SDLK_DOWN
#define MAP_3DO_JOY_BUTTON_DOWN_ALT		SDLK_KP_2
#define MAP_3DO_JOY_BUTTON_LEFT			SDLK_LEFT
#define MAP_3DO_JOY_BUTTON_LEFT_ALT		SDLK_KP_4
#define MAP_3DO_JOY_BUTTON_RIGHT		SDLK_RIGHT
#define MAP_3DO_JOY_BUTTON_RIGHT_ALT	SDLK_KP_6
#define MAP_3DO_JOY_BUTTON_A			SDLK_z
#define MAP_3DO_JOY_BUTTON_B			SDLK_x
#define MAP_3DO_JOY_BUTTON_C			SDLK_c
#define MAP_3DO_JOY_BUTTON_LPAD			SDLK_a
#define MAP_3DO_JOY_BUTTON_RPAD			SDLK_d
#define MAP_3DO_JOY_BUTTON_SELECT		SDLK_RSHIFT
#define MAP_3DO_JOY_BUTTON_START		SDLK_RETURN


static void basicInputSDL(SDL_Event *event)
{
	static int quit = false;

	switch (event->type)
	{
		case SDL_QUIT:
			quit = true;
		break;

		case SDL_KEYUP:
		case SDL_KEYDOWN:
		{
			const int key = event->key.keysym.sym;
			switch (key)
			{
			case SDLK_ESCAPE:
				quit = true;
				break;

			case SDLK_p:
				if (event->type == SDL_KEYDOWN) {
					vsync = !vsync;
				}
				break;

			case SDLK_f:
				if (event->type == SDL_KEYDOWN) {
					fpsDisplay = !fpsDisplay;
				}
				break;

			default:
				break;
			}
		}
		break;

		default:
			break;
	}
	if (quit) {
		SDL_Quit();
		exit(0);
	}
}

void update3DOinputSDL(ControlPadEventData *controlPadEventData3DO)
{
	static SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_KEYUP:
			case SDL_KEYDOWN:
			{
				const int key = event.key.keysym.sym;
				switch (key)
				{
				case MAP_3DO_JOY_BUTTON_UP:
				case MAP_3DO_JOY_BUTTON_UP_ALT:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlUp;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlUp;
					}
					break;

				case MAP_3DO_JOY_BUTTON_DOWN:
				case MAP_3DO_JOY_BUTTON_DOWN_ALT:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlDown;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlDown;
					}
					break;

				case MAP_3DO_JOY_BUTTON_LEFT:
				case MAP_3DO_JOY_BUTTON_LEFT_ALT:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlLeft;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlLeft;
					}
					break;

				case MAP_3DO_JOY_BUTTON_RIGHT:
				case MAP_3DO_JOY_BUTTON_RIGHT_ALT:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlRight;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlRight;
					}
					break;

				case MAP_3DO_JOY_BUTTON_A:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlA;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlA;
					}
					break;

				case MAP_3DO_JOY_BUTTON_B:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlB;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlB;
					}
					break;

				case MAP_3DO_JOY_BUTTON_C:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlC;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlC;
					}
					break;

				case MAP_3DO_JOY_BUTTON_LPAD:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlLeftShift;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlLeftShift;
					}
					break;

				case MAP_3DO_JOY_BUTTON_RPAD:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlRightShift;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlRightShift;
					}
					break;

				case MAP_3DO_JOY_BUTTON_SELECT:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlX;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlX;
					}
					break;

				case MAP_3DO_JOY_BUTTON_START:
					if (event.type == SDL_KEYDOWN) {
						controlPadEventData3DO->cped_ButtonBits |= ControlStart;
					}
					else {
						controlPadEventData3DO->cped_ButtonBits &= ~ControlStart;
					}
					break;

				default:
					break;
				}
				break;
			}

			default:
				break;
		}

		// Separate SDL window events (like escape closing the PC app) from 3DO specific input updates
		basicInputSDL(&event);
	}
	wasInputUpdateCalled = true;
}

static void screenInit(int width, int height)
{
	// I should grab that from the OS or something. Now I hack it for my laptop
	const int desktopScreenWidth = 1920;
	const int desktopScreenHeight = 1080;

	window = SDL_CreateWindow("Bizarro Engine", (desktopScreenWidth - width) / 2, (desktopScreenHeight - height) / 2, width, height, 0);
	surface = SDL_GetWindowSurface(window);

	screenSDL.width = surface->w;
	screenSDL.height = surface->h;
	screenSDL.vram = (unsigned int*)surface->pixels;
}

static void render3DOscreen(uint16 *src, VDLmapColor *VDLmap)
{
	uint32* dst = screenSDL.vram;
	const int screenWidth = screenSDL.width;

	for (int y = 0; y < SDL_SCREEN_HEIGHT; y += SCREEN_SCALE) {
		const int py = y / SCREEN_SCALE;
		for (int x = 0; x < SDL_SCREEN_WIDTH; x += SCREEN_SCALE) {
			const int px = x / SCREEN_SCALE;
			const int offset = (py >> 1) * SCREEN_WIDTH_3DO * 2 + (py & 1) + (px << 1);
			const uint16 c = *(src + offset);

			const int r = VDLmap[(c >> 10) & 31].r;
			const int g = VDLmap[(c >> 5) & 31].g;
			const int b = VDLmap[c & 31].b;
			const uint32 c32 = (r << 16) | (g << 8) | b;
			for (int j = 0; j < SCREEN_SCALE; ++j) {
				uint32* texelDst = dst + j * screenWidth + x;
				for (int i = 0; i < SCREEN_SCALE; ++i) {
					*(texelDst + i) = c32;
				}
			}
		}
		dst += screenWidth * SCREEN_SCALE;
	}
}

void update3DOscreenSDL(uint16 *vram3DOptr, VDLmapColor *VDLmap)
{
	static int timeTicks;

	render3DOscreen(vram3DOptr, VDLmap);

	if (fpsDisplay) {
		drawFps(&screenSDL, SDL_GetTicks(), 4, SDL_SCREEN_HEIGHT - 32, 4);
	}

	SDL_UpdateWindowSurface(window);

	if (vsync) {
		do {} while (SDL_GetTicks() - timeTicks < 20);
	}
	timeTicks = SDL_GetTicks();

	// Necessary hack in case a very basic 3DO project renders something but never calls the Update Controller functions from 3DO SDK as I also need to detect escape for app exit (like in lesson0.c)
	if (!wasInputUpdateCalled) {
		static SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			basicInputSDL(&event);
		}
	}
	wasInputUpdateCalled = false;
}

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	screenInit(SDL_SCREEN_WIDTH, SDL_SCREEN_HEIGHT);

	initFpsFonts();

	main3DO();

    return 0;
}
