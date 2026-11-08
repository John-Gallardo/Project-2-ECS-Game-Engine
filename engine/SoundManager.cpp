#include "SoundManager.h"

#include <stdexcept>

namespace ECSEngine {

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
