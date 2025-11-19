#pragma once

#include "ecs/Coordinator.h"
#include "ecs/Types.h"
#include <functional>
#include <nlohmann/json.hpp>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

using Json = nlohmann::json;

struct ComponentSerializer {
  int schema_version;
  std::function<Json(Entity, Coordinator &)> serialize;
  std::function<void(Entity, const Json &, Coordinator &)> deserialize;
};

class SerializationRegistry {
public:
  template<typename T>
  void RegisterComponent(ComponentSerializer s) {
    mSerializers[typeid(T)] = std::move(s);
  }

  template<typename T>
  const ComponentSerializer *Find() {
    std::type_index typeindex = typeid(T);
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
