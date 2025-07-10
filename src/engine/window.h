#pragma once

typedef struct GLFWwindow GLFWwindow;

typedef struct {
    GLFWwindow* window;
    int width;
    int height;
    const char* title;
} Window;

void window_initialise(Window* window);
void window_cleanup(Window* window);
