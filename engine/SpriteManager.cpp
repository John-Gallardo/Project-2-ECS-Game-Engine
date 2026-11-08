#include "SpriteManager.h"

#include <cassert>
#include <stdexcept>

namespace ECSEngine {

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
