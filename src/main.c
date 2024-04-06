#include <SDL2/SDL.h>
#undef main

#include "main.h"
#include "tinyFPS.h"

#include "project/main3DO.h"

#include "hardware.h"
#include "types.h"


static bool quit = false;
static bool vsync = false;

static SDL_Surface *surface;
static SDL_Window *window;
static ScreenBuffer screenSDL;


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

static void updateInput()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				quit = true;
				break;

			case SDL_KEYUP:
			case SDL_KEYDOWN:
			{
				const int key = event.key.keysym.sym;
				switch (key)
				{
					case SDLK_ESCAPE:
						quit = true;
						break;

					case SDLK_p:
						if (event.type == SDL_KEYDOWN) {
							vsync = !vsync;
						}
						break;

					case MAP_3DO_JOY_BUTTON_UP:
					case MAP_3DO_JOY_BUTTON_UP_ALT:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlUp;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlUp;
						}
						break;

					case MAP_3DO_JOY_BUTTON_DOWN:
					case MAP_3DO_JOY_BUTTON_DOWN_ALT:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlDown;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlDown;
						}
						break;

					case MAP_3DO_JOY_BUTTON_LEFT:
					case MAP_3DO_JOY_BUTTON_LEFT_ALT:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlLeft;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlLeft;
						}
						break;

					case MAP_3DO_JOY_BUTTON_RIGHT:
					case MAP_3DO_JOY_BUTTON_RIGHT_ALT:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlRight;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlRight;
						}
						break;

					case MAP_3DO_JOY_BUTTON_A:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlA;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlA;
						}
						break;

					case MAP_3DO_JOY_BUTTON_B:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlB;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlB;
						}
						break;

					case MAP_3DO_JOY_BUTTON_C:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlC;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlC;
						}
						break;

					case MAP_3DO_JOY_BUTTON_LPAD:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlLeftShift;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlLeftShift;
						}
						break;

					case MAP_3DO_JOY_BUTTON_RPAD:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlRightShift;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlRightShift;
						}
						break;

					case MAP_3DO_JOY_BUTTON_SELECT:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlX;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlX;
						}
						break;

					case MAP_3DO_JOY_BUTTON_START:
						if (event.type == SDL_KEYDOWN) {
							globalControlPadEventData.cped_ButtonBits |= ControlStart;
						} else {
							globalControlPadEventData.cped_ButtonBits &= ~ControlStart;
						}
						break;

					default:
						break;
				}
				break;

			default:
				break;
			}
		}
	}
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

static void render3DOscreen()
{
	uint16* src = vram3DOptr;
	uint32* dst = screenSDL.vram;
	const int screenWidth = screenSDL.width;

	for (int y = 0; y < SDL_SCREEN_HEIGHT; y += SCREEN_SCALE) {
		const int py = y / SCREEN_SCALE;
		for (int x = 0; x < SDL_SCREEN_WIDTH; x += SCREEN_SCALE) {
			const int px = x / SCREEN_SCALE;
			const int offset = (py >> 1) * SCREEN_WIDTH * 2 + (py & 1) + (px << 1);
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

int main()
{
	int timeTicks;

	SDL_Init(SDL_INIT_EVERYTHING);
	screenInit(SDL_SCREEN_WIDTH, SDL_SCREEN_HEIGHT);

	initFpsFonts();

	//initHardware3DO();

	do {
		timeTicks = SDL_GetTicks();

		updateInput();

		main3DO();

		render3DOscreen();

		drawFps(&screenSDL, timeTicks, 4, SDL_SCREEN_HEIGHT - 32, 4);

		SDL_UpdateWindowSurface(window);

		if (vsync) {
			do {} while (SDL_GetTicks() - timeTicks < 20);
		}
	} while (!quit);


	SDL_Quit();

    return 0;
}
