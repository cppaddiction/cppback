#pragma once

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include "model.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
