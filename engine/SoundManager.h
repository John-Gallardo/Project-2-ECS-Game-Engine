#pragma once

#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

namespace ECSEngine {

// Loads sound files and plays them back by name, e.g.
//   sounds.RegisterSound(path + "sfx_jump.ogg", "jump");   // while loading
//   sounds.GetSound("jump").play();                         // in a system
//
// Ownership and lifetime
//   - The SoundManager owns every sound and its audio data. Callers never free them.
//   - Each sound file is loaded once; several names registered with the same
//     file share its audio data but are separate sf::Sound objects, so they can
//     play at the same time.
//   - The reference returned by GetSound stays valid for the SoundManager's
//     lifetime (sounds live in an unordered_map, whose elements never move).
//     Registering the same name again replaces that sound in place.
class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;

    // Copying would leave the copied sounds pointing at the other manager's
    // audio data, so a SoundManager cannot be copied.
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    // Loads the file at `soundPath` (if not already loaded) and makes it
    // playable as `soundName`. Throws std::runtime_error if the file cannot be
    // loaded; nothing is added in that case.
    void RegisterSound(const std::string& soundPath, const std::string& soundName);

    // Returns the sound registered as `soundName`, ready to play() or adjust
    // (e.g. setVolume). Throws std::runtime_error for an unknown name.
    sf::Sound& GetSound(const std::string& soundName);

private:
    // Returns the audio data for `path`, loading it if this is the first request.
    const sf::SoundBuffer& LoadBuffer(const std::string& path);

    // Declared before mSounds so it is destroyed after them: every sf::Sound
    // points into one of these buffers.
    std::unordered_map<std::string, sf::SoundBuffer> mBuffers;  // keyed by file path
    std::unordered_map<std::string, sf::Sound> mSounds;         // keyed by sound name
};

}  // namespace ECSEngine
