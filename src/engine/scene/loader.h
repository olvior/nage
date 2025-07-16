#pragma once

#include "../renderer/renderer.h"


Mesh load_obj_mesh(Renderer* renderer, char* file_path);
Mesh* load_glft_meshes(Renderer* renderer, char* file_path);

