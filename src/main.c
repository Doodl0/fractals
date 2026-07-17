#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_thread.h"
#include <stddef.h>
#include <math.h>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

// Windows fixes
#ifdef _WIN32
    #define _USE_MATH_DEFINES
#endif
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Constant resolution for now
#define X_RESOLUTION 800
#define Y_RESOLUTION 600

// Max iterations for Mandelbrot set - 255 is an easy number for pixel calculations
#define MAX_ITERATIONS 255
#define RENDER_DEPTH 7
#define THREADS 8
#define USE_THREADS true

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Surface *surface = NULL;
SDL_Surface *thread_surfaces[THREADS] = {};

// Enum for current program state - which screen is currently displayed
typedef enum DisplayScreen {
    MENU,
    MANDELBROT_SET,
    SIERPINSKI_TRIANGLE,
    KOCH_SNOWFLAKE,
} DisplayScreen;

// Thread data struct for multithreading rendering
typedef struct ThreadData {
    int x_resolution;
    int y_resolution;
    int start_x;
    int start_y;
    int x_offset;
    int y_offset;
    float zoom;
    SDL_Surface *surface;
} ThreadData;

// Point struct for line drawing because IDK how to use SDL_Point
typedef struct Point {
    int x;
    int y;
} Point;

// Set starting screen to MENU
DisplayScreen current_screen = 0;

// Hopefully allows the fractals to be dynamically zoomed into and out of, and shifted left right up and down
// Last positions and zooms are to be used for stopping rendering when nothing changes between frames
float zoom = 1.0f;
float last_zoom = 0.0f;
int x_offset = X_RESOLUTION / 2;
int y_offset = Y_RESOLUTION / 2;
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

// Handle keydown inputs while the MENU is the active screen
SDL_AppResult MenuInput(SDL_Event *event) {
    // 1 Key -> Mandelbrot
    if (event->key.scancode == SDL_SCANCODE_1) {
        current_screen = MANDELBROT_SET;
    }
    // 2 Key -> Sierpinski
    else if (event->key.scancode == SDL_SCANCODE_2) {
        current_screen = SIERPINSKI_TRIANGLE;
    }
    // 3 Key -> Koch
    else if (event->key.scancode == SDL_SCANCODE_3) {
        current_screen = KOCH_SNOWFLAKE;
    }
    // 4 Key -> return a success result so that the app shuts down
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

    // Initialise x, y and iteration values as 0
    float x = 0, y = 0;
    int iteration = 0;

    // Optimized time escape algorithm
    float x2 = 0, y2 = 0;
    while (x2 + y2 <= 4 && iteration < MAX_ITERATIONS) {
        x2 = x * x;
        y2 = y * y;
        y = 2 * x * y + y0;
        x = x2 - y2 + x0;
        iteration += 1;
    }

    return iteration;
}

// Draw Mandelbrot to the screen
void DrawMandelbrot() {
    // Reset render scale in case it carries over from previous screen
    SDL_SetRenderScale(renderer, 1, 1);
    // Clear screen with black just in case
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    // Only render new frame if zoom or offset changes - saves a lot of performance kinda because my code is slow and single threaded
    if (zoom != last_zoom || x_offset != x_last_offset || y_offset != y_last_offset) {
        // Lock surface to allow writing to the pixels
        SDL_LockSurface(surface);
        // Loop over every pixel like a fragment shader
        for (int x = 0; x < X_RESOLUTION; x++) {
            for (int y = 0; y < Y_RESOLUTION; y++) {
                // Caculate the Mandelbrot iteration of the current pixel
                int iteration = Mandelbrot(x, y);
                // If pixel escapes, just write black to the surface
                if (iteration >= MAX_ITERATIONS) {
                    SDL_WriteSurfacePixel(surface, x, y, 0, 0, 0, 255);
                }
                else {
                    // Assign colours and write to the surface
                    float t = (float)iteration / MAX_ITERATIONS;
                    Uint8 r = (Uint8)(9 * (1 - t) * t * t * t * 255);
                    Uint8 g = (Uint8)(15 * (1 - t) * (1 - t) * t * t * 255);
                    Uint8 b = (Uint8)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
                    SDL_WriteSurfacePixel(surface, x, y, r, g, b, 255);
                }
            }
            // Set last zoom and offset to current so that the program knows nothing has changed between this frame and the next
            last_zoom = zoom;
            x_last_offset = x_offset;
            y_last_offset = y_offset;
        }

        // Create texture from surface and render it
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        const SDL_FRect rect = (SDL_FRect) {
            0.0f,
            0.0f + 0 * (1.0f / (float)THREADS),
            1.0f,
            1.0f / (float)THREADS
        };
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Destroy texture to free memory
        SDL_UnlockSurface(surface);
        SDL_DestroyTexture(texture);

    }
    else {
        // Create texture from surface and render it
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderTexture(renderer, texture, NULL,NULL);
        SDL_RenderPresent(renderer);

        // Destroy texture to free memory
        SDL_DestroyTexture(texture);
    }

}

