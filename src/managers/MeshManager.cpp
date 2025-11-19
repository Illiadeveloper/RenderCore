#include "managers/MeshManager.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "render/Mesh.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

MeshId MeshManager::LoadMesh(const std::string &path) {
  if (mPathToId.find(path) != mPathToId.end()) {
    return mPathToId[path];
  }
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  LoadOBJ(path, vertices, indices);

  auto mesh = std::make_shared<Mesh>(vertices, indices);

  MeshId id = mNextId++;
  mPathToId[path] = id;
  mIdToPath[id] = path;
  mIdToMesh[id] = mesh;

  return id;
}

std::shared_ptr<Mesh> MeshManager::GetMesh(MeshId id) {
  auto it = mIdToMesh.find(id);
  if (it == mIdToMesh.end())
    return nullptr;
  return it->second;
}

std::string &MeshManager::GetPath(MeshId id) { return mIdToPath[id]; }

void MeshManager::LoadOBJ(const std::string &path,
                          std::vector<Vertex> &outVertices,
                          std::vector<unsigned int> &outIndices) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Failed to open OBJ file: " << path << std::endl;
    return;
  }

  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> texCoords;
  std::map<std::string, unsigned int> uniqueVertexMap;

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream ss(line);
    std::string type;
    ss >> type;

    if (type == "v") {
      glm::vec3 pos;
      ss >> pos.x >> pos.y >> pos.z;
      positions.push_back(pos);
    } else if (type == "vt") {
      glm::vec2 uv;
      ss >> uv.x >> uv.y;
      texCoords.push_back(uv);
    } else if (type == "vn") {
      glm::vec3 normal;
      ss >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    } else if (type == "f") {
      for (int i = 0; i < 3; i++) {
        std::string vertexData;
        ss >> vertexData;

        if (uniqueVertexMap.count(vertexData)) {
          outIndices.push_back(uniqueVertexMap[vertexData]);
          continue;
        }

        std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
        std::istringstream vs(vertexData);
        int vi = 0, ti = 0, ni = 0;
        vs >> vi >> ti >> ni;

        Vertex vertex{};
        vertex.position = positions[vi - 1];
        if (ti > 0)
          vertex.texCoord = texCoords[ti - 1];
        if (ni > 0)
          vertex.normal = normals[ni - 1];

        unsigned int newIndex = static_cast<unsigned int>(outVertices.size());
        uniqueVertexMap[vertexData] = newIndex;
        outVertices.push_back(vertex);
        outIndices.push_back(newIndex);
      }
    }
  }
}
