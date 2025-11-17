#include "systems/RenderSystem.h"
#include "components/MaterialComponent.h"
#include "components/TransformComponent.h"
#include "ecs/Coordinator.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "managers/ResourceContext.h"
#include "managers/UniformBufferManager.h"
#include "render/uniforms/MaterialUBO.h"
#include <iostream>
#include <memory>

glm::mat4 RenderSystem::GetTransformMatrix(TransformComponent transform) {
  glm::mat4 model(1.0f);
  model = glm::translate(model, transform.mPosition);
  model = glm::rotate(model, transform.mRotation.x, glm::vec3(1, 0, 0));
  model = glm::rotate(model, transform.mRotation.y, glm::vec3(0, 1, 0));
  model = glm::rotate(model, transform.mRotation.z, glm::vec3(0, 0, 1));
  model = glm::scale(model, transform.mScale);
  return model;
}

void RenderSystem::SetMaterial(UniformBufferManager &uboManager,
                               MaterialComponent material) {
  MaterialUBO data{};
  data.ambient = material.ambient;
  data.diffuse = material.diffuse;
  data.specular = material.specular;
  data.shininess = material.shininess;
  uboManager.UpdateUBO("Material", data);
}

void RenderSystem::Update(Coordinator &coordinator, ResourceContext &resources,
                          UniformBufferManager &uboManager) {

  for (auto const &entity : mEntities) {
    auto &meshComponent = coordinator.GetComponent<MeshComponent>(entity);
    auto &shaderComponent = coordinator.GetComponent<ShaderComponent>(entity);
    auto &transformComponent =
        coordinator.GetComponent<TransformComponent>(entity);

    resources.shaders->BindShader(shaderComponent.mId);
    if (coordinator.HasComponent<MaterialComponent>(entity)) {
      auto &material = coordinator.GetComponent<MaterialComponent>(entity);
      SetMaterial(uboManager, material);
    }
    // ====== VERTEX SHADER ======
    resources.shaders->SetMat4(shaderComponent.mId, "uModel",
                               GetTransformMatrix(transformComponent));

    // ====== FRAG SHADER ==============
    resources.shaders->SetVec3(shaderComponent.mId, "uObjectColor",
                               shaderComponent.mObjectColor);

    auto mesh = resources.meshes->GetMesh(meshComponent.mId);

    mesh->Draw();
    resources.shaders->UnbindShader();
  }
}
