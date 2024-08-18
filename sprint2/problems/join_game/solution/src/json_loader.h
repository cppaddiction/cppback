#pragma once

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/json.hpp>
#include "model.h"

namespace json_loader {
constexpr static const char* ID = "id";
constexpr static const char* NAME = "name";
constexpr static const char* ROADS = "roads";
constexpr static const char* BUILDINGS = "buildings";
constexpr static const char* OFFICES = "offices";

constexpr static const char* X = "x";
constexpr static const char* Y = "y";
constexpr static const char* X0 = "x0";
constexpr static const char* Y0 = "y0";
constexpr static const char* X1 = "x1";
constexpr static const char* Y1 = "y1";
constexpr static const char* W = "w";
constexpr static const char* H = "h";
constexpr static const char* OffsetX = "offsetX";
constexpr static const char* OffsetY = "offsetY";

constexpr static const char* MAPS = "maps";
constexpr static const char* INVALID_FILE_PATH = "Invalid file path";

void LoadMaps(const boost::json::array& maps, model::Game& game);
void LoadRoads(const boost::json::array& roads, model::Map& map);
void LoadBuildings(const boost::json::array& buildings, model::Map& map);
void LoadOffices(const boost::json::array& maps, model::Map& map);

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
