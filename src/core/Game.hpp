#pragma once
#include <string>
#include <memory>
#include <unordered_map>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>

#include "data/DataRepository.hpp"
#include "ui/SpriteCache.hpp"
#include "core/World.hpp"
#include "core/BattleSystem.hpp"

namespace pm {

// Game state machine.
enum class GameState {
    Overworld,
    Battle,
    Menu,
    Paused,
};

class Game {
public:
    Game(const std::string& dataDir, const std::string& spriteCacheDir,
         const std::string& startMap = "pallet_town");

    // Load data (with src/data fallback).
    bool loadData();

    // Full windowed run: overworld + battle integration.
    bool run();

    // Headless check: load data + sprite + texture, print counts, no window.
    bool headlessSmoke(const Monster& mon);

    // Render the monster to an offscreen texture and save a PNG.
    bool renderToFile(const Monster& mon, const std::string& outputPath);

    const DataRepository& data() const { return data_; }
    const World& world() const { return world_; }
    GameState state() const { return state_; }

private:
    std::string dataDir_;
    std::string spriteCacheDir_;
    std::string startMap_;
    DataRepository data_;
    SpriteCache* cache_ = nullptr;

    // Overworld.
    World world_;
    sf::Texture tilesetTexture_;  // procedural fallback tileset
    std::unordered_map<std::string, sf::Texture> atlasTextures_;  // per-map-tileset atlases
    std::string tilesetBase_;  // resolved atlas dir ("<dataDir>/maps/tilesets" or src/data fallback)
    GameState state_ = GameState::Overworld;

    // Battle.
    BattleContext battleCtx_;
    bool battleInitialized_ = false;

    // UI.
    sf::Font font_;
    bool fontOk_ = false;

    // Render window (created in run()).
    std::unique_ptr<sf::RenderWindow> window_;

    // Create procedural tileset (16x16 tiles).
    bool createTileset();

    // Atlas texture for the current map's tileset, or the procedural fallback.
    const sf::Texture& currentTilesetTexture();

    // Overworld loop.
    bool runOverworld();

    // Battle loop.
    bool runBattle();

    // Handle input in overworld.
    void handleOverworldInput(const sf::Event& event);

    // Handle input in battle.
    void handleBattleInput(const sf::Event& event);

    // Render overworld.
    void renderOverworld();

    // Render battle.
    void renderBattle();

    // Transition to battle.
    void startBattle(bool isWild, int speciesNum, int trainerIndex = -1);

    // Return from battle to overworld.
    void endBattle();

    // Update overworld (movement, camera, encounters).
    void updateOverworld(float dt);

    // Update battle.
    void updateBattle(float dt);
};

}  // namespace pm
