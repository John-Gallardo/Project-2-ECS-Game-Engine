#include "SoundManager.h"

#include <stdexcept>

namespace ECSEngine {

// Loads sound files and plays them back by name.
//
// How it works
//   - mBuffers holds each file's audio data, keyed by path, so a file loads once.
//   - mSounds maps a name (e.g. "jump") to an sf::Sound that plays one buffer.
//   - Several names can share a buffer and still play at the same time.
//
// Lifetimes
//   - The SoundManager owns every sound and buffer; callers never free them.
//   - GetSound references stay valid for the SoundManager's whole life
//     (unordered_map elements never move).
//   - Registering a name again replaces that sound.

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    const sf::SoundBuffer& buffer = LoadBuffer(soundPath);
    mSounds.insert_or_assign(soundName, sf::Sound(buffer));
}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    auto found = mSounds.find(soundName);
    if (found == mSounds.end()) {
        throw std::runtime_error("SoundManager: no sound registered as '" + soundName + "'");
    }
    return found->second;
}

const sf::SoundBuffer& SoundManager::LoadBuffer(const std::string& path) {
    auto found = mBuffers.find(path);
    if (found != mBuffers.end()) return found->second;

    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(path)) {
        throw std::runtime_error("SoundManager: could not load sound '" + path + "'");
    }
    return mBuffers.emplace(path, std::move(buffer)).first->second;
}

}  // namespace ECSEngine
