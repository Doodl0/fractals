#include "SDL3/SDL_events.h"
#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_thread.h"
#include "SDL3/SDL_timer.h"
#include <stddef.h>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

// Constant resolution for now
#define X_RESOLUTION 800
#define Y_RESOLUTION 600

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Surface *surface = NULL;

float last_zoom = 0.0f;

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

// Hopefully allows the fractals to be dynamically zoomed into and out of
float zoom = 1.0f;
float input_zoom(const SDL_Event *event, float zoom) {
    if (event->key.scancode == SDL_SCANCODE_UP) {
        zoom -= 0.1f;
    }
    else if (event->key.scancode == SDL_SCANCODE_DOWN) {
        zoom += 0.1f;
    }
    return zoom;
}

// Mandelbrot function shamelessly copied from wikipedia
int Mandelbrot(int Px, int Py) {
    // Scaled x coordinate of pixel (scaled to lie in the Mandelbrot X scale (-2.00, 0.47))
    float x0 = (((float)Px / (float)X_RESOLUTION) * 2.47f - 2.0f) * zoom;
    // Scaled y coordinare of pixel (scaled to lie in the Mandelbrot Y scale (-1.12, 1.12))
    float y0 = ((float)Py / ((float)Y_RESOLUTION) * 2.24f - 1.0f) * zoom;

    float x, y = 0;
    int iteration = 0;
    int max_iteration = 500;

    while (x*x + y*y <= ( 1 << 16) && iteration < max_iteration) {
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
    zoom = input_zoom(event, zoom);
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    // Only render new frame if zoom changes - saves a lot of performance because my code is slow
    if (zoom != last_zoom) {
        int iteration_counts[X_RESOLUTION][Y_RESOLUTION] = {};
        int num_iteration_per_pixel[255] = {};

        SDL_LockSurface(surface);
        for (int x = 0; x < X_RESOLUTION; x++) {
            for (int y = 0; y < Y_RESOLUTION; y++) {
                int iteration = Mandelbrot(x, y);
                //iteration_counts[x][y] = iteration;
                //num_iteration_per_pixel[iteration]++;

                SDL_WriteSurfacePixel(surface, x, y, 0, 0, iteration / 2, 255);
            }
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderTexture(renderer, texture, NULL,NULL);
        SDL_RenderPresent(renderer);

        SDL_UnlockSurface(surface);
        SDL_DestroyTexture(texture);
        last_zoom = zoom;
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
