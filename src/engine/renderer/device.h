#pragma once

#include "../utils.h"
#include "renderer.h"

void device_initialise(Renderer* renderer);


void pick_gpu(Renderer* renderer);
int rate_device(VkPhysicalDevice gpu, VkSurfaceKHR* surface);
bool check_extention_device_support(VkPhysicalDevice gpu);
void create_device(Renderer* renderer);

