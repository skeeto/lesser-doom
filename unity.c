// $ cc -o lesser-doom unity.c $(sdl2-config --cflags --libs)
#include <stdbool.h>
#include "SDL.h"

#define PI   3.1415926535897931
#define PI_2 1.5707963267948966
#define MIN(x, y) ((x)<(y) ? (x) : (y))

#include "src/window.c"
#include "src/world.c"
#include "src/main.c"
