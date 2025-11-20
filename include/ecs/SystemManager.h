#pragma once
#include "Types.h"
#include <memory>
#include <typeindex>

class System {
public:
  std::set<Entity> mEntities;
  virtual ~System() = default;
};

class SystemManager {
public:
  template <typename T> //
  std::shared_ptr<T> RegisterSystem() {
    std::type_index typeIndex(typeid(T));

    assert(mSystems.find(typeIndex) == mSystems.end() &&
           "Registering system more than once!!");
    auto system = std::make_shared<T>();
    mSystems.insert({typeIndex, system});
    return system;
  }

  template <typename T> void SetSignature(Signature signature) {
    std::type_index typeIndex(typeid(T));

    assert(mSystems.find(typeIndex) != mSystems.end() &&
           "System used before registered!!");
    mSignatures.insert({typeIndex, signature});
  }

  template <typename T> std::shared_ptr<T> GetSystem() {
    std::type_index typeIndex(typeid(T));

    assert(mSystems.find(typeIndex) != mSystems.end() &&
           "System used before registered!!");
    return std::dynamic_pointer_cast<T>(mSystems[typeIndex]);
  }

  void EntityDestroyed(Entity entity) {
    for (auto const &pair : mSystems) {
      auto const &system = pair.second;
      system->mEntities.erase(entity);
    }
  }

  void EntitySignatureChanged(Entity entity, Signature entitySignature) {
    for (auto const &pair : mSystems) {
      auto const &type = pair.first;
      auto const &system = pair.second;
      auto const &systemSignature = mSignatures[type];

      if ((entitySignature & systemSignature) == systemSignature) {
        system->mEntities.insert(entity);
      } else {
        system->mEntities.erase(entity);
      }
    }
  }

  void Clear() {
    for (auto &[_, system] : mSystems) {
      system->mEntities.clear();
    }
  }

private:
  std::unordered_map<std::type_index, Signature> mSignatures{};
  std::unordered_map<std::type_index, std::shared_ptr<System>> mSystems{};
};
