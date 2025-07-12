#pragma once

#include "window.h"
#include "renderer/renderer.h"

typedef struct {
    Renderer renderer;
    Window window;
} Engine;


void engine_initialise(Engine* engine);
void engine_run(Engine* engine);
void engine_cleanup(Engine* engine);

