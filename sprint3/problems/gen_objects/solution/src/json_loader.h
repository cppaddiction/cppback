#pragma once

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/json.hpp>
#include <chrono>
#include "model.h"

namespace json_loader {
const std::string ID = "id";
const std::string NAME = "name";
const std::string ROADS = "roads";
const std::string BUILDINGS = "buildings";
const std::string OFFICES = "offices";

const std::string X = "x";
const std::string Y = "y";
const std::string X0 = "x0";
const std::string Y0 = "y0";
const std::string X1 = "x1";
const std::string Y1 = "y1";
const std::string W = "w";
const std::string H = "h";
const std::string OffsetX = "offsetX";
const std::string OffsetY = "offsetY";

const std::string MAPS = "maps";
const std::string INVALID_FILE_PATH = "Invalid file path";

const std::string DEFAULT_DOG_SPEED = "defaultDogSpeed";
const std::string MAP_DOG_SPEED = "dogSpeed";

const std::string LOOTGENCONFIG = "lootGeneratorConfig";
const std::string PERIOD = "period";
const std::string PROBABILITY = "probability";

const std::string LOOT_TYPES = "lootTypes";
const std::string FILE = "file";
const std::string TYPE = "type";
const std::string ROTATION = "rotation";
const std::string COLOR = "color";
const std::string SCALE = "scale";

void LoadMaps(const boost::json::array& maps, model::Game& game);
void LoadRoads(const boost::json::array& roads, model::Map& map);
void LoadBuildings(const boost::json::array& buildings, model::Map& map);
void LoadOffices(const boost::json::array& maps, model::Map& map);

model::Game LoadGame(const std::filesystem::path& json_path);

loot_gen::LootGenerator LoadLootGenerator(const std::filesystem::path& json_path);

}  // namespace json_loader
