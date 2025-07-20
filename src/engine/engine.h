#pragma once

#include "window.h"
#include "renderer/renderer.h"
#include "scene/scene.h"
#include <imgui/dcimgui.h>

typedef struct {
    Renderer renderer;
    Window window;
    ECS ecs;
    ImGuiIO* io;
} Engine;


void engine_initialise(Engine* engine);
void engine_run(Engine* engine);
void engine_cleanup(Engine* engine);

// internal
void process_inputs(Engine* engine);

