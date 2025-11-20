#pragma once
#include "ecs/Coordinator.h"
#include "ecs/SystemManager.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "managers/UniformBufferManager.h"
class CameraSystem : public System {
public:
  void Update(Coordinator& coordinator, float deltaTime);
  void UploadToUBO(Coordinator& coordinator, UniformBufferManager &uboManager, float aspectRation);
  // glm::mat4 GetView(Coordinator& coordinator);
  // glm::mat4 GetProjection(Coordinator& coordinator, float aspectRatio);
  void ToggleCamera(Coordinator& coordinator);
};
