#pragma once
#include "components/MaterialComponent.h"
#include "components/MeshComponent.h"
#include "components/ShaderComponent.h"
#include "components/TransformComponent.h"
#include "ecs/Coordinator.h"
#include "ecs/SystemManager.h"
#include "glm/mat4x4.hpp"
#include "managers/MeshManager.h"
#include "managers/ResourceContext.h"
#include "managers/ShaderManager.h"
#include "managers/UniformBufferManager.h"

class RenderSystem : public System {
public:
  glm::mat4 GetTransformMatrix(TransformComponent transform);
  void SetMaterial(UniformBufferManager &uboManager, MaterialComponent material);
  void Update(Coordinator &coordinator, ResourceContext& resoruces,
              UniformBufferManager& uboManager);
};
