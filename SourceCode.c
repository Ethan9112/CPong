#include <SDL2/SDL.h>
#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define SCORE_BAR_HEIGHT 50
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 100
#define BALL_SIZE 10
#define PADDLE_SPEED 10
#define BALL_SPEED 5
#define TARGET_FPS 60
#define FRAME_DELAY (1000 / TARGET_FPS)

typedef struct {
    int x, y, w, h;
} Paddle;

typedef struct {
    int x, y, dx, dy, size;
} Ball;

void move_paddle(Paddle *paddle, int direction) {
    paddle->y += direction * PADDLE_SPEED;
    if (paddle->y < 0) paddle->y = 0;
    if (paddle->y + PADDLE_HEIGHT > SCREEN_HEIGHT) paddle->y = SCREEN_HEIGHT - PADDLE_HEIGHT;
}

void move_ball(Ball *ball, Paddle *left_paddle, Paddle *right_paddle) {
    ball->x += ball->dx;
    ball->y += ball->dy;

    // Bounce off top and bottom walls
    if (ball->y <= 0 || ball->y + BALL_SIZE >= SCREEN_HEIGHT) {
        ball->dy = -ball->dy;
    }

    // Bounce off paddles
    if ((ball->x <= left_paddle->x + PADDLE_WIDTH && ball->y + BALL_SIZE >= left_paddle->y && ball->y <= left_paddle->y + PADDLE_HEIGHT) ||
        (ball->x + BALL_SIZE >= right_paddle->x && ball->y + BALL_SIZE >= right_paddle->y && ball->y <= right_paddle->y + PADDLE_HEIGHT)) {
        ball->dx = -ball->dx;
    }

    // Reset ball if it goes out of bounds
    if (ball->x < 0 || ball->x > SCREEN_WIDTH) {
        ball->x = SCREEN_WIDTH / 2 - BALL_SIZE / 2;
        ball->y = SCREEN_HEIGHT / 2 - BALL_SIZE / 2;
        ball->dx = (ball->dx > 0 ? 1 : -1) * BALL_SPEED;
        ball->dy = (ball->dy > 0 ? 1 : -1) * BALL_SPEED;
    }
}

void render_fps(SDL_Renderer *renderer, TTF_Font *font, int fps) {
    char fps_text[16];
    snprintf(fps_text, sizeof(fps_text), "FPS: %d", fps);

    SDL_Color white = {255, 255, 255, 255}; // Text color
    SDL_Surface *surface = TTF_RenderText_Solid(font, fps_text, white);
    if (!surface) {
        SDL_Log("Failed to create FPS surface: %s", TTF_GetError());
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_Log("Failed to create FPS texture: %s", SDL_GetError());
        SDL_FreeSurface(surface); // Ensure surface is freed even on failure
        return;
    }

    SDL_Rect dst_rect = {10, 10, surface->w, surface->h}; // Position of the FPS text
    SDL_RenderCopy(renderer, texture, NULL, &dst_rect);

    SDL_FreeSurface(surface); // Free surface after rendering
    SDL_DestroyTexture(texture); // Free texture after rendering
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        SDL_Log("Unable to initialize SDL_ttf: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Could not create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 24);
    if (!font) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    Paddle left_paddle = {10, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
    Paddle right_paddle = {SCREEN_WIDTH - 20, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
    Ball ball = {SCREEN_WIDTH / 2 - BALL_SIZE / 2, SCREEN_HEIGHT / 2 - BALL_SIZE / 2, BALL_SPEED, BALL_SPEED, BALL_SIZE};

    bool running = 1; // Use true as defined in <stdbool.h>
    SDL_Event event;

    Uint32 frame_start, frame_time;
    int fps = 0, frame_count = 0;
    Uint32 fps_timer = SDL_GetTicks();

    while (running) {
        frame_start = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0; // Quit when the window is closed
            }
        }

        const Uint8 *keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_W]) move_paddle(&left_paddle, -1);
        if (keystate[SDL_SCANCODE_S]) move_paddle(&left_paddle, 1);
        if (keystate[SDL_SCANCODE_UP]) move_paddle(&right_paddle, -1);
        if (keystate[SDL_SCANCODE_DOWN]) move_paddle(&right_paddle, 1);
        if (keystate[SDL_SCANCODE_Q]) running = 0; // Quit when Q is pressed

        move_ball(&ball, &left_paddle, &right_paddle);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect left_rect = {left_paddle.x, left_paddle.y, left_paddle.w, left_paddle.h};
        SDL_Rect right_rect = {right_paddle.x, right_paddle.y, right_paddle.w, right_paddle.h};
        SDL_Rect ball_rect = {ball.x, ball.y, ball.size, ball.size};
        SDL_RenderFillRect(renderer, &left_rect);
        SDL_RenderFillRect(renderer, &right_rect);
        SDL_RenderFillRect(renderer, &ball_rect);

        render_fps(renderer, font, fps);

        SDL_RenderPresent(renderer);

        frame_count++;
        if (SDL_GetTicks() - fps_timer >= 1000) {
            fps = frame_count;
            frame_count = 0;
            fps_timer = SDL_GetTicks();
        }

        frame_time = SDL_GetTicks() - frame_start;
        if (FRAME_DELAY > frame_time) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}