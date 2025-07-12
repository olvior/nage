#pragma once

typedef struct GLFWwindow GLFWwindow;

typedef struct {
    GLFWwindow* window;
    const char* title;
    int width;
    int height;
} Window;

void window_initialise(Window* window);
void window_cleanup(Window* window);
