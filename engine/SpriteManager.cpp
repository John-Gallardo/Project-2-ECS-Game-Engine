#include "SpriteManager.h"

#include <cassert>
#include <stdexcept>

namespace ECSEngine {

// Loads textures and hands out sprites cut from them.
//
// How it works
//   - mTextures holds each image file, keyed by path, so a file loads once.
//   - mSprites holds one sf::Sprite per RegisterTexture call. A SpriteID is
//     just an index into it.
//   - Many sprites can share one texture (e.g. every tile uses the same sheet).
//
// Lifetimes
//   - The SpriteManager owns every texture and sprite; callers never free them.
//   - A SpriteID stays valid for the SpriteManager's whole life.
//   - GetSprite references last only until the next RegisterTexture (the
//     vector may grow and move). Store the SpriteID, not the reference.

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    const sf::Texture& texture = LoadTexture(texturePath);
    const sf::IntRect area({static_cast<int>(sourceRect.topLeft.x),
                            static_cast<int>(sourceRect.topLeft.y)},
                           {static_cast<int>(sourceRect.width), static_cast<int>(sourceRect.height)});
    mSprites.emplace_back(texture, area);
    return mSprites.size() - 1;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    assert(id < mSprites.size());
    return mSprites[id];
}

const sf::Texture& SpriteManager::LoadTexture(const std::string& path) {
    auto found = mTextures.find(path);
    if (found != mTextures.end()) return found->second;

    sf::Texture texture;
    if (!texture.loadFromFile(path)) {
        throw std::runtime_error("SpriteManager: could not load texture '" + path + "'");
    }
    return mTextures.emplace(path, std::move(texture)).first->second;
}

}  // namespace ECSEngine
