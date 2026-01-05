#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "renderers/SDL2/clay_renderer_SDL2.c"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

#include "trees.h"

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

Clay_Color backgroundColor = {220,220,225,255};
Clay_Color sidebarOrange = {221, 123, 24, 255 };
Clay_Color borderGray = { 60, 60, 60, 255 };
Clay_Color nodeColor = { 100, 150, 200, 255 };
Clay_Color edgeColor = { 80, 80, 80, 255 };

typedef struct {
    float x_offset;
    float y_offset;
    struct tree_t* tree;
} app_data_t;

// Configuration for tree rendering
#define NODE_RADIUS 20.0f
#define VERTICAL_SPACING 60.0f
#define HORIZONTAL_SCALE 30.0f

// Recursive function to render tree nodes using Clay floating elements
void RenderTreeNode(struct tree_t *node, float x_offset, float y_offset) {
    if (node == NULL) return;

    // Calculate position (scaled to screen coordinates)
    float x = (float)node->x_pos * HORIZONTAL_SCALE + x_offset;
    float y = (float)node->y_pos * VERTICAL_SPACING + y_offset;

    // Draw the node as a circle (rounded rectangle)
    CLAY(CLAY_IDI("Node", node->id), {
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(NODE_RADIUS * 2), CLAY_SIZING_FIXED(NODE_RADIUS * 2) }
        },
        .backgroundColor = nodeColor,
        .cornerRadius = CLAY_CORNER_RADIUS(NODE_RADIUS), // Makes it circular
        .floating = {
            .zIndex = 0,
            .attachTo = CLAY_ATTACH_TO_PARENT,
            .offset = { x - NODE_RADIUS, y - NODE_RADIUS }, // Center the circle on the position
            .clipTo = CLAY_CLIP_TO_ATTACHED_PARENT
        },
    }) {}

    // Recursively render children
    RenderTreeNode(node->left_child, x_offset, y_offset);
    RenderTreeNode(node->right_child, x_offset, y_offset);
}

Clay_RenderCommandArray CreateLayout(app_data_t* app_data) {
    Clay_BeginLayout();

    // Container to center the rectangle
    CLAY(CLAY_ID("OuterContainer"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP },
            .padding =  CLAY_PADDING_ALL(16),
            .childGap = 16
        },
        .backgroundColor = backgroundColor
    }) {
        CLAY(CLAY_ID("Sidebar"), {
            .layout = { .sizing = { CLAY_SIZING_FIXED(200), CLAY_SIZING_GROW() } },
            .backgroundColor = sidebarOrange,
            .cornerRadius = CLAY_CORNER_RADIUS(10)
        }) {
        }
        CLAY(CLAY_ID("TreeDisplay"), {
            .layout = { .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW() } },
            .backgroundColor = backgroundColor,
            .cornerRadius = CLAY_CORNER_RADIUS(10),
            .border = { .color = borderGray, .width = CLAY_BORDER_ALL(4) },
            .clip = { .horizontal = true, .vertical = true }
        }) {
            if (app_data->tree != NULL) {
                Clay_ElementData treeDisplayData = Clay_GetElementData(Clay_GetElementId(CLAY_STRING("TreeDisplay")));

                float centerX = treeDisplayData.boundingBox.width / 2.0f - app_data->x_offset;
                float centerY = 100.0f - app_data->y_offset;

                RenderTreeNode(app_data->tree, centerX, centerY);
            }
        }
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

    app_data_t app_data = { 0 };

    bool running = true;

    // Control the framerate to not burn up the cpu
    const int target_fps = 60;
    const int frame_delay = 1000 / target_fps;

    while (running) {
        int frame_start = (int)SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        tree_free(app_data.tree);
                        const int min_height = 3;
                        const int max_height = 7;
                        const float chance_to_continue = 0.4f;
                        app_data.tree = tree_random(min_height, max_height, chance_to_continue);
                        tree_compute_layout(app_data.tree);

                        char* tree_str = tree_to_string(app_data.tree);
                        if (tree_str) {
                            printf("%s\n", tree_str);
                        }
                        free(tree_str);
                    }
                default:
                    break;
            }
        }

        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_W]) {
            app_data.y_offset += 0.1f;
        }
        if (keystate[SDL_SCANCODE_A]) {
            app_data.x_offset -= 0.1f;
        }
        if (keystate[SDL_SCANCODE_S]) {
            app_data.y_offset -= 0.1f;
        }
        if (keystate[SDL_SCANCODE_D]) {
            app_data.x_offset += 0.1f;
        }

        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        Clay_SetLayoutDimensions((Clay_Dimensions) { (float)windowWidth, (float)windowHeight });

        Clay_RenderCommandArray renderCommands = CreateLayout(&app_data);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        Clay_SDL2_Render(renderer, renderCommands, NULL);

        SDL_RenderPresent(renderer);

        int frame_time = (int)SDL_GetTicks() - frame_start;
        if (frame_delay > frame_time) {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    tree_free(app_data.tree);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}