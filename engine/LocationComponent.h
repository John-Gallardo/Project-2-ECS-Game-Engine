#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// This is a top-left location of an entity all other stored locations are relative to this
// location. This means that moving this single location will move anything else which is relative
// to the location such as:
// - sprite bounding boxes
// - collision bounding boxes
struct LocationComponent {
    Point2D loc;
};

}  // namespace ECSEngine
