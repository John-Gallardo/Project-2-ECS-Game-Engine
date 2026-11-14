#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// Index of a sprite inside the SpriteManager, returned by RegisterTexture.
using SpriteID = size_t;

// Loads textures and hands out sprites cut from them, e.g.
//   SpriteID star = sprites.RegisterTexture(path + "tiles.png", {{640, 320}, 64, 64});
//   sprites.GetSprite(star);   // in a system
// See SpriteManager.cpp for how it works and how long references stay valid.
class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;

    // No copying: copied sprites would still point at the original's textures.
    SpriteManager(const SpriteManager&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;

    // Makes a sprite showing the `sourceRect` part (in pixels) of the image. Throws if it can't load.
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    // Returns the sprite for `id`. Callers may set its position/scale before drawing.
    sf::Sprite& GetSprite(SpriteID id);

private:
    // Returns the texture for `path`, loading it the first time.
    const sf::Texture& LoadTexture(const std::string& path);

    std::unordered_map<std::string, sf::Texture> mTextures;  // keyed by file path
    std::vector<sf::Sprite> mSprites;                        // indexed by SpriteID
};

}  // namespace ECSEngine
