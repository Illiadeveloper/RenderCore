#include "App.h"
#include "GLFW/glfw3.h"
#include "components/CameraComponent.h"
#include "components/DirectionalLightComponent.h"
#include "components/MaterialComponent.h"
#include "components/MeshComponent.h"
#include "components/PointLightComponent.h"
#include "components/ShaderComponent.h"
#include "components/SpotLightComponent.h"
#include "components/TransformComponent.h"
#include "ecs/Coordinator.h"
#include "ecs/Types.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include "managers/MeshManager.h"
#include "managers/ResourceContext.h"
#include "managers/SceneManager.h"
#include "managers/SerializationRegistry.h"
#include "managers/ShaderManager.h"
#include "render/uniforms/CameraUBO.h"
#include "render/uniforms/DirectionalLightUBO.h"
#include "render/uniforms/MaterialUBO.h"
#include "render/uniforms/PointLightUBO.h"
#include "render/uniforms/SpotLightUBO.h"
#include "systems/CameraSystem.h"
#include "systems/DirectionalLightSystem.h"
#include "systems/PointLightSystem.h"
#include "systems/RenderSystem.h"
#include "systems/SpotLightSystem.h"
#include <GL/gl.h>
#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <sys/ucontext.h>
#include <utility>

App::App(int width, int height, const char *title)
    : mWidth(width), mHeight(height), mLastFrameTime(0) {
  if (!glfwInit()) {
    throw std::runtime_error("Couldn't init glfw");
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  mWindow = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!mWindow) {
    throw std::runtime_error("Couldn't create a window");
  }

  glfwMakeContextCurrent(mWindow);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    throw std::runtime_error("Couldn't load GLAD");
  }
  UpdateViewport(mWidth, mHeight);

  glfwSetWindowUserPointer(mWindow, this);
  glfwSetFramebufferSizeCallback(mWindow, framebuffer_size_callback);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void App::UpdateViewport(int w, int h) {
  this->mWidth = w;
  this->mHeight = h;
  glViewport(0, 0, w, h);
}

void App::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  App *app = static_cast<App *>(glfwGetWindowUserPointer(window));
  if (app)
    app->UpdateViewport(width, height);
}

void App::Init() {
  mUniformManager.CreateUBO<CameraUBO>("Camera", 0);
  mUniformManager.CreateUBO<DirectionalLightUBO>("DirectionalLight", 1);
  mUniformManager.CreateUBO<PointLightUBO>("PointLight", 2);
  mUniformManager.CreateUBO<SpotLightUBO>("SpotLight", 3);
  mUniformManager.CreateUBO<MaterialUBO>("Material", 4);

  mCoordinator.Init();
  mCoordinator.RegisterComponent<MeshComponent>();
  mCoordinator.RegisterComponent<ShaderComponent>();
  mCoordinator.RegisterComponent<TransformComponent>();
  mCoordinator.RegisterComponent<CameraComponent>();
  mCoordinator.RegisterComponent<MaterialComponent>();

  mCoordinator.RegisterComponent<DirectionalLightComponent>();
  mCoordinator.RegisterComponent<PointLightComponent>();
  mCoordinator.RegisterComponent<SpotLightComponent>();

  mCoordinator.RegisterSystem<RenderSystem>();
  Signature RenderSignature;
  RenderSignature.set(mCoordinator.GetComponentType<MeshComponent>());
  RenderSignature.set(mCoordinator.GetComponentType<ShaderComponent>());
  RenderSignature.set(mCoordinator.GetComponentType<TransformComponent>());
  mCoordinator.SetSystemSignature<RenderSystem>(RenderSignature);

  mCoordinator.RegisterSystem<CameraSystem>();
  Signature CameraSignature;
  CameraSignature.set(mCoordinator.GetComponentType<CameraComponent>());
  CameraSignature.set(mCoordinator.GetComponentType<TransformComponent>());
  mCoordinator.SetSystemSignature<CameraSystem>(CameraSignature);

  mCoordinator.RegisterSystem<DirectionalLightSystem>();
  Signature DirectionalLightSignature;
  DirectionalLightSignature.set(
      mCoordinator.GetComponentType<DirectionalLightComponent>());
  mCoordinator.SetSystemSignature<DirectionalLightSystem>(
      DirectionalLightSignature);

  mCoordinator.RegisterSystem<PointLightSystem>();
  Signature PointLightSignature;
  PointLightSignature.set(mCoordinator.GetComponentType<PointLightComponent>());
  PointLightSignature.set(mCoordinator.GetComponentType<TransformComponent>());
  mCoordinator.SetSystemSignature<PointLightSystem>(PointLightSignature);

  mCoordinator.RegisterSystem<SpotLightSystem>();
  Signature SpotLightSignature;
  SpotLightSignature.set(mCoordinator.GetComponentType<SpotLightComponent>());
  SpotLightSignature.set(mCoordinator.GetComponentType<TransformComponent>());
  mCoordinator.SetSystemSignature<SpotLightSystem>(SpotLightSignature);

  // ======= RESOURCES =======

  mResources.meshes = &mMeshManager;
  mResources.shaders = &mShaderManager;

  mSerializeRegistry = RegisterSerializeDefaultComponents();
  mSceneManager = std::make_unique<SceneManager>(
      SceneManager{mCoordinator, mSerializeRegistry, mResources});
}

