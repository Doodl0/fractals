#include "SDL3/SDL_events.h"
#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_thread.h"
#include "SDL3/SDL_timer.h"
#include <stddef.h>
#include <xkbcommon/xkbcommon.h>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

// Constant resolution for now
#define X_RESOLUTION 800
#define Y_RESOLUTION 600
#define MAX_ITERATIONS 255

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Surface *surface = NULL;


// Hopefully allows the fractals to be dynamically zoomed into and out of, and shifted left right up and down
// Last positions and zooms are to be used for stopping rendering when nothing changes between frames
float zoom = 1.0f;
float last_zoom = 0.0f;
int x_offset = 400;
int y_offset = 300;
int x_last_offset, y_last_offset = 0;

typedef struct {
    int x;
    int y;
} Pixel;

// hardcoded max data because i'm stupid
typedef struct {
    int width;
    int height;
    int start_x;
    int start_y;
    SDL_Color pixel_data[X_RESOLUTION][Y_RESOLUTION];
} ChunkData;


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

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
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
    // Only keydown otherwise double input from keyup
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_EQUALS || event->key.scancode == SDL_SCANCODE_KP_PLUS) {
            zoom -= 0.01f;
        }
        else if (event->key.scancode == SDL_SCANCODE_MINUS || event->key.scancode == SDL_SCANCODE_KP_MINUS) {
            zoom += 0.01f;
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
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{

    // Only render new frame if zoom or offset changes - saves a lot of performance because my code is slow
    if (zoom != last_zoom || x_offset != x_last_offset || y_offset != y_last_offset) {

        last_zoom = zoom;
        x_last_offset = x_offset;
        y_last_offset = y_offset;
        int iteration_counts[X_RESOLUTION][Y_RESOLUTION] = {};
        int num_iteration_per_pixel[255] = {};

        SDL_LockSurface(surface);
        for (int x = 0; x < X_RESOLUTION; x++) {
            for (int y = 0; y < Y_RESOLUTION; y++) {
                int iteration = Mandelbrot(x, y);
                if (iteration >= MAX_ITERATIONS) {
                    SDL_WriteSurfacePixel(surface, x, y, 0, 0, 0, 255);
                }
                else {
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

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{

}
