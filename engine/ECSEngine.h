#pragma once

#include <cstddef>
#include <iostream>
#include <optional>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

// What systems see of the engine: one getter per resource manager.
//   - Systems fetch managers through these getters on every Run; they never
//     keep their own pointer to a manager.
//   - The returned references are valid for the engine's whole life. Data you
//     get from a manager has its own rules (see that manager's comment).
template <typename... Components>
class EngineView {
public:
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    // Runs every system once per frame until a system returns kExit or the window closes.
    void Run();

    // The engine owns all managers. Each reference is valid as long as the engine exists.
    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    // Only used by main to add systems; systems themselves don't see it.
    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

private:
    // Handles window events and updates every InputComponent. Returns false
    // when the window is closed.
    bool ProcessEvents();
    // Sets or clears one key's bit in every InputComponent.
    void SetKey(sf::Keyboard::Scancode key, bool down);
    // Handles the F1/F2/F3 debug keys. Returns true if `key` was one of them.
    bool HandleDebugKey(sf::Keyboard::Scancode key);
    // Outlines every collision box (red = moving, green = static).
    void DebugDraw();

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;

    // For debugging - pause and step the engine
    bool running;  // false while paused (F1)
    bool step;     // run exactly one frame while paused (F2)
    int debug;     // 0 = off, 1 = draw collision boxes (F3)
};

// Systems such as the dash timer count frames, so the game runs at a fixed rate.
inline constexpr unsigned int kFramesPerSecond = 60;

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name), running(true), step(false), debug(0) {
    mWindowManager.GetWindow()->setFramerateLimit(kFramesPerSecond);
}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {}

// One loop pass is one frame:
//   1. ProcessEvents  - read the keyboard and window events
//   2. systems        - update the game; the SpriteSystem clears and draws the sprites
//   3. DebugDraw      - collision boxes on top, if turned on
//   4. display()      - show the finished frame (this also waits to keep 60 FPS)
// The loop ends when the window is closed or a system returns kExit.
template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();
    while (window->isOpen()) {
        if (!ProcessEvents()) break;

        if (!running && !step) {
            // Paused: keep showing the last frame. display() is what normally
            // waits for the next frame, so sleep instead of spinning the CPU.
            sf::sleep(sf::milliseconds(1000 / kFramesPerSecond));
            continue;
        }
        step = false;

        if (!mSystemManager.Run(this)) break;
        if (debug) DebugDraw();
        window->display();
    }
    window->close();
}

template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    // Remember last frame's keys so systems can tell a new press
    // (keydown set, lastKeydown clear) from a key that is being held.
    auto& inputs = mEntityManager.template GetComponentStorage<InputComponent>();
    for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
        if (inputs.mValid[i]) inputs.mStorage[i].lastKeydown = inputs.mStorage[i].keydown;
    }

    std::shared_ptr<sf::RenderWindow> window = mWindowManager.GetWindow();
    while (const std::optional<sf::Event> event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            return false;
        } else if (const auto* pressed = event->getIf<sf::Event::KeyPressed>()) {
            if (!HandleDebugKey(pressed->scancode)) SetKey(pressed->scancode, true);
        } else if (const auto* released = event->getIf<sf::Event::KeyReleased>()) {
            SetKey(released->scancode, false);
        } else if (event->is<sf::Event::FocusLost>()) {
            // Key releases are missed while another window has focus, so let
            // go of every key instead of leaving one stuck down.
            for (auto& input : inputs.mStorage) input.keydown.reset();
        }
    }
    return true;
}

template <typename... Components>
void ECSEngine<Components...>::SetKey(sf::Keyboard::Scancode key, bool down) {
    if (key == sf::Keyboard::Scancode::Unknown) return;  // -1, not a valid bit
    const auto bit = static_cast<size_t>(key);

    auto& inputs = mEntityManager.template GetComponentStorage<InputComponent>();
    for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
        if (inputs.mValid[i]) inputs.mStorage[i].keydown.set(bit, down);
    }
}

template <typename... Components>
bool ECSEngine<Components...>::HandleDebugKey(sf::Keyboard::Scancode key) {
    switch (key) {
        case sf::Keyboard::Scancode::F1:  // pause / resume
            running = !running;
            std::cout << (running ? "Resumed\n" : "Paused (F2 steps one frame)\n");
            return true;
        case sf::Keyboard::Scancode::F2:  // advance one frame, staying paused
            running = false;
            step = true;
            return true;
        case sf::Keyboard::Scancode::F3:  // collision boxes on / off
            debug = debug ? 0 : 1;
            return true;
        default:
            return false;
    }
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {
    auto& collisions = mEntityManager.template GetComponentStorage<CollisionComponent>();
    for (size_t i = 0; i < collisions.mStorage.size(); ++i) {
        if (!collisions.mValid[i]) continue;
        const CollisionComponent& c = collisions.mStorage[i];
        // The box is stored relative to the entity's location.
        Rect box = c.collisionBox;
        const EntityID owner = collisions.mOwner[i];
        if (mEntityManager.template HasComponent<LocationComponent>(owner)) {
            const Point2D& loc = mEntityManager.template GetComponent<LocationComponent>(owner).loc;
            box.topLeft = Point2D{box.topLeft.x + loc.x, box.topLeft.y + loc.y};
        }
        box = mWindowManager.WorldToWindow(box);
        if (c.staticEntity) {
            mWindowManager.DebugFrameRect(box, 0, 255, 0);
        } else {
            mWindowManager.DebugFrameRect(box, 255, 0, 0);
        }
    }
}

}  // namespace ECSEngine
