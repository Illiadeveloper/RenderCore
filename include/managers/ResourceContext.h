#pragma once

#include "managers/MeshManager.h"
#include "managers/ShaderManager.h"

struct ResourceContext {
  MeshManager* meshes;
  ShaderManager* shaders;
};
