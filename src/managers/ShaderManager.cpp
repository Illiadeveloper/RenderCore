#include "managers/ShaderManager.h"
#include "App.h"
#include "glm/detail/qualifier.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "render/Mesh.h"
#include <fstream>
#include <iostream>
#include <utility>

ShaderId ShaderManager::LoadShader(const std::string &frag,
                                   const std::string &vert) {
  std::string fragStr = GetFileContext(frag);
  std::string vertStr = GetFileContext(vert);

  const char *fragContext = fragStr.c_str();
  const char *vertContext = vertStr.c_str();

  GLuint fragShader, vertShader;
  fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragShader, 1, &fragContext, NULL);
  glCompileShader(fragShader);

  GLint success;
  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[1024];
    glGetShaderInfoLog(fragShader, sizeof(infoLog), nullptr, infoLog);
    std::cerr << "[ShaderManager] Fragment Shader compilation error (" << frag
              << "):\n"
              << infoLog << std::endl;
    glDeleteShader(fragShader);
    return 0;
  }

  vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertShader, 1, &vertContext, NULL);
  glCompileShader(vertShader);

  glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[1024];
    glGetShaderInfoLog(vertShader, sizeof(infoLog), nullptr, infoLog);
    std::cerr << "[ShaderManager] Vertex Shader compilation error (" << vert
              << "):\n"
              << infoLog << std::endl;
    glDeleteShader(fragShader);
    glDeleteShader(vertShader);
    return 0;
  }

  ShaderId program = glCreateProgram();
  glAttachShader(program, fragShader);
  glAttachShader(program, vertShader);
  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[1024];
    glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
    std::cerr << "[ShaderManager] Shader Program linking error:\n"
              << infoLog << std::endl;
    glDeleteProgram(program);
    glDeleteShader(fragShader);
    glDeleteShader(vertShader);
    return 0;
  }

  GLuint blockIndex = glGetUniformBlockIndex(program, "CameraUBO");
  if (blockIndex != GL_INVALID_INDEX) {
    glUniformBlockBinding(program, blockIndex, 0);
  }

  glDeleteShader(fragShader);
  glDeleteShader(vertShader);

  mIds.push_back(program);
  mIdToPath[program] = std::make_pair(vert, frag);
  return program;
}

std::pair<std::string, std::string> ShaderManager::GetPath(ShaderId id) {
  return mIdToPath[id];
}

void ShaderManager::BindShader(ShaderId id) { glUseProgram(id); }
void ShaderManager::UnbindShader() { glUseProgram(0); }

ShaderManager::~ShaderManager() {
  for (auto id : mIds) {
    if (id != 0) {
      glDeleteProgram(id);
    }
    mUniformLocationCache.erase(id);
  }
  mIds.clear();
  mUniformLocationCache.clear();
}

GLint ShaderManager::GetUniformLocation(ShaderId id, const std::string &name) {
  auto &progCache = mUniformLocationCache[id];
  auto it = progCache.find(name);
  if (it != progCache.end())
    return it->second;

  GLint loc = glGetUniformLocation(id, name.c_str());
  progCache.emplace(name, loc);
  return loc;
}

void ShaderManager::InvalidateUniformCache(ShaderId id) {
  mUniformLocationCache.erase(id);
}

void ShaderManager::SetMat4(ShaderId id, const std::string &name,
                            const glm::mat4 &matrix) {
  GLint loc = GetUniformLocation(id, name);
  if (loc == -1) {
    return;
  }
  glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
}

void ShaderManager::SetVec3(ShaderId id, const std::string &name,
                            const glm::vec3 &vec) {
  GLint loc = GetUniformLocation(id, name);
  if (loc == -1)
    return;

  glUniform3fv(loc, 1, glm::value_ptr(vec));
}

std::string ShaderManager::GetFileContext(const std::string &path) {
  std::ifstream file(path);
  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}
