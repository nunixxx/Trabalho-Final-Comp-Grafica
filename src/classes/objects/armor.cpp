#include "armor.h"

Armor::Armor(
    glm::vec4 position,
    float     scale,
    int       armorAmount,
    int       maxArmor
)
    : WorldObject(
        "armor",
        position,
        Matrix_Scale(scale, scale, scale),
        1.5f,
        1.2f
    )
    , armorAmount(armorAmount)
    , maxArmor(maxArmor)
{}

void Armor::onInteract(Player& player)
{
    if (consumed) return;

    int before = player.armor;
    player.armor = std::min(player.armor + armorAmount, maxArmor);
    int gained = player.armor - before;

    printf("[Armor] Player ganhou %d de armadura. Armadura atual: %d/%d\n",
           gained, player.armor, maxArmor);

    consumed = true;
    active   = false;
}
