#pragma once

#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

namespace ECSEngine {

// Loads sound files and plays them back by name, e.g.
//   sounds.RegisterSound(path + "sfx_jump.ogg", "jump");   // while loading
//   sounds.GetSound("jump").play();                         // in a system
// See SoundManager.cpp for how it works and how long references stay valid.
class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;

    // No copying: copied sounds would still point at the original's buffers.
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    // Makes the file at `soundPath` playable as `soundName`. Throws if it can't load.
    void RegisterSound(const std::string& soundPath, const std::string& soundName);

    // Returns the sound named `soundName`. Throws for an unknown name.
    sf::Sound& GetSound(const std::string& soundName);

private:
    // Returns the audio data for `path`, loading it the first time.
    const sf::SoundBuffer& LoadBuffer(const std::string& path);

    // Declared first so it is destroyed last: every sf::Sound points into a buffer.
    std::unordered_map<std::string, sf::SoundBuffer> mBuffers;  // keyed by file path
    std::unordered_map<std::string, sf::Sound> mSounds;         // keyed by sound name
};

}  // namespace ECSEngine
