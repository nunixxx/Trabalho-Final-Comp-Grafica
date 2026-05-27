#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define MAX_TEXTURES 128
#define PI 3.141592f
#define WINDOW_SIZE_X 800
#define WINDOW_SIZE_Y 600

#define PATH_CSV "../../data/paths.csv"

// Camera Settings
#define FARPLANE -200.0f
#define NEARPLANE -0.1f
#define FREECAM true
#define LOOKAT false
#define CAMERA_UP_VECTOR glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)
#define CAMERA_SPEED 0.1f
#define INITIAL_CAMERA_THETA 0.0f
#define INITIAL_CAMERA_PHI 0.4f
#define INITIAL_CAMERA_DISTANCE 5.0f

// Player initial values
#define PLAYER_INITIAL_POSITION glm::vec4(-38.08f, 0.74f, -161.84f, 1.0f)
#define PLAYER_INITIAL_YAW -1.57f
#define PLAYER_INITIAL_HEALTH 100
#define PLAYER_INITIAL_ARMOR 25
#define PLAYER_INITIAL_SPEED 0.05f
#define PLAYER_HALF_W 0.3f
#define PLAYER_HEIGHT 1.0f
#define PLAYER_RADIUS 0.3f

// Soldiers values
#define SOLDIERS_SCALE Matrix_Scale(0.1f, 0.1f, 0.1f)

#endif