SerializationRegistry App::RegisterSerializeDefaultComponents() {
  SerializationRegistry registry;

  // --- TransformComponent ---
  registry.RegisterComponent<TransformComponent>(
      {.schema_version = 1,
       .serialize = [](Entity e, Coordinator &c, ResourceContext &) -> Json {
         const auto &t = c.GetComponent<TransformComponent>(e);
         return {{"pos_x", t.mPosition.x}, {"pos_y", t.mPosition.y},
                 {"pos_z", t.mPosition.z}, {"rot_x", t.mRotation.x},
                 {"rot_y", t.mRotation.y}, {"rot_z", t.mRotation.z},
                 {"scale_x", t.mScale.x},  {"scale_y", t.mScale.y},
                 {"scale_z", t.mScale.z}};
       },
       .deserialize =
           [](Entity e, const Json &j, Coordinator &c, ResourceContext &) {
             if (!c.HasComponent<TransformComponent>(e))
               c.AddComponent(e, TransformComponent{});
             auto &t = c.GetComponent<TransformComponent>(e);
             t.mPosition.x = j["pos_x"];
             t.mPosition.y = j["pos_y"];
             t.mPosition.z = j["pos_z"];
             t.mRotation.x = j["rot_x"];
             t.mRotation.y = j["rot_y"];
             t.mRotation.z = j["rot_z"];
             t.mScale.x = j["scale_x"];
             t.mScale.y = j["scale_y"];
             t.mScale.z = j["scale_z"];
           }});

  // --- CameraComponent ---
  registry.RegisterComponent<CameraComponent>(
      {.schema_version = 1,
       .serialize = [](Entity e, Coordinator &c, ResourceContext &) -> Json {
         const auto &cam = c.GetComponent<CameraComponent>(e);
         return {{"active", cam.mActive},
                 {"target_x", cam.mTarget.x},
                 {"target_y", cam.mTarget.y},
                 {"target_z", cam.mTarget.z},
                 {"distance", cam.mDistance},
                 {"yaw", cam.mYaw},
                 {"pitch", cam.mPitch},
                 {"minPitch", cam.mMinPitch},
                 {"maxPitch", cam.mMaxPitch},
                 {"autoRotate", cam.mAutoRotate},
                 {"autoRotateSpeed", cam.mAutoRotateSpeed},
                 {"fov", cam.mFov},
                 {"nearPlane", cam.mNearPlane},
                 {"farPlane", cam.mFarPlane}};
       },
       .deserialize =
           [](Entity e, const Json &j, Coordinator &c, ResourceContext &) {
             if (!c.HasComponent<CameraComponent>(e)) {
               c.AddComponent(e, CameraComponent{});
               std::cout << "ADDED CAMERA COMPONENT" << std::endl;
             }
             auto &cam = c.GetComponent<CameraComponent>(e);
             cam.mActive = j["active"];
             cam.mTarget.x = j["target_x"];
             cam.mTarget.y = j["target_y"];
             cam.mTarget.z = j["target_z"];
             cam.mDistance = j["distance"];
             cam.mYaw = j["yaw"];
             cam.mPitch = j["pitch"];
             cam.mMinPitch = j["minPitch"];
             cam.mMaxPitch = j["maxPitch"];
             cam.mAutoRotate = j["autoRotate"];
             cam.mAutoRotateSpeed = j["autoRotateSpeed"];
             cam.mFov = j["fov"];
             cam.mNearPlane = j["nearPlane"];
             cam.mFarPlane = j["farPlane"];
           }});

  // --- DirectionalLightComponent ---
  registry.RegisterComponent<DirectionalLightComponent>(
      {.schema_version = 1,
       .serialize = [](Entity e, Coordinator &c, ResourceContext &) -> Json {
         const auto &l = c.GetComponent<DirectionalLightComponent>(e);
         return {{"dir_x", l.direction.x},    {"dir_y", l.direction.y},
                 {"dir_z", l.direction.z},    {"color_r", l.lightColor.r},
                 {"color_g", l.lightColor.g}, {"color_b", l.lightColor.b},
                 {"intensity", l.intensity}};
       },
       .deserialize =
           [](Entity e, const Json &j, Coordinator &c, ResourceContext &) {
             if (!c.HasComponent<DirectionalLightComponent>(e))
               c.AddComponent(e, DirectionalLightComponent{});
             auto &l = c.GetComponent<DirectionalLightComponent>(e);
             l.direction.x = j["dir_x"];
             l.direction.y = j["dir_y"];
             l.direction.z = j["dir_z"];
             l.lightColor.r = j["color_r"];
             l.lightColor.g = j["color_g"];
             l.lightColor.b = j["color_b"];
             l.intensity = j["intensity"];
           }});

  // --- MaterialComponent ---
  registry.RegisterComponent<MaterialComponent>(
      {.schema_version = 1,
       .serialize = [](Entity e, Coordinator &c, ResourceContext &) -> Json {
         const auto &m = c.GetComponent<MaterialComponent>(e);
         return {{"ambient_r", m.ambient.r},   {"ambient_g", m.ambient.g},
                 {"ambient_b", m.ambient.b},   {"diffuse_r", m.diffuse.r},
                 {"diffuse_g", m.diffuse.g},   {"diffuse_b", m.diffuse.b},
                 {"specular_r", m.specular.r}, {"specular_g", m.specular.g},
                 {"specular_b", m.specular.b}, {"shininess", m.shininess}};
       },
       .deserialize =
           [](Entity e, const Json &j, Coordinator &c, ResourceContext &) {
             if (!c.HasComponent<MaterialComponent>(e))
               c.AddComponent(e, MaterialComponent{});
             auto &m = c.GetComponent<MaterialComponent>(e);
             m.ambient.r = j["ambient_r"];
             m.ambient.g = j["ambient_g"];
             m.ambient.b = j["ambient_b"];
             m.diffuse.r = j["diffuse_r"];
             m.diffuse.g = j["diffuse_g"];
             m.diffuse.b = j["diffuse_b"];
             m.specular.r = j["specular_r"];
             m.specular.g = j["specular_g"];
             m.specular.b = j["specular_b"];
             m.shininess = j["shininess"];
           }});

  // --- MeshComponent ---
  registry.RegisterComponent<MeshComponent>(
      {.schema_version = 1,
       .serialize = [](Entity e, Coordinator &c, ResourceContext &rm) -> Json {
         const auto &m = c.GetComponent<MeshComponent>(e);
         return {{"path", rm.meshes->GetPath(m.mId)}};
       },
       .deserialize =
           [](Entity e, const Json &j, Coordinator &c, ResourceContext &rm) {
             if (!c.HasComponent<MeshComponent>(e))
               c.AddComponent(e, MeshComponent{});
             auto &m = c.GetComponent<MeshComponent>(e);
             m.mId = rm.meshes->LoadMesh(j["path"]);
           }});

  // --- PointLightComponent ---
  registry.RegisterComponent<PointLightComponent>(
      {.schema_version = 1,
       .serialize = [](Entity e, Coordinator &c, ResourceContext &) -> Json {
         const auto &p = c.GetComponent<PointLightComponent>(e);
         return {{"color_r", p.lightColor.r}, {"color_g", p.lightColor.g},
                 {"color_b", p.lightColor.b}, {"intensity", p.intensity},
                 {"linear", p.linear},        {"constant", p.constant},
                 {"quadratic", p.quadratic}};
       },
       .deserialize =
           [](Entity e, const Json &j, Coordinator &c, ResourceContext &) {
             if (!c.HasComponent<PointLightComponent>(e))
               c.AddComponent(e, PointLightComponent{});
             auto &p = c.GetComponent<PointLightComponent>(e);
             p.lightColor.r = j["color_r"];
             p.lightColor.g = j["color_g"];
             p.lightColor.b = j["color_b"];
             p.intensity = j["intensity"];
             p.linear = j["linear"];
             p.constant = j["constant"];
             p.quadratic = j["quadratic"];
           }});

  // --- ShaderComponent ---
  registry.RegisterComponent<ShaderComponent>(
      {.schema_version = 1,
       .serialize = [](Entity e, Coordinator &c, ResourceContext &rm) -> Json {
         const auto &s = c.GetComponent<ShaderComponent>(e);
         std::pair<std::string, std::string> paths = rm.shaders->GetPath(s.mId);
         return {{"vertex_path", paths.first},
                 {"fragment_path", paths.second},
                 {"color_r", s.mObjectColor.r},
                 {"color_g", s.mObjectColor.g},
                 {"color_b", s.mObjectColor.b}};
       },
       .deserialize =
           [](Entity e, const Json &j, Coordinator &c, ResourceContext &rm) {
             if (!c.HasComponent<ShaderComponent>(e))
               c.AddComponent(e, ShaderComponent{});
             auto &s = c.GetComponent<ShaderComponent>(e);
             s.mId =
                 rm.shaders->LoadShader(j["fragment_path"], j["vertex_path"]);
             s.mObjectColor.r = j["color_r"];
             s.mObjectColor.g = j["color_g"];
             s.mObjectColor.b = j["color_b"];
           }});

  // --- SpotLightComponent ---
  registry.RegisterComponent<SpotLightComponent>(
      {.schema_version = 1,
       .serialize = [](Entity e, Coordinator &c, ResourceContext &) -> Json {
         const auto &s = c.GetComponent<SpotLightComponent>(e);
         return {{"color_r", s.lightColor.r},    {"color_g", s.lightColor.g},
                 {"color_b", s.lightColor.b},    {"intensity", s.intensity},
                 {"dir_x", s.direction.x},       {"dir_y", s.direction.y},
                 {"dir_z", s.direction.z},       {"cutOff", s.cutOff},
                 {"outerCutOff", s.outerCufOff}, {"linear", s.linear},
                 {"constant", s.constant},       {"quadratic", s.quadratic}};
       },
       .deserialize =
           [](Entity e, const Json &j, Coordinator &c, ResourceContext &) {
             if (!c.HasComponent<SpotLightComponent>(e))
               c.AddComponent(e, SpotLightComponent{});
             auto &s = c.GetComponent<SpotLightComponent>(e);
             s.lightColor.r = j["color_r"];
             s.lightColor.g = j["color_g"];
             s.lightColor.b = j["color_b"];
             s.intensity = j["intensity"];
             s.direction.x = j["dir_x"];
             s.direction.y = j["dir_y"];
             s.direction.z = j["dir_z"];
             s.cutOff = j["cutOff"];
             s.outerCufOff = j["outerCutOff"];
             s.linear = j["linear"];
             s.constant = j["constant"];
             s.quadratic = j["quadratic"];
           }});

  return registry;
}

