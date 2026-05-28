#include "game_object.h"

// =========================================================
// buildModelMatrix
//
// Compõe a transformação padrão do objeto:
//   Translate(position) * RotateY(yaw) * Scale(matrixScale)
//
// A ordem importa: primeiro escala o modelo no espaço local,
// depois rotaciona, depois posiciona no mundo.
// =========================================================
glm::mat4 GameObject::buildModelMatrix() const
{
    return Matrix_Translate(position.x, position.y, position.z)
         * Matrix_Rotate_Y(yaw)
         * matrixScale;
}

// =========================================================
// draw
//
// Implementação padrão: monta a matrix e chama DrawModel().
// Subclasses podem sobrescrever para casos especiais
// (ex: Enemy que também desenha a arma com outra matrix).
// =========================================================
void GameObject::draw()
{
    if (!active) return;
    DrawModel(modelName, buildModelMatrix());
}
