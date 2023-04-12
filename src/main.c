#include <stdbool.h>

#define WIDTH 800
#define HEIGHT 600
#define NUM_THREADS 4

typedef struct {
    void* threads;
    int thread_num;
} ThreadArg;

typedef struct Threads {
    SDL_Thread* threads[NUM_THREADS];
    ThreadArg args[NUM_THREADS];
    SDL_mutex* lock;
    SDL_cond* cond;
    unsigned counter;
    int remaining;
    bool done;
} Threads;

World* world;
char map[] = 
    "rrrrrrrrrrrrrrrrrrrrrrrrrrrrrr"
    "b                 g          g"
    "b                 g          g"
    "b   P             g          g"
    "b                 g          g"
    "b                 g          g"
    "b                 g          g"
    "b     ggggggggggggg     #####g"
    "b                 g          g"
    "b  r              g          g"
    "b                 g          g"
    "b                            g"
    "b                            g"
    "b    gb           rrrrr    bbg"
    "b                 r          g"
    "b                 r          g"
    "b                 r          g"
    "b                 r          g"
    "b###################  ########";

const int world_width = 30;
const int world_height = 19;
const double world_scale = 10;

Position player_position;
double player_angle = 0;
const double player_speed = 70;
const double mouse_sensitivity = 20;

const double fov = 100;
double half_fov;
double focus_to_image;

const double max_fog_distance = 20;
const double min_fog_distance = 2;
const unsigned int fog_color = 0x87CEEB;
const unsigned int light_color = 0xFFFFFF;

Window window = NULL;
char texture_data[WIDTH*HEIGHT*3];

bool quit = false;
SDL_Event event;

bool keydown_w = false;
bool keydown_a = false;
bool keydown_s = false;
bool keydown_d = false;
bool keydown_left = false;
bool keydown_right = false;

int mouse_move_x = 0;

unsigned int lerpColor(unsigned int color_1, unsigned int color_2, double lin_val) {

    unsigned int newColor = 0x000000;
    unsigned int mask = 0xFF;

    newColor |= (unsigned char) (((float)((color_1 >> 16)&mask) * (1-lin_val)) + ((float)((color_2 >> 16)&mask) * lin_val));
    newColor <<= 8;
    newColor |= (unsigned char) (((float)((color_1 >> 8)&mask) * (1-lin_val)) + ((float)((color_2 >> 8)&mask) * lin_val));
    newColor <<= 8;
    newColor |= (unsigned char) (((float)((color_1 >> 0)&mask) * (1-lin_val)) + ((float)((color_2 >> 0)&mask) * lin_val));

    return newColor;
}

double getFogAmount(double depth) {

    return (depth > min_fog_distance) ? MIN((depth-min_fog_distance)/(max_fog_distance-min_fog_distance), 0.8) : 0;
}

void renderScene(int thread_num) {

    float thread_div = (float) WIDTH / NUM_THREADS;

    int thread_start = thread_div * thread_num;
    int thread_end = thread_div * (thread_num + 1);

    for (int x = thread_start; x < thread_end; x++) {

        Ray ray = worldCastRay(world, player_position, player_angle + SDL_atan((x-(WIDTH/2))/focus_to_image), player_angle);

        ray.color = lerpColor(light_color, ray.color, SDL_sqrt(SDL_sin(ray.angle_of_incidence))); // Specular highlight

        int wall_height = (int) (( HEIGHT / (ray.depth)));

        double fog_amount = getFogAmount(ray.depth);
        
        if (fog_amount > 0) {

            ray.color = lerpColor(ray.color, fog_color, fog_amount);
        }

        for (int y = 0; y < HEIGHT; y++) {

            if (y > (HEIGHT-wall_height)/2 && y < wall_height+(HEIGHT-wall_height)/2) {

                texture_data[(y*WIDTH + x)*3 + 0] = ray.color >> 16;
                texture_data[(y*WIDTH + x)*3 + 1] = ray.color >> 8;
                texture_data[(y*WIDTH + x)*3 + 2] = ray.color >> 0;

            } else {

                if (y < HEIGHT/2) {

                    // Sky color
                    texture_data[(y*WIDTH + x)*3 + 0] = 0x87;
                    texture_data[(y*WIDTH + x)*3 + 1] = 0xCE;
                    texture_data[(y*WIDTH + x)*3 + 2] = 0xEB;

                } else {
                    
                    double floor_depth = HEIGHT / ((y - HEIGHT/2.0f)*2.0f); // Opposite of the wall_height equation

                    int floor_color = 0x202020;

                    double floor_fog = getFogAmount(floor_depth);
                    
                    if (floor_fog > 0) {

                        floor_color = lerpColor(floor_color, fog_color, floor_fog);
                    }

                    // Floor color
                    texture_data[(y*WIDTH + x)*3 + 0] = floor_color >> 16;
                    texture_data[(y*WIDTH + x)*3 + 1] = floor_color >> 8;
                    texture_data[(y*WIDTH + x)*3 + 2] = floor_color >> 0;
                }
            }
        }
    }
}

int renderMulti(void* ptr)
{
    ThreadArg* arg = ptr;
    Threads* threads = arg->threads;
    int thread_num = arg->thread_num;

    for (unsigned counter = 0;; counter++) {
        SDL_LockMutex(threads->lock);
        while (threads->counter == counter) {
            SDL_CondWait(threads->cond, threads->lock);
        }
        bool done = threads->done;
        SDL_UnlockMutex(threads->lock);
        if (done) {
            return 0;
        }

        renderScene(thread_num);

        SDL_LockMutex(threads->lock);
        if (!--threads->remaining) {
            SDL_CondSignal(threads->cond);
        }
        SDL_UnlockMutex(threads->lock);
    }
}

