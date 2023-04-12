typedef struct {
    char* map;
    int map_width;
    int map_height;
    double scale;
} World;

typedef struct {
    double x;
    double y;
} Position;

typedef struct {
    double depth;
    unsigned int color;
    double angle_of_incidence;
} Ray;

enum Side {
    SOUTH,
    NORTH,
    EAST,
    WEST
};

World* worldCreate(char* map, int width, int height, double scale) {

    World* world = SDL_malloc(sizeof(World));

    world->map = map;
    world->map_width = width;
    world->map_height = height;
    world->scale = scale;

    return world;
}

unsigned int getRayColor(unsigned char symbol) {

    switch(symbol) {
        case 'r':
            return 0xFF2222;
        case 'g':
            return 0x22FF22;
        case 'b':
            return 0x2222FF;
        default:
            return 0x222222;
    }
}

double getAngleOfIncidence(enum Side side, double angle) {

    switch(side) {
        case WEST:
            return PI_2 - (angle - PI - PI_2);
        case EAST:
            return PI_2 - (angle - PI_2);
        case NORTH:
            return PI_2 - angle;
        case SOUTH:
            return PI_2 - (angle - PI);
        default:
            return 0;
    }
}

enum Side getSideHit(int x, int y, int prev_x, int prev_y) {

    if (x > prev_x) {
        return WEST;
    }
    if (x < prev_x) {
        return EAST;
    }
    if (y > prev_y) {
        return NORTH;
    }
    if (y < prev_y) {
        return SOUTH;
    }
    SDL_assert_release(0);
    return -1;
}

double getDepth(Position player_position, int map_x, int map_y, double delta_x, double delta_y, double scale, enum Side side) {

    if (delta_x == 0) {

        int add_height = (delta_y < 0) ? 1 : 0; // When casting in negative directions, you need to account for the width of map cells
        return SDL_fabs((map_y + add_height)*scale - player_position.y);
    }

    if (delta_y == 0) {

        int add_width = (delta_x < 0) ? 1 : 0;
        return SDL_fabs((map_x + add_width)*scale - player_position.x);
    }

    if (side == NORTH || side == SOUTH) {

        double slope = delta_y/delta_x;
        int add_height = (side == SOUTH) ? 1 : 0;

        double y = (map_y + add_height) * scale;
        double x = y/slope + player_position.x - player_position.y/slope;

        return SDL_sqrt((player_position.x - x)*(player_position.x - x) + (player_position.y - y)*(player_position.y - y));
    }

    if (side == EAST || side == WEST) {

        double slope = delta_x/delta_y;
        int add_width = (side == EAST) ? 1 : 0;

        double x = (map_x + add_width) * scale;
        double y = x/slope + player_position.y - player_position.x/slope;

        return SDL_sqrt((player_position.x - x)*(player_position.x - x) + (player_position.y - y)*(player_position.y - y));
    }
    SDL_assert_release(0);
    return -1;
}

Ray worldCastRay(World* world, Position position, double ray_angle, double player_angle) {

    Ray ray;

    int map_x = (int) (position.x/world->scale);
    int map_y = (int) (position.y/world->scale);

    int prev_map_x;
    int prev_map_y;

    double march_x = 0;
    double march_y = 0;

    double detail = 10; // This will determine how small the steps are when doing a raycast
    // Detail values too low will cause corners of cells be be lost due to resampling

    double delta_x = -SDL_sin(ray_angle)/detail;
    double delta_y = SDL_cos(ray_angle)/detail;

    char curr;

    do {
        march_x += delta_x;
        march_y += delta_y;

        prev_map_x = map_x;
        prev_map_y = map_y;

        map_x = (int) ((position.x+march_x)/world->scale);
        map_y = (int) ((position.y+march_y)/world->scale);

        if (map_x < 0 || map_x >= world->map_width || map_y < 0 || map_y >= world->map_height) {

            ray.depth = 1000000;
            ray.color = 0x000000;
            
            return ray;
        }

        curr = world->map[map_y*world->map_width + map_x];

    } while (curr == ' ' || curr == 'P');

    if ((map_x != prev_map_x) && (map_y != prev_map_y)) {
        // There are cases where the ray passes perfectly through a corner and the side of the block that was hit cannot be known
        // In these cases, do the ray cast in a very slightly different direction
        return worldCastRay(world, position, ray_angle - 0.0001, player_angle);

    } else {

        enum Side side = getSideHit(map_x, map_y, prev_map_x, prev_map_y);

        double raw_depth = getDepth(position, map_x, map_y, delta_x, delta_y, world->scale, side)/world->scale;
        ray.depth = SDL_fabs(SDL_cos(player_angle - ray_angle)*raw_depth); // Find the distance to the plane of the player rather than to the player
        ray.color = getRayColor(curr);
        ray.angle_of_incidence = getAngleOfIncidence(side, ray_angle);
    }
    
    return ray;
}

Position worldGetPlayerPosition(World* world) {

    Position position;

    for (int y = 0; y < world->map_height; y++) {
        for (int x = 0; x < world->map_width; x++) {

            if (world->map[y*world->map_width + x] == 'P') {

                position.x = x*world->scale;
                position.y = y*world->scale;

                return position;
            }
        }
    }

    position.x = -1;
    position.y = -1;

    return position;
}