App::~App() {
  if (mWindow)
    glfwDestroyWindow(mWindow);
  glfwTerminate();
}

// =============== MAIN LOOP =================
void App::Run() {
  auto renderer = mCoordinator.GetSystem<RenderSystem>();
  auto cameraSystem = mCoordinator.GetSystem<CameraSystem>();

  auto directionalLightSystem =
      mCoordinator.GetSystem<DirectionalLightSystem>();
  auto pointLightSystem = mCoordinator.GetSystem<PointLightSystem>();
  auto spotLightSystem = mCoordinator.GetSystem<SpotLightSystem>();

  // auto shader = mResources.shaders->LoadShader(
  //     "resources/shaders/default.frag", "resources/shaders/default.vert");
  // auto lightShader = mResources.shaders->LoadShader(
  //     "resources/shaders/color.frag", "resources/shaders/default.vert");
  //
  // auto surfaceMaterial =
  //     MaterialComponent{glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.5, 0.5, 0.5),
  //                       glm::vec3(0.7, 0.7, 0.7), 25};
  // auto material = MaterialComponent{glm::vec3(0.1f), glm::vec3(1.0f),
  //                                   glm::vec3(0.1f), 64.0f};
  //
  // auto cubeMaterial =
  //     MaterialComponent{glm::vec3(0.02, 0.17, 0.02), glm::vec3(0.07, 0.6,
  //     0.07),
  //                       glm::vec3(0.6, 0.7, 0.6), 0.6};
  //
  // // ======== CAMERA =======
  // Entity camera = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(camera, TransformComponent{});
  // mCoordinator.AddComponent(camera, CameraComponent{});
  //
  // mCoordinator.GetComponent<CameraComponent>(camera).mPitch = 0.5f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mDistance = 10.0f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mFov = 60.0f;
  //
  // // ======= LIGHT =======
  // Entity directionalLight1 = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(
  //     directionalLight1,
  //     DirectionalLightComponent{glm::vec3(1.0f, -1.0f, 0.0f),
  //                               glm::vec3(0.2f, 1.0f, 0.2f), 0.3f});
  //
  // // Entity directionalLight2 = mCoordinator.CreateEntity();
  // // mCoordinator.AddComponent(
  // //     directionalLight2,
  // //     DirectionalLightComponent{glm::vec3(-1.0f, -1.0f, 0.0f),
  // //                               glm::vec3(0.2f, 0.2f, 0.8f), 0.5f});
  //
  //
  // Entity pointLight1 = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(
  //     pointLight1, PointLightComponent{glm::vec3(1.0f, 0.5f, 0.2f), 6.0f});
  // mCoordinator.AddComponent(pointLight1,
  //                           TransformComponent{glm::vec3(0.0f, 2.0f,
  //                           -2.0f)});
  //
  // mCoordinator.AddComponent(
  //     pointLight1,
  //     MeshComponent{mResources.meshes->LoadMesh("resources/objects/cube.obj")});
  //
  // mCoordinator.AddComponent(
  //     pointLight1, ShaderComponent{lightShader, glm::vec3(1.0f, 0.5f,
  //     0.2f)});
  //
  // Entity pointLight2 = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(
  //     pointLight2, PointLightComponent{glm::vec3(0.2f, 0.5f, 1.0f), 6.0f});
  // mCoordinator.AddComponent(pointLight2,
  //                           TransformComponent{glm::vec3(0.0f, 2.0f, 2.0f)});
  // mCoordinator.AddComponent(
  //     pointLight2,
  //     MeshComponent{mResources.meshes->LoadMesh("resources/objects/cube.obj")});
  // mCoordinator.AddComponent(
  //     pointLight2, ShaderComponent{lightShader, glm::vec3(0.2f,
  //     0.5f, 1.0f)});
  //
  // // Entity spotLight = mCoordinator.CreateEntity();
  // // mCoordinator.AddComponent(
  // //     spotLight, SpotLightComponent{glm::vec3(0.5f, 0.9f, 0.9f), 8.0f,
  // //                                   glm::vec3(0.0f, -1.0f, -1.0f),
  // //                                   glm::cos(glm::radians(30.0f)),
  // //                                   glm::cos(glm::radians(40.0f))});
  // //
  // // mCoordinator.AddComponent(spotLight,
  // // TransformComponent{glm::vec3(0.0f, 3.0f, 4.0f)});
  // // mCoordinator.AddComponent(spotLight,
  // // MeshComponent{mResources.meshes.LoadMesh(
  // // "resources/objects/cube.obj")});
  // // mCoordinator.AddComponent(
  // //     spotLight, ShaderComponent{lightShader, glm::vec3(0.5f, 0.9f,
  // // 0.9f)});
  //
  // // ====== PLACE ======
  //
  // Entity surface1 = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(surface1,
  // MeshComponent{mResources.meshes->LoadMesh(
  //                                         "resources/objects/surface.obj")});
  // mCoordinator.AddComponent(
  //     surface1, ShaderComponent{shader, glm::vec3(0.3f, 0.3f, 0.3f)});
  //
  // mCoordinator.AddComponent(surface1,
  //                           TransformComponent{glm::vec3(0.0f, -1.0f, 0.0f),
  //                                              glm::vec3(0.0f, 0.0f, 0.0f),
  //                                              glm::vec3(7.0f)});
  // mCoordinator.AddComponent(surface1, surfaceMaterial);
  //
  // // ====== WALL =====
  // // Entity surface2 = mCoordinator.CreateEntity();
  // // mCoordinator.AddComponent(surface2,
  // // MeshComponent{mResources.meshes.LoadMesh(
  // // "resources/objects/surface.obj")});
  // // mCoordinator.AddComponent(
  // //     surface2, ShaderComponent{shader, glm::vec3(0.3f, 0.3f, 0.3f)});
  // //
  // // mCoordinator.AddComponent(
  // //     surface2, TransformComponent{glm::vec3(3.0f, 0.0f, 0.0f),
  // //                                  glm::vec3(0.0f, 0.0f,
  // //                                  glm::radians(90.0f)),
  // glm::vec3(7.0f)});
  // // mCoordinator.AddComponent(surface2, surfaceMaterial);
  // // mCoordinator.AddComponent(surface2, surfaceRigidBody);
  // // mCoordinator.AddComponent(
  // //     surface2, ColliderComponent{glm::vec3(0.0f, 5.0f, 5.1f), true});
  // //
  // // ====== CUBE 1 ======
  //
  // Entity cube1 = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(cube1, MeshComponent{mResources.meshes->LoadMesh(
  //                                      "resources/objects/cube.obj")});
  // mCoordinator.AddComponent(
  //     cube1, ShaderComponent{shader, glm::vec3(0.5f, 0.5f, 1.0f)});
  // mCoordinator.AddComponent(cube1,
  //                           TransformComponent{glm::vec3(2.0f, 1.0f, 0.0f)});
  // mCoordinator.AddComponent(cube1, cubeMaterial);
  //
  // // ======= CUBE 2 ============
  // Entity cube2 = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(cube2, MeshComponent{mResources.meshes->LoadMesh(
  //                                      "resources/objects/cube.obj")});
  // mCoordinator.AddComponent(
  //     cube2, ShaderComponent{shader, glm::vec3(1.0f, 0.5f, 0.5f)});
  // mCoordinator.AddComponent(cube2,
  //                           TransformComponent{glm::vec3(-2.0f, 0.0f,
  //                           0.0f)});
  // mCoordinator.AddComponent(cube2, material);
  //
  // mCoordinator.GetComponent<TransformComponent>(cube2);

  // mSceneManager->LoadScene("resources/scenes/scene1.json");
  //
  // auto shader = mResources.shaders->LoadShader(
  //     "resources/shaders/default.frag", "resources/shaders/default.vert");
  //
  // Entity directionLight = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(
  //     directionLight,
  //     DirectionalLightComponent{.direction = glm::vec3(0.2f, -1.0f, 0.0f),
  //                               .lightColor = glm::vec3(1.0f),
  //                               .intensity = 0.0f});
  //
  // Entity piramid = mCoordinator.CreateEntity();
  // glm::vec3 lightColor = glm::vec3(0.3f, 0.9f, 0.3f);
  // mCoordinator.AddComponent(piramid,
  // MeshComponent{mResources.meshes->LoadMesh(
  //                                        "resources/objects/cube.obj")});
  // mCoordinator.AddComponent(
  //     piramid, ShaderComponent{mResources.shaders->LoadShader(
  //                                  "resources/shaders/color.frag",
  //                                  "resources/shaders/default.vert"),
  //                              lightColor});
  // mCoordinator.AddComponent(
  //     piramid, TransformComponent{.mRotation = glm::vec3(0.5f, 0.1f, 0.3f),
  //                                 .mScale = glm::vec3(2.0f)});
  // mCoordinator.AddComponent(
  //     piramid,
  //     PointLightComponent{.lightColor = lightColor, .intensity = 6.0f});
  //
  // Entity camera = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(camera, TransformComponent{});
  // mCoordinator.AddComponent(camera, CameraComponent{});
  //
  // mCoordinator.GetComponent<CameraComponent>(camera).mPitch = 0.4f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mYaw = -1.3f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mDistance = 13.0f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mFov = 60.0f;
  //
  // auto createTentacle = [&](const glm::vec3 &tentacleDir) {
  //   glm::vec3 lastPos = glm::vec3(0.0f);
  //
  //   glm::vec3 forward = glm::normalize(tentacleDir);
  //   glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
  //   if (glm::abs(glm::dot(forward, up)) > 0.99f)
  //     up = glm::vec3(1.0f, 0.0f, 0.0f);
  //
  //   glm::vec3 right = glm::normalize(glm::cross(up, forward));
  //   up = glm::cross(forward, right);
  //
  //   float padding = 3.0f;
  //
  //   for (int i = 0; i < 20; ++i) {
  //     float k = (float)i;
  //
  //     float scale = 0.8f + k * 0.2f;
  //     float spacing = scale * 0.5f;
  //
  //     Entity cube = mCoordinator.CreateEntity();
  //     mCoordinator.AddComponent(cube,
  //     MeshComponent{mResources.meshes->LoadMesh(
  //                                         "resources/objects/piramid.obj")});
  //     mCoordinator.AddComponent(cube, ShaderComponent{shader,
  //     glm::vec3(1.0f)});
  //
  //     float offsetForward = padding + k * spacing;
  //     float offsetRight = 0.5f * sin(k);
  //     float offsetUp = 0.5f * cos(k);
  //
  //     glm::vec3 pos =
  //         offsetForward * forward + offsetRight * right + offsetUp * up;
  //
  //
  //     glm::vec3 dir = glm::normalize(pos - lastPos);
  //
  //     glm::quat rotQuat = glm::quatLookAt(dir, up);
  //     glm::vec3 rotEuler = glm::eulerAngles(rotQuat);
  //
  //     mCoordinator.AddComponent(cube,
  //                               TransformComponent{.mPosition = pos,
  //                                                  .mRotation = rotEuler,
  //                                                  .mScale =
  //                                                  glm::vec3(scale)});
  //     mCoordinator.AddComponent(
  //         cube, MaterialComponent{.ambient = glm::vec3(0.32f, 0.22f, 0.02f),
  //                                 .diffuse = glm::vec3(0.78, 0.56, 0.11),
  //                                 .specular = glm::vec3(0.99, 0.94, 0.80),
  //                                 .shininess = 0.21f});
  //
  //     lastPos = pos;
  //   }
  // };
  //
  // createTentacle(glm::vec3(0.1f, 1.0f, 0.0f));
  // createTentacle(glm::vec3(0.0f, -1.0f, -0.2f));
  //
  // createTentacle(glm::vec3(0.0f, 0.2f, 1.0f));
  // createTentacle(glm::vec3(-0.1f, 0.08f, -1.0f));
  //
  // createTentacle(glm::vec3(1.0f, -0.2f, 0.1f));
  // createTentacle(glm::vec3(-1.0f, 0.1f, 0.0f));
  //
  //
  // mSceneManager->SaveScene("resources/scenes/scene2.json");
  //

  // auto surfaceMaterial =
  //     MaterialComponent{glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.55, 0.55, 0.55),
  //                       glm::vec3(0.7, 0.7, 0.7), 25};
  //
  // auto shader = mResources.shaders->LoadShader(
  //     "resources/shaders/default.frag", "resources/shaders/default.vert");
  //
  // Entity camera = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(camera, TransformComponent{});
  // mCoordinator.AddComponent(camera, CameraComponent{});
  //
  // // mCoordinator.GetComponent<CameraComponent>(camera).mPitch = -0.05f;
  //
  // mCoordinator.GetComponent<CameraComponent>(camera).mPitch = 0.3f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mYaw = 0.0f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mDistance = 5.0f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mFov = 60.0f;
  // mCoordinator.GetComponent<CameraComponent>(camera).mAutoRotate = true;
  //
  // Entity directionalLight = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(
  //     directionalLight,
  //     DirectionalLightComponent{.direction = glm::vec3(0.2f, -1.0f, 0.3f), .lightColor=glm::vec3(0.0f, 0.4f, 0.8f),
  //                               .intensity = 0.2f});
  //
  // auto lightShader = mResources.shaders->LoadShader(
  //     "resources/shaders/color.frag", "resources/shaders/default.vert");
  // auto createSpotLight = [&](const glm::vec3 &color, const glm::vec3 &pos) {
  //   Entity spotLight = mCoordinator.CreateEntity();
  //   mCoordinator.AddComponent(spotLight,
  //                             MeshComponent{mResources.meshes->LoadMesh(
  //                                 "resources/objects/piramid.obj")});
  //   mCoordinator.AddComponent(spotLight, ShaderComponent{lightShader, color});
  //   mCoordinator.AddComponent(spotLight, TransformComponent{.mPosition = pos});
  //
  //   glm::vec3 ligthDirection = glm::normalize(glm::vec3(0.0f) - pos);
  //
  //   mCoordinator.AddComponent(
  //       spotLight,
  //       SpotLightComponent{.lightColor = color,
  //                          .intensity = 2.0f,
  //                          .direction = ligthDirection,
  //                          .cutOff = glm::cos(glm::radians(30.0f)),
  //                          .outerCufOff = glm::cos(glm::radians(40.0f))});
  // };
  //
  // float k = 6.0f;
  // float h = 2.0f;
  // createSpotLight(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0, h, 0.57f * k));
  //
  // createSpotLight(glm::vec3(0.0f, 1.0f, 0.0f),
  //                 glm::vec3(-0.5 * k, h, -0.28 * k));
  // createSpotLight(glm::vec3(0.0f, 0.0f, 1.0f),
  //                 glm::vec3(0.5 * k, h, -0.28 * k));
  //
  // Entity surface = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(surface, MeshComponent{mResources.meshes->LoadMesh(
  //                                        "resources/objects/surface.obj")});
  // mCoordinator.AddComponent(
  //     surface, ShaderComponent{shader, glm::vec3(1.0f, 1.0f, 1.0f)});
  //
  // mCoordinator.AddComponent(surface,
  //                           TransformComponent{glm::vec3(0.0f, -0.45f, 0.0f),
  //                                              glm::vec3(0.0f, 0.0f, 0.0f),
  //                                              glm::vec3(10.0f)});
  // mCoordinator.AddComponent(surface, surfaceMaterial);
  //
  // // auto monkeyMaterial = MaterialComponent{glm::vec3(0.17f, 0.01f, 0.01f),
  // //                                         glm::vec3(0.61f, 0.04f, 0.04f),
  // //                                         glm::vec3(0.72f, 0.62f, 0.62f), 0.6};
  //
  // auto monkeyMaterial = MaterialComponent{glm::vec3(0.24f, 0.19f, 0.07f),
  //                                         glm::vec3(0.75f, 0.6f, 0.22f),
  //                                         glm::vec3(0.62f, 0.55f, 0.36f), 0.4};
  // Entity monkey = mCoordinator.CreateEntity();
  // mCoordinator.AddComponent(monkey,
  //                           MeshComponent{mResources.meshes->LoadMesh(
  //                               "resources/objects/blender_monkey.obj")});
  // mCoordinator.AddComponent(monkey, ShaderComponent{shader});
  // mCoordinator.AddComponent(
  //     monkey, TransformComponent{
  //                 .mPosition = glm::vec3(0.6f, -1.4f, 0.5f),
  //                 .mRotation = glm::vec3(glm::radians(-35.0f), 0.0f, 0.3f)});
  // mCoordinator.AddComponent(monkey, monkeyMaterial);
  //
  // mSceneManager->SaveScene("resources/scenes/scene3.json");

  mSceneManager->LoadScene("resources/scenes/scene1.json");
  static bool spaceWasPressed = false;
  static bool keyWasPressed[10] = {false};

  while (!glfwWindowShouldClose(mWindow)) {
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - mLastFrameTime;
    mLastFrameTime = currentTime;

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i <= 9; ++i) {
      if (glfwGetKey(mWindow, GLFW_KEY_0 + i) == GLFW_PRESS) {
        if (!keyWasPressed[i]) {
          std::string filename =
              "resources/scenes/scene" + std::to_string(i) + ".json";
          mCoordinator.DestroyAllEntities();
          mResources.meshes->Clear();
          mResources.shaders->Clear();

          mSceneManager->LoadScene(filename);
          keyWasPressed[i] = true;
        }
      } else {
        keyWasPressed[i] = false;
      }
    }

    directionalLightSystem->Update(mCoordinator, mUniformManager);
    pointLightSystem->Update(mCoordinator, mUniformManager);
    spotLightSystem->Update(mCoordinator, mUniformManager);

    cameraSystem->Update(mCoordinator, deltaTime);
    cameraSystem->UploadToUBO(mCoordinator, mUniformManager,
                              (float)mWidth / mHeight);
    renderer->Update(mCoordinator, mResources, mUniformManager);

    if (glfwGetKey(mWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
      if (!spaceWasPressed) {
        cameraSystem->ToggleCamera(mCoordinator);
        spaceWasPressed = true;
      }
    } else {
      spaceWasPressed = false;
    }

    glfwSwapBuffers(mWindow);
    glfwPollEvents();
  }
}