void threadsInit(Threads* threads) {

    threads->counter = 0;
    threads->done = false;
    threads->lock = SDL_CreateMutex();
    threads->cond = SDL_CreateCond();
    for (int i = 0; i < NUM_THREADS; i++) {
        ThreadArg* arg = threads->args + i;
        arg->threads = threads;
        arg->thread_num = i;
        threads->threads[i] = SDL_CreateThread(renderMulti, NULL, arg);
    }
}

void threadsDestroy(Threads* threads) {

    SDL_LockMutex(threads->lock);
    threads->counter++;
    threads->done = true;
    SDL_UnlockMutex(threads->lock);
    SDL_CondBroadcast(threads->cond);
    for (int i = 0; i < NUM_THREADS; i++) {
        SDL_WaitThread(threads->threads[i], NULL);
    }
    SDL_DestroyCond(threads->cond);
    SDL_DestroyMutex(threads->lock);
}

void render(Threads *threads) {

    SDL_LockMutex(threads->lock);
    threads->counter++;
    threads->remaining = NUM_THREADS;
    SDL_CondBroadcast(threads->cond);
    do {
        SDL_CondWait(threads->cond, threads->lock);
    } while (threads->remaining);
    SDL_UnlockMutex(threads->lock);

    SDL_UpdateTexture(window->pixels, NULL, texture_data, WIDTH*3);
    SDL_RenderCopy(window->renderer, window->pixels, NULL, NULL);
    SDL_RenderPresent(window->renderer);
}

void updatePlayer(uint64_t delta) {

    // Mouse movement
    player_angle += mouse_move_x/1000.0 * mouse_sensitivity;
    
    // Arrow movement
    if (keydown_left != keydown_right) {

        float change = (keydown_left) ? -1 : 1;
        float arrow_speed = 3.5f;
        player_angle += (delta/1000.0)*change*arrow_speed;
    }

    float x_fraction;
    float y_fraction;

    float mult = (delta/1000.0)*player_speed;

    if ((keydown_w != keydown_s)) {

        mult = keydown_s ? mult*-1 : mult;

        x_fraction = -SDL_sin(player_angle);
        y_fraction = SDL_cos(player_angle);

        player_position.x += x_fraction*mult;
        player_position.y += y_fraction*mult;
    }

    if ((keydown_a != keydown_d)) {

        mult = keydown_s ? mult*-1 : mult;

        float turn_angle = keydown_d ? PI_2 : -PI_2;

        x_fraction = -SDL_sin(player_angle + turn_angle);
        y_fraction = SDL_cos(player_angle + turn_angle);

        player_position.x += x_fraction*mult;
        player_position.y += y_fraction*mult;
    }
}

void pollEvents() {

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                // User clicked the close button, exit loop
                SDL_Log("Quit event received\n");
                quit = true;
                break;

            case SDL_MOUSEMOTION:

                mouse_move_x = event.motion.xrel;
                break;

            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {

                    case SDLK_ESCAPE:
                        quit = true;
                        break;

                    case SDLK_w:
                        keydown_w = true;
                        break;

                    case SDLK_a:
                        keydown_a = true;
                        break;

                    case SDLK_s:
                        keydown_s = true;
                        break;

                    case SDLK_d:
                        keydown_d = true;
                        break;
                    
                    case SDLK_LEFT:
                        keydown_left = true;
                        break;
                    
                    case SDLK_RIGHT:
                        keydown_right = true;
                        break;
                }
                break;

            case SDL_KEYUP:
                switch(event.key.keysym.sym) {

                    case SDLK_w:
                        keydown_w = false;
                        break;

                    case SDLK_a:
                        keydown_a = false;
                        break;

                    case SDLK_s:
                        keydown_s = false;
                        break;

                    case SDLK_d:
                        keydown_d = false;
                        break;

                    case SDLK_LEFT:
                        keydown_left = false;
                        break;
                    
                    case SDLK_RIGHT:
                        keydown_right = false;
                        break;
                }
                break;

            default:
                // Ignore other events
                break;
        }
    }
}

int main(int argv, char** args) {

    window = windowCreate(WIDTH, HEIGHT);

    world = worldCreate(map, world_width, world_height, world_scale);
    player_position = worldGetPlayerPosition(world);

    if (!windowInit(window)) {

        SDL_Log("Failed to initialize SDL\n");

    } else {

        SDL_Log("SDL initialized\n");

        window->pixels = SDL_CreateTexture(
            window->renderer,
            SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
            WIDTH, HEIGHT
        );

        half_fov = (fov/ 180.0f * PI)/2.0f;
        focus_to_image = (WIDTH/2)/SDL_tan(half_fov);

        Threads threads;
        threadsInit(&threads);

        uint64_t last_frame = SDL_GetTicks64();
        uint64_t current_frame = SDL_GetTicks64();
        uint64_t delta_time = 0;

        while (!quit) {

            current_frame = SDL_GetTicks64();
            delta_time = current_frame - last_frame;
            last_frame = current_frame;

            // Poll for events
            pollEvents();
            updatePlayer(delta_time);

            render(&threads);

            mouse_move_x = 0;
        }

        threadsDestroy(&threads);
    }  

    windowDestroy(window);

    return 0;
}