#pragma once

#include "Keyboard.h"

namespace ECSEngine {

const float kMaxRunVelocity = 15;
const float kDashVelocity = 50;
const float kJumpVelocity = 25;
const float kGroundRunAcceleration = 5;
const float kAirRunAcceleration = 4;
const float kDashInterval = 25;

// additional variables
const int kDashFrames = 4;              // how long one dash lasts
const float kWallSlideVelocity = 4;     // fastest fall speed while sliding down a wall
const float kJumpCutFactor = 0.4f;      // letting go of jump early keeps 40% of the upward speed

/**
 * Processes user input for player movement.
 * A/left = move left
 * D/right = move right
 * Z/W/Space/Up = jump (let go early for a lower jump)
 * C/Left Shift = dash
 */
template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto &entities{engine->GetEntityManager()};
        auto &sounds  {engine->GetSoundManager()};

        // loop through all input components
        auto &inputs{entities.template GetComponentStorage<InputComponent>()};
        for (size_t i{0}; i < inputs.mStorage.size(); i++) {
            // Edge case: invalid input component
            if (!inputs.mValid[i])
                continue;
            
            EntityID owner{inputs.mOwner[i]};
            // Edge case: needs a velocity to change and collision flags to read
            if (!entities.template HasComponent<MovementComponent>(owner) ||
                !entities.template HasComponent<CollisionComponent>(owner)) {
                continue;
            }

            InputComponent &input{inputs.mStorage[i]};
            Point2D &velocity{entities.template GetComponent<MovementComponent>(owner).velocity};
            CollisionComponent &touching{entities.template GetComponent<CollisionComponent>(owner)};

            // Booleans for keys pressed & if the player is on the ground or not
            bool left       {Down(input, Key::Left) || Down(input, Key::A)};
            bool right      {Down(input, Key::Right) || Down(input, Key::D)};
            bool jumpPressed {JumpKey(input.keydown) && !JumpKey(input.lastKeydown)};
            bool jumpReleased{!JumpKey(input.keydown) && JumpKey(input.lastKeydown)};
            bool onGround   {static_cast<bool>(touching.onGround)};
            bool onWall     {!onGround && (touching.onLeftWall || touching.onRightWall)};

            // Case 1: new left shift press -> do a dash after kDashInterval frames have passed
            if (mFrame - input.lastDash >= kDashInterval && (Pressed(input, Key::C) || Pressed(input, Key::LShift))) {
                // dash direction you are holding, otherwise the way you are already moving.
                float direction{static_cast<float>(right) - static_cast<float>(left)};
                if (direction == 0) {
                    direction = static_cast<float>((velocity.x > 0) - (velocity.x < 0));
                }

                if (direction != 0) {
                    velocity = {direction * kDashVelocity, 0.0f};

                    // GravitySystem skips this entity while disabledTimer counts down
                    if (entities.template HasComponent<GravityComponent>(owner)) {
                        entities.template GetComponent<GravityComponent>(owner).disabledTimer = kDashFrames;
                    }

                    input.lastDash = mFrame;
                    sounds.GetSound("dash").play();
                }
            }

            // No steering allowed while dashing
            if (mFrame - input.lastDash < kDashFrames) {
                continue;
            }

            // Case 2: running. accelerate towards held direction, slow down when nothing is held
            float accel = onGround ? kGroundRunAcceleration : kAirRunAcceleration;
            if (left != right) {
                velocity.x += right ? accel : -accel;
            } else if (velocity.x > 0) {
                velocity.x = std::max(0.0f, velocity.x - accel);
            } else if (velocity.x < 0) {
                velocity.x = std::min(0.0f, velocity.x + accel);
            }
            velocity.x = std::clamp(velocity.x, -kMaxRunVelocity, kMaxRunVelocity);

            // Case 3: jump -> only jumps on a new press
            if (jumpPressed) {
                if (onGround) {
                    velocity.y = -kJumpVelocity;
                    sounds.GetSound("jump").play();
                } else if (onWall) {
                    // wall jump up and away from the wall
                    velocity.y = -kJumpVelocity;
                    velocity.x = touching.onLeftWall ? kMaxRunVelocity : -kMaxRunVelocity;
                    sounds.GetSound("jump").play();
                }
            }

            // Case 3b: short hop -> letting go of jump while still going up cuts the jump
            if (jumpReleased && velocity.y < 0) {
                velocity.y *= kJumpCutFactor;
            }

            // Case 4: wall slide -> falling while pushing into wall caps fall speed
            const bool pushingIntoWall{(touching.onLeftWall && left) || (touching.onRightWall && right)};
            if (onWall && pushingIntoWall && velocity.y > 0) {
                velocity.y = std::min(velocity.y, kWallSlideVelocity);
                sf::Sound &slide{sounds.GetSound("wallslide")};

                // don't restart sound while it's still playing since this runs every frame
                if (slide.getStatus() != sf::Sound::Status::Playing) {
                    slide.play();
                }
            }

        }

        mFrame++;
        return kContinue;
    }    
private:
    // Count frames for dash timer
    int mFrame{};

    // Helper function for checking if key is held down
    static bool Down(const InputComponent &input, unsigned int key) {
        return input.keydown[key];
    }

    // Helper function for checking if is down now but not the last frame (new press)
    static bool Pressed(const InputComponent &input, unsigned int key) {
        return input.keydown[key] && !input.lastKeydown[key];
    }

    // Helper function for checking if any jump key is down in a key set
    static bool JumpKey(const std::bitset<sf::Keyboard::ScancodeCount> &keys) {
        return keys[Key::Z] || keys[Key::Space] || keys[Key::Up] || keys[Key::W];
    }

};

}  // namespace ECSEngine
