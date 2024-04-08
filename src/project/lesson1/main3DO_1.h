#ifndef MAIN_3DO_1_H
#define MAIN_3DO_1_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "graphics.h"
#include "displayutils.h"
#include "operamath.h"
#include "mem.h"
#include "math.h"
#include "io.h"
#include "event.h"
#include "task.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_SIZE_IN_BYTES (SCREEN_WIDTH * SCREEN_HEIGHT * 2)
#define SCREEN_PAGES 2

int main3DO_1();

#endif
