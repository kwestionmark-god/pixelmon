#include "ui/SpriteCache.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace pm {

// Simple, stable string hash -> deterministic cache filename.
static std::string urlHash(const std::string& url) {
    std::hash<std::string> h;
    return std::to_string(h(url) & 0x7FFFFFFF);
}

static bool fileExists(const fs::path& p) {
    std::error_code ec;
    return fs::exists(p, ec) && !ec;
}

SpriteCache::SpriteCache(const std::string& cacheDir) : cacheDir_(cacheDir) {
    std::error_code ec;
    fs::create_directories(cacheDir, ec);
}

std::string SpriteCache::download(const std::string& url) {
    const fs::path dir(cacheDir_);
    const fs::path file = dir / (urlHash(url) + ".png");

    if (fileExists(file)) return file.string();

    std::ostringstream cmd;
    cmd << "curl -fsSL \"" << url << "\" -o \"" << file << "\"";
    std::system(cmd.str().c_str());

    return fileExists(file) ? file.string() : "";
}

sf::Texture* SpriteCache::get(const std::string& url) {
    auto it = textures_.find(url);
    if (it != textures_.end()) return it->second.get();

    const std::string local = download(url);
    if (local.empty()) return nullptr;

    auto tex = std::make_shared<sf::Texture>();
    if (!tex->loadFromFile(local)) return nullptr;

    textures_.emplace(url, tex);
    return tex.get();
}

sf::Image* SpriteCache::image(const std::string& url) {
    auto it = images_.find(url);
    if (it != images_.end()) return it->second.get();

    const std::string local = download(url);
    if (local.empty()) return nullptr;

    auto img = std::make_shared<sf::Image>();
    if (!img->loadFromFile(local)) return nullptr;

    images_.emplace(url, img);
    return img.get();
}

bool SpriteCache::isLoaded(const std::string& url) const {
    return textures_.find(url) != textures_.end();
}

}  // namespace pm
