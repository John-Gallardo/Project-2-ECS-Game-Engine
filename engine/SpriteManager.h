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

// Loads textures and hands out sprites cut from them.
//
// Ownership and lifetime
//   - The SpriteManager owns every texture and sprite. Callers never free them.
//   - Each texture file is loaded once and shared by every sprite that uses it.
//     Textures live in an unordered_map, whose elements never move, so the
//     texture a sprite points at stays valid for the SpriteManager's lifetime.
//   - A SpriteID stays valid for the SpriteManager's lifetime (sprites are
//     never removed).
//   - The reference returned by GetSprite is only valid until the next
//     RegisterTexture call, because the sprite vector may reallocate. Look it up
//     by SpriteID when you need it; do not store the reference. Register all
//     textures while loading, before the game loop starts.
class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;

    // Copying would leave the copied sprites pointing at the other manager's
    // textures, so a SpriteManager cannot be copied.
    SpriteManager(const SpriteManager&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;

    // Makes a sprite showing the `sourceRect` part (in pixels) of the image at
    // `texturePath`. The file is only loaded the first time its path is seen.
    // Throws std::runtime_error if the file cannot be loaded; nothing is added
    // in that case.
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    // Returns the sprite for `id`, which must come from RegisterTexture.
    // Callers may change its position/scale/colour before drawing it.
    sf::Sprite& GetSprite(SpriteID id);

private:
    // Returns the texture for `path`, loading it if this is the first request.
    const sf::Texture& LoadTexture(const std::string& path);

    std::unordered_map<std::string, sf::Texture> mTextures;  // keyed by file path
    std::vector<sf::Sprite> mSprites;                        // indexed by SpriteID
};

}  // namespace ECSEngine
