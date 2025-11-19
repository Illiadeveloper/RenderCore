#pragma once
#include "Types.h"

class IComponentArray {
public:
  virtual ~IComponentArray() = default;
  virtual void EntityDestroyed(Entity entity) = 0;
  virtual bool HasData(Entity entity) = 0;
};

template <typename T> class ComponentArray : public IComponentArray {
public:
  void InsertData(Entity entity, T component) {
    assert(mEntityToIndexMap.find(entity) == mEntityToIndexMap.end() &&
           "Component added to same entity more than once!!");

    size_t newIndex = mSize;
    mEntityToIndexMap[entity] = newIndex;
    mIndexToEntityMap[newIndex] = entity;
    mComponentArray[newIndex] = component;
    mSize++;
  }
  
  bool HasData(Entity entity) override {
    return mEntityToIndexMap.find(entity) != mEntityToIndexMap.end();
  }

  void RemoveData(Entity entity) {
    assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() &&
           "Removing non-existent component!!");

    size_t indexOfRemovedEntity = mEntityToIndexMap[entity];
    size_t indexOfLastComponent = mSize - 1;

    mComponentArray[indexOfRemovedEntity] =
        mComponentArray[indexOfLastComponent];

    Entity entityOfLastElement = mIndexToEntityMap[indexOfLastComponent];
    mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
    mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

    mEntityToIndexMap.erase(entity);
    mIndexToEntityMap.erase(indexOfLastComponent);

    mSize--;
  }

  T &GetData(Entity entity) {
    assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() &&
           "Retrieving non-existent component.");
    return mComponentArray[mEntityToIndexMap[entity]];
  }
  void EntityDestroyed(Entity entity) {
    if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end()) {
      RemoveData(entity);
    }
  }

private:
  std::array<T, MAX_ENTITIES> mComponentArray;
  std::unordered_map<Entity, size_t> mEntityToIndexMap;
  std::unordered_map<size_t, Entity> mIndexToEntityMap;
  size_t mSize;
};

