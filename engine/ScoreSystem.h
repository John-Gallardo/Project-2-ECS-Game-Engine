#pragma once

#include <algorithm>
#include <string>
#include "Entity.h"
#include "EntityManager.h"
#include "ScoreComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

// globals
const int kMaxScore{999};  // only 3 digits -> 999 is max

// Shows the score on screen as three digits (hundreds, tens, ones).
// CollisionSystem is what adds to the score when the player collects a star.
template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto &entities{engine->GetEntityManager()};

        // Go through entities with score component only
        auto &scores{entities.template GetComponentStorage<ScoreComponent>()};
        for (size_t i{0}; i < scores.mStorage.size(); i++) {
            // Edge case: invalid score component
            if (!scores.mValid[i]) {
                continue;
            }

            ScoreComponent &score{scores.mStorage[i]};
            int value{std::clamp(score.score, 0, kMaxScore)};

            // start from right-most slot
            for (int slot{2}; slot >= 0; slot--) {
                const int digit{value % 10};
                value /= 10;

                const EntityID digitEntity{score.digitID[slot]};

                // Edge case: no sprite to change
                if (!entities.template HasComponent<SpriteComponent>(digitEntity)) {
                    continue;
                }

                entities.template GetComponent<SpriteComponent>(digitEntity).textureID = score.digitTextureID[digit];
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