int MandelbrotThreaded(void *data) {
    ThreadData* thread_data = (ThreadData* )data;
    if (thread_data == NULL) {
        return -1;
    }
    SDL_LockSurface(thread_data->surface);

    // Loop over every pixel like a fragment shader
    for (int Px = thread_data->start_x; Px < thread_data->x_resolution + thread_data->start_x; Px++) {
        for (int Py = thread_data->start_y; Py < thread_data->y_resolution + thread_data->start_y; Py++) {
            // Caculate the Mandelbrot iteration of the current pixel

            // Scaled x coordinate of pixel (scaled to lie in the Mandelbrot X scale (-2.00, 0.47))
            float x0 = ((((float)(Px - (X_RESOLUTION / 2)) * thread_data->zoom + thread_data->x_offset  ) / (float)X_RESOLUTION) * 2.47f - 2.0f) ;
            // Scaled y coordinare of pixel (scaled to lie in the Mandelbrot Y scale (-1.12, 1.12))
            float y0 = (((float)(Py - (Y_RESOLUTION / 2)) * thread_data->zoom + thread_data->y_offset ) / ((float)Y_RESOLUTION) * 2.24f - 1.0f) ;

            // Initialise x, y and iteration values as 0
            float x = 0, y = 0;
            int iteration = 0;

            // Optimized time escape algorithm
            float x2 = 0, y2 = 0;
            while (x2 + y2 <= 4 && iteration < MAX_ITERATIONS) {
                x2 = x * x;
                y2 = y * y;
                y = 2 * x * y + y0;
                x = x2 - y2 + x0;
                iteration += 1;
            }

            // If pixel escapes, just write black to the surface
            if (iteration >= MAX_ITERATIONS) {
                SDL_WriteSurfacePixel(thread_data->surface, Px - thread_data->start_x, Py - thread_data->start_y, 0, 0, 0, 255);
            }
            else {
                // Assign colours and write to the surface
                float t = (float)iteration / MAX_ITERATIONS;
                Uint8 r = (Uint8)(9 * (1 - t) * t * t * t * 255);
                Uint8 g = (Uint8)(15 * (1 - t) * (1 - t) * t * t * 255);
                Uint8 b = (Uint8)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
                SDL_WriteSurfacePixel(thread_data->surface, Px - thread_data->start_x, Py - thread_data->start_y, r, g, b, 255);
            }
        }
    }

    return 0;
}

