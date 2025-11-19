#pragma once

#include "render/Mesh.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using MeshId = std::uint32_t;

class MeshManager {
public:
  MeshManager() = default;

  MeshId LoadMesh(const std::string& path);
  std::shared_ptr<Mesh> GetMesh(MeshId id);
  std::string& GetPath(MeshId id);
private:
  std::unordered_map<std::string, MeshId> mPathToId;
  std::unordered_map<MeshId,std::string> mIdToPath;
  std::unordered_map<MeshId, std::shared_ptr<Mesh>> mIdToMesh;

  void LoadOBJ(const std::string &path, std::vector<Vertex> &outVertices,
                  std::vector<unsigned int>& outIndices);
  MeshId mNextId = 0;
};
