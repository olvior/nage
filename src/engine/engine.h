#include "utils.h"

typedef struct GLFWwindow {} GLFWwindow;

typedef struct {
    GLFWwindow* window;
    int width;
    int height;
    const char* title;
} Window;

typedef struct {
    Window window;
} Engine;




void engine_initialise(Engine* engine);
void engine_run(Engine* engine);


void window_initialise(Window* window);
