#pragma once
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"
#include "ecs/Types.h"
#include <typeindex>
class Coordinator {
public:
  void Init() {
    mComponentManager = std::make_unique<ComponentManager>();
    mEntityManager = std::make_unique<EntityManager>();
    mSystemManager = std::make_unique<SystemManager>();
  }

  Entity CreateEntity() { return mEntityManager->CreateEntity(); }
  Entity CreateEntity(Entity id) { return mEntityManager->CreateEntity(id); }

  void DestroyEntity(Entity entity) { mEntityManager->DestroyEntity(entity); }

  template <typename T> //
  void RegisterComponent() {
    mComponentManager->RegisterComponent<T>();
  }

  template <typename T> //
  std::shared_ptr<T> GetSystem() {
    return mSystemManager->GetSystem<T>();
  }

  template <typename T> //
  void AddComponent(Entity entity, T component) {
    mComponentManager->AddComponent<T>(entity, component);

    auto signature = mEntityManager->GetSignature(entity);
    signature.set(mComponentManager->GetComponentType<T>(), true);
    mEntityManager->SetSignature(entity, signature);

    mSystemManager->EntitySignatureChanged(entity, signature);
  }

  template <typename T> //
  bool HasComponent(Entity entity) {
    return mComponentManager->HasComponent<T>(entity);
  }

  bool HasComponent(std::type_index typeindex, Entity entity) {
    return mComponentManager->HasComponent(typeindex, entity);
  }

  template <typename T> //
  void RemoveComponent(Entity entity) {
    mComponentManager->RemoveComponent<T>(entity);

    auto signature = mEntityManager->GetSignature(entity);
    signature.set(mComponentManager->GetComponentType<T>(), false);
    mEntityManager->SetSignature(entity, signature);

    mSystemManager->EntitySignatureChanged(entity, signature);
  }

  template <typename T> //
  T &GetComponent(Entity entity) {
    return mComponentManager->GetComponent<T>(entity);
  }

  template <typename T> //
  ComponentType GetComponentType() {
    return mComponentManager->GetComponentType<T>();
  }

  template <typename T> //
  std::shared_ptr<T> RegisterSystem() {
    return mSystemManager->RegisterSystem<T>();
  }

  template <typename T> //
  void SetSystemSignature(Signature signature) {
    mSystemManager->SetSignature<T>(signature);
  }

  const std::vector<Entity> &GetAllEntities() {
    return mEntityManager->GetAllEntities();
  }

  void DestroyAllEntities() { 
    mEntityManager->DestroyAllEntities();
    mComponentManager->ClearAllEntities();
    // for(auto e : GetAllEntities()) {
    //   // mEntityManager->DestroyEntity(e);
    //   mSystemManager->EntityDestroyed(e);
    //   // mComponentManager->EntityDestroyed(e);
    // }
    mSystemManager->Clear();
  }

private:
  std::unique_ptr<ComponentManager> mComponentManager;
  std::unique_ptr<EntityManager> mEntityManager;
  std::unique_ptr<SystemManager> mSystemManager;
};
