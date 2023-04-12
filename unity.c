// $ eval cc -o lesser-doom unity.c $(pkg-config --cflags --libs glew sdl2)
#include <stdbool.h>
#include <GL/glew.h>
#include "SDL.h"

#define PI   3.1415926535897931
#define PI_2 1.5707963267948966
#define MIN(x, y) ((x)<(y) ? (x) : (y))

#include "src/shader.c"
#include "src/window.c"
#include "src/world.c"
#include "src/main.c"
