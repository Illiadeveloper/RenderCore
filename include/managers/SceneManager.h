#pragma once

#include "ecs/Coordinator.h"
#include "managers/ResourceContext.h"
#include "managers/SerializationRegistry.h"
class SceneManager {
public:
  SceneManager(Coordinator &coord, SerializationRegistry &reg,
               ResourceContext &resources);

  Json SerializeScene();
  void DeserializeScene(const Json &scene);

  void LoadScene(const std::string &path);
  void SaveScene(const std::string &path);

private:
  Coordinator &mCoordinator;
  SerializationRegistry &mRegistry;
  ResourceContext &mResourceContext;
};
