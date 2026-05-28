#pragma once
#include <glad/glad.h>
#include "..\modelRendering\model_rendering.h"


GLuint LoadTextureImage(const char* filename);
void LoadMaterialTextures(ObjModel* model, const char* basepath);
void BindAllTextures(GLuint program_id);