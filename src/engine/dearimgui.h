#pragma once
#include <dcimgui.h>

#include "renderer/renderer.h"

void imgui_initialise(Renderer* renderer, GLFWwindow* window, ImGuiIO** io_out);
void imgui_cleanup(Renderer* renderer);

void imgui_draw(Renderer* renderer, VkCommandBuffer cmd_buf, VkImageView target_image_view);
void imgui_frame(ImGuiIO* io, Renderer* renderer);

