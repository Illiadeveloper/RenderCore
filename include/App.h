#pragma once

#include "glad/glad.h"
//
#include "GLFW/glfw3.h"
#include "ecs/Coordinator.h"
#include "managers/ResourceContext.h"
#include "managers/SceneManager.h"
#include "managers/SerializationRegistry.h"
#include "managers/UniformBufferManager.h"
#include <memory>
#include <stdexcept>

class App {
public:
  App(int width, int height, const char *title);
  void Init();
  ~App();
  void Run();
  SerializationRegistry RegisterSerializeDefaultComponents();

  void UpdateViewport(int w, int h);

private:
  int mWidth;
  int mHeight;

  float mLastFrameTime;

  static void framebuffer_size_callback(GLFWwindow *window, int width,
                                        int heiht);

  UniformBufferManager mUniformManager;
  std::unique_ptr<SceneManager> mSceneManager;
  SerializationRegistry mSerializeRegistry;
  GLFWwindow *mWindow;

  ResourceContext mResources;
  MeshManager mMeshManager;
  ShaderManager mShaderManager;

  Coordinator mCoordinator;
};
