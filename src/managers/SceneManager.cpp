#include "managers/SceneManager.h"
#include "ecs/Types.h"
#include "managers/ResourceContext.h"
#include "managers/SerializationRegistry.h"
#include <fstream>
#include <iostream>

SceneManager::SceneManager(Coordinator &coord, SerializationRegistry &reg,
                           ResourceContext &resources)
    : mCoordinator(coord), mRegistry(reg), mResourceContext(resources) {}

Json SceneManager::SerializeScene() {
  Json scene;
  Json entities_json = Json::array();

  auto allTypes = mRegistry.AllTypes();

  for (Entity e : mCoordinator.GetAllEntities()) {
    Json entity_json;
    entity_json["id"] = e;
    Json comps = Json::object();

    for (auto type : allTypes) {
      const ComponentSerializer *ser = mRegistry.Find(type);
      if (!ser)
        continue;

      if (mCoordinator.HasComponent(type, e)) {
        Json data = ser->serialize(e, mCoordinator, mResourceContext);
        comps[type.name()] = data;
      }
    }

    entity_json["components"] = comps;
    entities_json.push_back(entity_json);
  }

  scene["entities"] = entities_json;
  return scene;
}

void SceneManager::DeserializeScene(const Json &scene) {
  for (auto &ent : scene["entities"]) {
    Entity e = mCoordinator.CreateEntity(ent["id"]);

    for (auto &kv : ent["components"].items()) {
      std::string type_name = kv.key();
      const Json &data = kv.value();

      // std::cout << kv.key() << std::endl;
      // std::cout << kv.value() << std::endl;

      for (auto type : mRegistry.AllTypes()) {
        if (type.name() == type_name) {
          const ComponentSerializer *ser = mRegistry.Find(type);
          ser->deserialize(e, data, mCoordinator, mResourceContext);
        }
      }
    }
  }
}

void SceneManager::LoadScene(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Failed to open scene file: " << path << std::endl;
    return;
  }

  Json sceneJson;
  try {
    file >> sceneJson;
  } catch (const std::exception &e) {
    std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
    return;
  }

  DeserializeScene(sceneJson);
  std::cout << "Scene loaded: " << path << std::endl;
}

void SceneManager::SaveScene(const std::string& path) {
  Json data = SerializeScene();

  std::ofstream file(path);
  if (!file.is_open()) {
    std::cerr << "Failed to create scene file: " << path << std::endl;
    return;
  }
  file << data;
  std::cout << "Scene saved: " << path << std::endl;
}
