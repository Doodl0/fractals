#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_surface.h"
#include <stddef.h>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <math.h>

// Constant resolution for now
#define X_RESOLUTION 800
#define Y_RESOLUTION 600
#define MAX_ITERATIONS 255

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Surface *surface = NULL;

typedef enum DisplayScreen {
    MENU,
    MANDELBROT_SET,
    SIERPINSKI_TRIANGLE,
    KOCH_SNOWFLAKE,
} DisplayScreen;

DisplayScreen current_screen = 0;

// Hopefully allows the fractals to be dynamically zoomed into and out of, and shifted left right up and down
// Last positions and zooms are to be used for stopping rendering when nothing changes between frames
float zoom = 1.0f;
float last_zoom = 0.0f;
int x_offset = 400;
int y_offset = 300;
int x_last_offset, y_last_offset = 0;

// Draw a simple selection menu with debug text to allow the user to select a fractal
void DrawMenu() {

    // Set draw colour to white
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    // Increase scale for title (Debug text is only at 16x16)
    SDL_SetRenderScale(renderer, 5, 5);
    SDL_RenderDebugText(renderer, 10, 10, "Fractals Menu");

    // Decrease render scale for options
    SDL_SetRenderScale(renderer, 3, 3);

    // Split line as too long
    SDL_RenderDebugText(renderer, 16, 40, "Press keys 1, 2, 3 or 4");
    SDL_RenderDebugText(renderer, 16, 50, "to select an option");

    // List options
    SDL_RenderDebugText(renderer, 16, 70, "1. Mandelbrot Set");
    SDL_RenderDebugText(renderer, 16, 90, "2. Sierpinski Triangle");
    SDL_RenderDebugText(renderer, 16, 110, "3. Koch Snowflake");
    SDL_RenderDebugText(renderer, 16, 130, "4. Quit");

    // Render
    SDL_RenderPresent(renderer);
}

SDL_AppResult MenuInput(SDL_Event *event) {
    if (event->key.scancode == SDL_SCANCODE_1) {
        current_screen = MANDELBROT_SET;
    }
    else if (event->key.scancode == SDL_SCANCODE_2) {
        current_screen = SIERPINSKI_TRIANGLE;
    }
    else if (event->key.scancode == SDL_SCANCODE_3) {
        current_screen = KOCH_SNOWFLAKE;
    }
    else if (event->key.scancode == SDL_SCANCODE_4) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

// Mandelbrot function shamelessly copied from wikipedia
int Mandelbrot(int Px, int Py) {
    // Scaled x coordinate of pixel (scaled to lie in the Mandelbrot X scale (-2.00, 0.47))
    float x0 = ((((float)(Px - (X_RESOLUTION / 2)) * zoom + x_offset  ) / (float)X_RESOLUTION) * 2.47f - 2.0f) ;
    // Scaled y coordinare of pixel (scaled to lie in the Mandelbrot Y scale (-1.12, 1.12))
    float y0 = (((float)(Py - (Y_RESOLUTION / 2)) * zoom + y_offset ) / ((float)Y_RESOLUTION) * 2.24f - 1.0f) ;

    float x, y = 0;
    int iteration = 0;

    while (x*x + y*y <= ( 1 << 16) && iteration < MAX_ITERATIONS) {
        float xtemp = x*x - y*y + x0;
        y = 2*x*y + y0;
        x = xtemp;
        iteration += 1;
    }

    return iteration;
}

void DrawMandelbrot() {
    // Only render new frame if zoom or offset changes - saves a lot of performance because my code is slow
    if (zoom != last_zoom || x_offset != x_last_offset || y_offset != y_last_offset) {

        last_zoom = zoom;
        x_last_offset = x_offset;
        y_last_offset = y_offset;

        SDL_LockSurface(surface);
        // Loop over every pixel like a fragment shader
        for (int x = 0; x < X_RESOLUTION; x++) {
            for (int y = 0; y < Y_RESOLUTION; y++) {
                int iteration = Mandelbrot(x, y);
                if (iteration >= MAX_ITERATIONS) {
                    SDL_WriteSurfacePixel(surface, x, y, 0, 0, 0, 255);
                }
                else {
                    // Assign colours
                    float t = (float)iteration / MAX_ITERATIONS;
                    Uint8 r = (Uint8)(9 * (1 - t) * t * t * t * 255);
                    Uint8 g = (Uint8)(15 * (1 - t) * (1 - t) * t * t * 255);
                    Uint8 b = (Uint8)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
                    SDL_WriteSurfacePixel(surface, x, y, r, g, b, 255);
                }
            }
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderTexture(renderer, texture, NULL,NULL);
        SDL_RenderPresent(renderer);

        SDL_UnlockSurface(surface);
        SDL_DestroyTexture(texture);

    }
    else {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderTexture(renderer, texture, NULL,NULL);
        SDL_RenderPresent(renderer);

        SDL_DestroyTexture(texture);
    }

}

void MandelbrotInput(SDL_Event *event) {
    if (event->key.scancode == SDL_SCANCODE_EQUALS || event->key.scancode == SDL_SCANCODE_KP_PLUS) {
        zoom -= 0.05f;
    }
    else if (event->key.scancode == SDL_SCANCODE_MINUS || event->key.scancode == SDL_SCANCODE_KP_MINUS) {
        zoom += 0.05f;
    }
    else if (event->key.scancode == SDL_SCANCODE_UP) {
        y_offset -= 25;
    }
    else if (event->key.scancode == SDL_SCANCODE_DOWN) {
        y_offset += 25;
    }
    else if (event->key.scancode == SDL_SCANCODE_LEFT) {
        x_offset -= 25;
    }
    else if (event->key.scancode == SDL_SCANCODE_RIGHT) {
        x_offset += 25;
    }
}

void DrawSierpinksi() {

}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    //current_screen = MENU;
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Fractals", X_RESOLUTION, Y_RESOLUTION, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create surface
    surface = SDL_CreateSurface(X_RESOLUTION, Y_RESOLUTION, SDL_PIXELFORMAT_RGBA8888);

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    // Only keydown otherwise double input from keyup
    if (event->type == SDL_EVENT_KEY_DOWN) {
        // If in the menu, listen for number keys to change the screen or quit
        if (current_screen == MENU) {
            return MenuInput(event);
        }
        else {
            MandelbrotInput(event);
        }
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    // Switch through to get current screen and draw
    switch (current_screen) {
        case MENU: DrawMenu(); break;
        case MANDELBROT_SET: DrawMandelbrot(); break;
        case SIERPINSKI_TRIANGLE: {}; break;
        case KOCH_SNOWFLAKE: {}; break;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{

}