void DrawMandelbrotThreaded() {
    // Reset render scale in case it carries over from previous screen
    SDL_SetRenderScale(renderer, 1, 1);
    // Clear screen with black just in case
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Only render new frame if zoom or offset changes - saves a lot of performance kinda because my code is slow and single threaded
    if (zoom != last_zoom || x_offset != x_last_offset || y_offset != y_last_offset) {
        // Each thread has it's own surface to write to, which will then be used to draw the final image
        SDL_Thread *threads[THREADS] = {};
        ThreadData thread_data[THREADS] = {};

        // Thread surface resolution - each thread will render a horizontal slice, so x resolution will be the same
        int thread_y_resolution = (Y_RESOLUTION / THREADS);

        for (int i = 0; i < THREADS; i++) {
            // Create surface, lock it and package it with thread data so it can be written to
            thread_surfaces[i] = SDL_CreateSurface(X_RESOLUTION, thread_y_resolution, SDL_PIXELFORMAT_RGBA8888);
            SDL_LockSurface(thread_surfaces[i]);
            thread_data[i] = (ThreadData) {
                X_RESOLUTION,
                thread_y_resolution,
                0,
                i * thread_y_resolution,
                x_offset,
                y_offset,
                zoom,
                thread_surfaces[i]
            };

            // Create threads with thread data
            char thread_name[32] = "Thread";
            sprintf(thread_name, "Thread %i", i);

            threads[i] = SDL_CreateThread(MandelbrotThreaded, thread_name, &thread_data[i]);
        }

        for (int i = 0; i < THREADS; i++) {
            int return_value;
            SDL_WaitThread(threads[i], &return_value);
            //SDL_Log("Thread returned value: %d", return_value);
        }

        // Set last zoom and offset to current so that the program knows nothing has changed between this frame and the next
        last_zoom = zoom;
        x_last_offset = x_offset;
        y_last_offset = y_offset;
    }

    // Create texture from each surface and render it at the corresponding vertical offset
    for (int i = 0; i < THREADS; i++) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, thread_surfaces[i]);
        const SDL_FRect rect = (SDL_FRect) {
            0.0f,
            0.0f + i * (Y_RESOLUTION / (float)THREADS),
            X_RESOLUTION,
            Y_RESOLUTION / (float)THREADS
        };
        SDL_RenderTexture(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(renderer);
}

// Handle input while Mandelbrot is the active screen
void FractalInput(SDL_Event *event) {
    // Zoom in
    if (event->key.scancode == SDL_SCANCODE_EQUALS || event->key.scancode == SDL_SCANCODE_KP_PLUS) {
        zoom -= 0.01f;
        if (zoom <= 0) {
            zoom = 0.01f;
        }
    }
    // Zoom out
    else if (event->key.scancode == SDL_SCANCODE_MINUS || event->key.scancode == SDL_SCANCODE_KP_MINUS) {
        zoom += 0.01f;
    }
    // Shift up
    else if (event->key.scancode == SDL_SCANCODE_UP) {
        int y = 10 * zoom;
        if (y <= 0) {y = 1;}
        y_offset -= y;
    }
    // Shift down
    else if (event->key.scancode == SDL_SCANCODE_DOWN) {
        int y = 10 * zoom;
        if (y <= 0) {y = 1;}
        y_offset += y;
    }
    // Shift left
    else if (event->key.scancode == SDL_SCANCODE_LEFT) {
        int x = 10 * zoom;
        if (x <= 0) {x = 1;}
        x_offset -= x;
    }
    // Shift right
    else if (event->key.scancode == SDL_SCANCODE_RIGHT) {
        int x = 10 * zoom;
        if (x <= 0) {x = 1;}
        x_offset += x;
    }
}

void DrawTriangle(Point p1, Point p2, Point p3) {
    SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
    SDL_RenderLine(renderer, p2.x, p2.y, p3.x, p3.y);
    SDL_RenderLine(renderer, p3.x, p3.y, p1.x, p1.y);
}

void DrawIterativeTriangles(Point p1, Point p2, Point p3, int depth) {
    DrawTriangle(p1, p2, p3);
    if (depth <= 0) {return;}

    Point p4 = {(p2.x + p3.x) / 2, (p2.y + p3.y) / 2};
    Point p5 = {(p3.x + p1.x) / 2, (p3.y + p1.y) / 2};
    Point p6 = {(p1.x + p2.x) / 2, (p1.y + p2.y) / 2};

    DrawIterativeTriangles(p1, p5, p6, depth - 1);
    DrawIterativeTriangles(p4, p2, p6, depth - 1);
    DrawIterativeTriangles(p4, p5, p3, depth - 1);
}

