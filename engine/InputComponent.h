#pragma once

#include <bitset>
#include <unordered_map>

#include "Entity.h"
#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace ECSEngine {

// Keyboard state for an entity the player controls. The engine fills the bits
// every frame; InputSystem reads them.
struct InputComponent {
    InputComponent(EntityID whichEntity) : entity(whichEntity), lastDash(0) {}
    EntityID entity;                                       // the entity this input controls
    int lastDash;                                          // frame of the last dash (dash cooldown)
    std::bitset<sf::Keyboard::ScancodeCount> keydown;      // keys held this frame, indexed by scancode
    std::bitset<sf::Keyboard::ScancodeCount> lastKeydown;  // keys held last frame (to spot new presses)
};

}  // namespace ECSEngine
