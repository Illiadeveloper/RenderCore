#pragma once
#include "ComponentArray.h"
#include "Types.h"
#include <typeindex>

class ComponentManager {
public:
  template <typename T> //
  void RegisterComponent() {
    std::type_index typeIndex(typeid(T));

    assert(mComponentTypes.find(typeIndex) == mComponentTypes.end() &&
           "Registering component type more than once!!");
    mComponentTypes.insert({typeIndex, mNextComponentType});
    mComponentArrays.insert({typeIndex, std::make_shared<ComponentArray<T>>()});

    ++mNextComponentType;
  }

  template <typename T> //
  ComponentType GetComponentType() {
    std::type_index typeIndex(typeid(T));

    assert(mComponentTypes.find(typeIndex) != mComponentTypes.end() &&
           "Component not registered before use!!");
    return mComponentTypes[typeIndex];
  }

  template <typename T> //
  void AddComponent(Entity entity, T component) {
    GetComponentArray<T>()->InsertData(entity, component);
  }

  template <typename T> //
  bool HasComponent(Entity entity) {
    return GetComponentArray<T>()->HasData(entity);
  }

  bool HasComponent(std::type_index typeindex, Entity entity) {
    auto it = mComponentArrays.find(typeindex);
    if (it == mComponentArrays.end())
      return false;
    return it->second->HasData(entity);
  }

  template <typename T> //
  void RemoveComponent(Entity entity) {
    GetComponentArray<T>()->RemoveData(entity);
  }

  template <typename T> //
  T &GetComponent(Entity entity) {
    return GetComponentArray<T>()->GetData(entity);
  }

  void EntityDestroyed(Entity entity) {
    for (auto const &pair : mComponentArrays) {
      auto const &component = pair.second;

      component->EntityDestroyed(entity);
    }
  }

private:
  std::unordered_map<std::type_index, ComponentType> mComponentTypes{};
  std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>>
      mComponentArrays{};
  ComponentType mNextComponentType{};

  template <typename T> //
  std::shared_ptr<ComponentArray<T>> GetComponentArray() {
    std::type_index typeIndex(typeid(T));
    assert(mComponentTypes.find(typeIndex) != mComponentTypes.end() &&
           "Component not registered before use!!");
    return std::static_pointer_cast<ComponentArray<T>>(
        mComponentArrays[typeIndex]);
  }
};
