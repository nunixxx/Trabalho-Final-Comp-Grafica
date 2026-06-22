# Este arquivo CMakeLists.txt foi adaptado a partir do projeto castor
# do PET INF/UFRGS (https://github.com/petcomputacaoufrgs/castor-fcg),
# com algumas modificações vindas do arquivo CMakeLists.txt criado
# pelos alunos Luis Melo e Santiago Gonzaga em 2023/1.

# Arquivos fonte C/C++. Inclua nesta lista todos os arquivos que devem
# ser compilados.
set(SOURCES
  src/main.cpp
  src/correcao.cpp
  src/textrendering.cpp
  src/tiny_obj_loader.cpp
  src/stb_image.cpp
  src/glad.c
  src/modelRendering/model_rendering.cpp
  src/collision/collision.cpp
  src/textureRendering/texture_rendering.cpp
  src/classes/game_object.cpp
  src/classes/player/player.cpp
  src/classes/enemy/enemy.cpp
  src/classes/objects/world_object.cpp
  src/classes/objects/health_pack.cpp
  src/classes/objects/gun.cpp
  src/hitbox/hitbox_renderer.cpp
  src/classes/objects/armor.cpp
  src/animation/skeletal_animation.cpp
  src/animation/animation_system.cpp
)

cmake_minimum_required(VERSION 3.10)
project(LAB_FCG VERSION 1.0.0)

set(CMAKE_CXX_STANDARD          14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${PROJECT_SOURCE_DIR}/bin/Debug")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin/Release")
elseif(UNIX)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/Linux")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(EXECUTABLE_NAME main)

foreach(source_file IN LISTS SOURCES)
  if(NOT EXISTS ${PROJECT_SOURCE_DIR}/${source_file})
    message(FATAL_ERROR "O arquivo ${PROJECT_SOURCE_DIR}/${source_file} não existe.")
    break()
  endif()
endforeach()

add_executable(${EXECUTABLE_NAME} ${SOURCES})

target_include_directories(${EXECUTABLE_NAME} BEFORE PRIVATE ${PROJECT_SOURCE_DIR}/include)

# =============================================================
# Windows
# =============================================================
if(WIN32)

  if(MINGW)
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} "-v"
      ERROR_VARIABLE  COMPILER_VERSION_OUTPUT
      RESULT_VARIABLE COMPILER_VERSION_RESULT
    )

    if(COMPILER_VERSION_RESULT EQUAL 0)
      if(COMPILER_VERSION_OUTPUT MATCHES "ucrt64")
        set(LIBGLFW      ${PROJECT_SOURCE_DIR}/lib-ucrt-64/libglfw3.a)
        set(ASSIMP_INCLUDE "C:/msys64/ucrt64/include")
        set(ASSIMP_LIB     "C:/msys64/ucrt64/lib/libassimp.dll.a")
      elseif(COMPILER_VERSION_OUTPUT MATCHES "mingw64")
        set(LIBGLFW      ${PROJECT_SOURCE_DIR}/lib-mingw-64/libglfw3.a)
        set(ASSIMP_INCLUDE "C:/msys64/mingw64/include")
        set(ASSIMP_LIB     "C:/msys64/mingw64/lib/libassimp.dll.a")
      else()
        set(LIBGLFW      ${PROJECT_SOURCE_DIR}/lib-mingw-32/libglfw3.a)
        set(ASSIMP_INCLUDE "C:/msys64/mingw32/include")
        set(ASSIMP_LIB     "C:/msys64/mingw32/lib/libassimp.dll.a")
      endif()
    else()
      message(FATAL_ERROR "Failed to get MinGW compiler version.")
    endif()

  elseif(MSVC)
    set(LIBGLFW        ${PROJECT_SOURCE_DIR}/lib-vc2022/glfw3.lib)
    set(ASSIMP_INCLUDE "C:/Program Files/Assimp/include")
    set(ASSIMP_LIB     "C:/Program Files/Assimp/lib/x64/assimp-vc145-mt.lib")
  else()
    message(FATAL_ERROR "Only MINGW or MSVC supported on Windows.")
  endif()

  message(STATUS "LIBGLFW      = ${LIBGLFW}")
  message(STATUS "ASSIMP_LIB   = ${ASSIMP_LIB}")
  message(STATUS "ASSIMP_INCLUDE = ${ASSIMP_INCLUDE}")

  target_include_directories(${EXECUTABLE_NAME} PRIVATE
      ${ASSIMP_INCLUDE}
  )

  target_link_libraries(${EXECUTABLE_NAME}
      ${LIBGLFW}
      gdi32
      opengl32
      ${ASSIMP_LIB}
  )

# =============================================================
# Linux
# =============================================================
elseif(UNIX)

  target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wno-unused-function)

  add_custom_target(run
      COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ./main
      DEPENDS main
      USES_TERMINAL
  )

  find_package(OpenGL REQUIRED)
  find_package(X11 REQUIRED)
  find_library(MATH_LIBRARY m)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  find_package(assimp REQUIRED)

  target_include_directories(${EXECUTABLE_NAME} PRIVATE
      ${ASSIMP_INCLUDE_DIRS}
  )

  target_link_libraries(${EXECUTABLE_NAME}
      ${CMAKE_DL_LIBS}
      ${MATH_LIBRARY}
      ${PROJECT_SOURCE_DIR}/lib-linux/libglfw3.a
      ${CMAKE_THREAD_LIBS_INIT}
      ${OPENGL_LIBRARIES}
      ${X11_LIBRARIES}
      ${X11_Xrandr_LIB}
      ${X11_Xcursor_LIB}
      ${X11_Xinerama_LIB}
      assimp::assimp
  )

endif()