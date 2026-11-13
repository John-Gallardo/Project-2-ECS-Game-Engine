#pragma once

#include <bitset>
#include <cmath>
#include <unordered_map>

#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "WindowManager.h"
#include "CameraComponent.h"
#include "EntityManager.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpriteSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 
        auto& entities = engine->GetEntityManager();
        auto& sprites = engine->GetSpriteManager();
        auto& windowManager = engine->GetWindowManager();
        auto window = windowManager.GetWindow();
        const sf::Vector2u windowSize = window->getSize();

        window->clear(sf::Color(135, 206, 235));

        // Walk only the sprite pool. Slots are in creation order, so things
        // created first (the sky are drawn first, underneath everything else)
        auto& spriteStore = entities.template GetComponentStorage<SpriteComponent>();
        for (size_t i = 0; i < spriteStore.mStorage.size(); ++i) {
            if (!spriteStore.mValid[i]) {
                continue;
            }

            const EntityID owner = spriteStore.mOwner[i];
            if (!entities.template HasComponent<LocationComponent>(owner)) {
                continue;
            }

            const SpriteComponent& sc = spriteStore.mStorage[i];
            const Point2D& loc = entities.template GetComponent<LocationComponent>(owner).loc;

            // bounds is relative to the entity's location
            Rect r{{loc.x + sc.bounds.topLeft.x, loc.y + sc.bounds.topLeft.y}, sc.bounds.width, sc.bounds.height};

            if (sc.worldSpace) {
                r = windowManager.WorldToWindow(r);
            }

            // Skip sprites that are completely off screen
            if (r.topLeft.x + r.width < 0 || r.topLeft.y + r.height < 0 || r.topLeft.x > windowSize.x || r.topLeft.y > windowSize.y) {
                continue;
            }

            // Several entities share one sf::Sprite, so set is position and
            // scale right before each draw. Scale stretches the source rect to r
            sf::Sprite& sprite = sprites.GetSprite(sc.textureID);
            const sf::IntRect source = sprite.getTextureRect();

            // Snap edges to whole pixels: an edge exactly on a pixel center makes
            // SFML sample the neighbouring tile in the sheet
            const float left = std::round(r.topLeft.x);
            const float top = std::round(r.topLeft.y);
            const float right = std::round(r.topLeft.x + r.width);
            const float bottom = std::round(r.topLeft.y + r.height);

            sprite.setPosition({left, top});
            sprite.setScale({(right - left) / source.size.x, (bottom - top) / source.size.y});
            window->draw(sprite);
        }

        // No display() here: ECSEngine::Run calls it after the debug drawing
        return kContinue; 
    }
};

}  // namespace ECSEngine
