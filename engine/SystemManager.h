#pragma once

#include <vector>
#include <iostream>
// #include "ECSEngine.h"

namespace ECSEngine {

// What a system returns from Run: kContinue keeps the game going, kExit ends it.
enum tSystemReturn { kContinue, kExit };

template <typename... Components>
class EngineView;

// Base class for every system. Run is called once per frame and gets the
// engine's managers through `scene`.
template <typename... Components>
class System {
public:
    virtual ~System() = default;
    virtual tSystemReturn Run(EngineView<Components...>* scene) = 0;
    virtual const std::string& GetName() const = 0;
};

// Holds every system and runs them in the order they were added.
//   - AddSystem takes ownership; systems are freed with the SystemManager.
//   - Run returns false as soon as a system returns kExit (later systems are skipped).
template <typename... Components>
class SystemManager {
public:
    bool Run(EngineView<Components...>* scene) {
        for (auto& system : mSystems) {
            if (system->Run(scene) == kExit)  // system wants to exit
            {
                std::cout << system->GetName() << " requested exit from system manager\n";
                return false;
            }
        }
        return true;
    }
    void AddSystem(std::unique_ptr<System<Components...>>&& system) {
        mSystems.push_back(std::move(system));
    }
    void ClearSystems() { mSystems.clear(); }

private:
    std::vector<std::unique_ptr<System<Components...>>> mSystems;
};

}  // namespace ECSEngine
