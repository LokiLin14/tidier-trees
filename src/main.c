#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "renderers/SDL2/clay_renderer_SDL2.c"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

Clay_RenderCommandArray CreateLayout() {
    Clay_BeginLayout();

    // Container to center the rectangle
    CLAY(CLAY_ID("OuterContainer"), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
        }
    }) {
        // Orange rectangle
        CLAY(CLAY_ID("Rectangle"), {
            .layout = { .sizing = { CLAY_SIZING_FIXED(200), CLAY_SIZING_FIXED(200) } },
            .backgroundColor = { 255, 165, 0, 255 }
        }) {}
    }

    return Clay_EndLayout();
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error: could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode); // 0 = primary display

    // Option 1: Percentage of screen (80%)
    int windowWidth = (int)(displayMode.w * 0.6);
    int windowHeight = (int)(displayMode.h * 0.6);

    SDL_Window *window = SDL_CreateWindow("Clay SDL2 Test",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          windowWidth, windowHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Initialize(clayMemory, (Clay_Dimensions) { (float)windowWidth, (float)windowHeight }, (Clay_ErrorHandler) { HandleClayErrors });

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        Clay_SetLayoutDimensions((Clay_Dimensions) { (float)windowWidth, (float)windowHeight });

        Clay_RenderCommandArray renderCommands = CreateLayout();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        Clay_SDL2_Render(renderer, renderCommands, NULL);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}