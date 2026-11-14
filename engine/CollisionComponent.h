#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// A collision box. 1-bit flags keep the component small.
struct CollisionComponent {
    uint8_t staticEntity : 1;   // never moves (walls, floor); others get pushed out of it
    uint8_t doesDamage : 1;     // touching it sends the player back to respawn (spikes)
    // Set by CollisionSystem each frame: which sides are touching something solid
    uint8_t onGround : 1;
    uint8_t onCeiling : 1;
    uint8_t onLeftWall : 1;
    uint8_t onRightWall : 1;
    Rect collisionBox;          // relative to the entity's location
    Rect previousCollisionBox;  // last frame's box in world space, saved by CollisionLateSystem
};

}  // namespace ECSEngine
