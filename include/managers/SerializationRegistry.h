#pragma once

#include "ecs/Coordinator.h"
#include "ecs/Types.h"
#include "managers/ResourceContext.h"
#include <functional>
#include <nlohmann/json.hpp>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

using Json = nlohmann::json;

struct ComponentSerializer {
  int schema_version;
  std::function<Json(Entity, Coordinator &, ResourceContext&)> serialize;
  std::function<void(Entity, const Json &, Coordinator &, ResourceContext&)> deserialize;
};

class SerializationRegistry {
public:
  template<typename T>
  void RegisterComponent(ComponentSerializer s) {
    mSerializers[typeid(T)] = std::move(s);
  }

  const ComponentSerializer *Find(std::type_index typeindex) {
    auto it = mSerializers.find(typeindex);
    return it == mSerializers.end() ? nullptr : &it->second;
  }

  std::vector<std::type_index> AllTypes() const {
    std::vector<std::type_index> r;
    for (auto &p : mSerializers)
      r.push_back(p.first);
    return r;
  }

private:
  std::map<std::type_index, ComponentSerializer> mSerializers;
};
