#pragma once
#include "Types.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

class EntityManager {
public:
  EntityManager() {
    for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
      mAvailableEntities.push(entity);
    }
  }

  Entity CreateEntity() {
    assert(mLivingEntities.size() < MAX_ENTITIES &&
           "Too many entities in existence!!");

    Entity id = mAvailableEntities.front();
    mAvailableEntities.pop();

    mLivingEntities.push_back(id);
    // mLivingEntityCount++;

    return id;
  }

  Entity CreateEntity(Entity id) {
    assert(id < MAX_ENTITIES && "Too many entities in existence!!");
    auto it = std::find(mLivingEntities.begin(), mLivingEntities.end(), id);
    assert(it == mLivingEntities.end() &&
           "Entity with this ID already exists!");

    std::queue<Entity> newQueue;
    bool found = false;
    while (!mAvailableEntities.empty()) {
      Entity e = mAvailableEntities.front();
      mAvailableEntities.pop();

      if (e == id) {
        found = true;
        continue;
      }

      newQueue.push(e);
    }
    assert(found && "ID not available for creation!");

    mAvailableEntities = std::move(newQueue);

    mLivingEntities.push_back(id);

    mSignatures[id].reset();

    return id;
  }

  void DestroyEntity(Entity entity) {
    assert(entity < MAX_ENTITIES && "Entity out of range!!");
    mSignatures[entity].reset();
    mAvailableEntities.push(entity);

    auto it = std::find(mLivingEntities.begin(), mLivingEntities.end(), entity);
    if (it != mLivingEntities.end()) {
      std::cout << "DELETE: " << entity << std::endl;
      mLivingEntities.erase(it);
    }
    // mLivingEntityCount--;
  }

  void SetSignature(Entity entity, Signature signature) {
    assert(entity < MAX_ENTITIES && "Entity out of range!!");
    mSignatures[entity] = signature;
  }

  Signature GetSignature(Entity entity) {
    assert(entity < MAX_ENTITIES && "Entity out of range!!");
    return mSignatures[entity];
  }

  const std::vector<Entity> &GetAllEntities() const { return mLivingEntities; }

  void DestroyAllEntities() {
    for (Entity e : mLivingEntities) {
      mSignatures[e].reset();
      mAvailableEntities.push(e);
    }
    mLivingEntities.clear();
  }

private:
  std::queue<Entity> mAvailableEntities{};
  std::array<Signature, MAX_ENTITIES> mSignatures{};

  // uint32_t mLivingEntityCount{};
  std::vector<Entity> mLivingEntities;
};
