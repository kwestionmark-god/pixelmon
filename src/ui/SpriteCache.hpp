#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace pm {

// Downloads PokeAPI sprite URLs (PNG) on demand, caches them on disk, and
// exposes them as SFML textures. Repeated get() calls for the same URL reuse
// the cached download and the in-memory texture.
class SpriteCache {
public:
    explicit SpriteCache(const std::string& cacheDir);

    // Returns a pointer to the loaded texture, or nullptr on failure
    // (missing curl, network error, or corrupt image).
    sf::Texture* get(const std::string& url);

    // Returns a pointer to the raw sprite pixels (the cached PNG decoded into
    // an sf::Image), or nullptr on failure. Used to build output images that
    // must read pixel data back, which sf::Texture cannot do in SFML 2.6.
    sf::Image* image(const std::string& url);
    bool isLoaded(const std::string& url) const;

    std::size_t loadedCount() const { return textures_.size(); }

private:
    std::string download(const std::string& url);  // local path or ""
    std::string cacheDir_;
    std::unordered_map<std::string, std::shared_ptr<sf::Texture>> textures_;
    std::unordered_map<std::string, std::shared_ptr<sf::Image>> images_;
};

}  // namespace pm