void DrawSierpinksi() {
    SDL_SetRenderScale(renderer, 1, 1);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    // Initial vertices for a large centered triangle
    Point p1 = {(400 - x_offset) / zoom + (X_RESOLUTION / 2), (50  - y_offset) / zoom + (Y_RESOLUTION / 2)};
    Point p2 = {(150 - x_offset) / zoom + (X_RESOLUTION / 2), (483 - y_offset) / zoom + (Y_RESOLUTION / 2)};
    Point p3 = {(650 - x_offset) / zoom + (X_RESOLUTION / 2), (483 - y_offset) / zoom + (Y_RESOLUTION / 2)};

    DrawIterativeTriangles(p1,p2,p3, RENDER_DEPTH);

    SDL_RenderPresent(renderer);
}

// Draw a single Koch Curve
void DrawKochCurve(Point p1, Point p2, int depth) {
    // If depth is 0, this is the final iteration needed, so just draw a line and return
    if (depth <= 0) {
        SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
        return;
    }

    // Calculate middle-line points
    Point p3 = {p1.x + (p2.x-p1.x) / 3.0, p1.y + (p2.y - p1.y) / 3.0};
    Point p5 = {p1.x + 2.0 * (p2.x - p1.x) / 3.0, p1.y + 2.0 * (p2.y - p1.y) / 3.0};

    // Caluclate the angle
    float angle = -60 * (M_PI / 180.0);
    // Caculate centre point
    Point p4 = {
        p3.x + cos(angle) * (p5.x - p3.x) - sin(angle) * (p5.y - p3.y),
        p3.y + sin(angle) * (p5.x - p3.x) + cos(angle) * (p5.y - p3.y)
    };

    // Draw recursively
    DrawKochCurve(p1, p3, depth - 1);
    DrawKochCurve(p3, p4, depth - 1);
    DrawKochCurve(p4, p5, depth - 1);
    DrawKochCurve(p5, p2, depth - 1);
}

// Draw a snowflake to the screen using SDL line drawing
void DrawKochSnowflake() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderScale(renderer, 1, 1);
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    // Initial vertices for a large centered triangle
    Point p1 = {(400 - x_offset) / zoom + (X_RESOLUTION / 2), (50 - y_offset) / zoom + (Y_RESOLUTION / 2)};
    Point p2 = {(150 - x_offset) / zoom + (X_RESOLUTION / 2), (483 - y_offset) / zoom + (Y_RESOLUTION / 2)};
    Point p3 = {(650 - x_offset) / zoom + (X_RESOLUTION / 2), (483 - y_offset) / zoom + (Y_RESOLUTION / 2)};

    // Draw the 3 sides of the triangle
    DrawKochCurve(p2, p1, RENDER_DEPTH);
    DrawKochCurve(p3, p2, RENDER_DEPTH);
    DrawKochCurve(p1, p3, RENDER_DEPTH);

    SDL_RenderPresent(renderer);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    // Just make absolutely sure that the menu is loaded first
    current_screen = MENU;
    // Create window and renderer
    if (!SDL_CreateWindowAndRenderer("Fractals", X_RESOLUTION, Y_RESOLUTION, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create surface to write pixels to
    surface = SDL_CreateSurface(X_RESOLUTION, Y_RESOLUTION, SDL_PIXELFORMAT_RGBA8888);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // If the application is called to quit, such as the close button, repott a success and close
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    // Ensure only keydown otherwise double input from keyup
    if (event->type == SDL_EVENT_KEY_DOWN) {
        // If in the menu, listen for number keys to change the screen or quit
        switch (current_screen) {
            case MENU: return MenuInput(event); break;
            case MANDELBROT_SET: FractalInput(event); break;
            case SIERPINSKI_TRIANGLE: FractalInput(event); break;
            case KOCH_SNOWFLAKE: FractalInput(event); break;
        };
    }

    return SDL_APP_CONTINUE;
}

// Per frame iteration
SDL_AppResult SDL_AppIterate(void *appstate)
{
    // Switch through to get current screen and draw the relavent screen
    switch (current_screen) {
        case MENU: DrawMenu(); break;
        case MANDELBROT_SET:
            if (USE_THREADS) {
                DrawMandelbrotThreaded();
            }
            else {
                DrawMandelbrot();
            }
            break;
        case SIERPINSKI_TRIANGLE: DrawSierpinksi(); break;
        case KOCH_SNOWFLAKE: DrawKochSnowflake(); break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{

}
