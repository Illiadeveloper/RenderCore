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
        } 
        else if (type == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            texCoords.push_back(uv);
        } 
        else if (type == "vn") {
            glm::vec3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } 
        else if (type == "f") {
            std::vector<std::string> faceVertices;
            std::string vertexData;

            while (ss >> vertexData) {
                faceVertices.push_back(vertexData);
            }

            if (faceVertices.size() < 3) continue;

            auto processVertex = [&](const std::string& vData) -> unsigned int {
                if (uniqueVertexMap.count(vData))
                    return uniqueVertexMap[vData];

                std::string temp = vData;
                std::replace(temp.begin(), temp.end(), '/', ' ');

                std::istringstream vs(temp);
                int vi = 0, ti = 0, ni = 0;
                vs >> vi >> ti >> ni;

                Vertex vertex{};
                vertex.position = positions[vi - 1];

                if (ti > 0)
                    vertex.texCoord = texCoords[ti - 1];

                if (ni > 0)
                    vertex.normal = normals[ni - 1];

                unsigned int idx = outVertices.size();
                uniqueVertexMap[vData] = idx;
                outVertices.push_back(vertex);
                return idx;
            };

            if (faceVertices.size() == 3) {
                for (int i = 0; i < 3; i++)
                    outIndices.push_back(processVertex(faceVertices[i]));
            }
            else if (faceVertices.size() == 4) {
                unsigned int v0 = processVertex(faceVertices[0]);
                unsigned int v1 = processVertex(faceVertices[1]);
                unsigned int v2 = processVertex(faceVertices[2]);
                unsigned int v3 = processVertex(faceVertices[3]);

                outIndices.push_back(v0);
                outIndices.push_back(v1);
                outIndices.push_back(v2);

                outIndices.push_back(v0);
                outIndices.push_back(v2);
                outIndices.push_back(v3);
            }
        }
    }
}

void MeshManager::Clear() {
  mPathToId.clear();
  mIdToPath.clear();
  mIdToMesh.clear();
  mNextId = 0;
}
