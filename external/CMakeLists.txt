add_subdirectory(glfw)

add_library(glad glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include)

add_subdirectory(glm)

add_library(external_libs INTERFACE)
target_include_directories(external_libs INTERFACE nlohmann)

target_link_libraries(external_libs INTERFACE glfw glad glm)
