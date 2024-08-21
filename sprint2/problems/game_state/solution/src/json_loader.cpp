#include "json_loader.h"

namespace json = boost::json;
using namespace std::literals;

namespace json_loader {

void LoadMaps(const json::array& maps, model::Game& game)
{
    for (const auto& m : maps)
    {
        model::Map map(model::Map::Id(static_cast<std::string>(m.as_object().at(ID).as_string())), static_cast<std::string>(m.as_object().at(NAME).as_string()));
        ::boost::json::value const* const p_value{ m.as_object().if_contains(MAP_DOG_SPEED) };
        if (p_value)
        {
            map.AddSpecificMapDogSpeed(m.as_object().at(MAP_DOG_SPEED).as_uint64());
        }
        LoadRoads(m.as_object().at(ROADS).as_array(), map);
        LoadBuildings(m.as_object().at(BUILDINGS).as_array(), map);
        LoadOffices(m.as_object().at(OFFICES).as_array(), map);
        game.AddMap(map);
    }
}
void LoadRoads(const json::array& roads, model::Map& map)
{
    for (const auto& r : roads)
    {
        ::boost::json::value const* const p_value{ r.as_object().if_contains(X1) };
        if (p_value)
        {
            map.AddRoad(model::Road{ model::Road::HORIZONTAL, model::Point{static_cast<int>(r.as_object().at(X0).as_int64()), static_cast<int>(r.as_object().at(Y0).as_int64())},  static_cast<int>(r.as_object().at(X1).as_int64()) });
        }
        else
        {
            map.AddRoad(model::Road{ model::Road::VERTICAL, model::Point{static_cast<int>(r.as_object().at(X0).as_int64()), static_cast<int>(r.as_object().at(Y0).as_int64())},  static_cast<int>(r.as_object().at(Y1).as_int64()) });
        }
    }
}
void LoadBuildings(const json::array& buildings, model::Map& map)
{
    for (const auto& b : buildings)
    {
        map.AddBuilding(model::Building{ model::Rectangle{model::Point{static_cast<int>(b.as_object().at(X).as_int64()), static_cast<int>(b.as_object().at(Y).as_int64())}, model::Size{static_cast<int>(b.as_object().at(W).as_int64()), static_cast<int>(b.as_object().at(H).as_int64())}} });
    }
}
void LoadOffices(const json::array& offices, model::Map& map)
{
    for (const auto& o : offices)
    {
        map.AddOffice(model::Office{ model::Office::Id(static_cast<std::string>(o.as_object().at(ID).as_string())), model::Point{static_cast<int>(o.as_object().at(X).as_int64()), static_cast<int>(o.as_object().at(Y).as_int64())}, model::Offset{static_cast<int>(o.as_object().at(OffsetX).as_int64()), static_cast<int>(o.as_object().at(OffsetY).as_int64())} });
    }
}
model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла

    if (std::ifstream in{ json_path })
    {
        std::string file, line;

        while (getline(in, line))
        {
            file += line;
        }

        auto value = json::parse(file);
        auto maps=value.as_object().at(MAPS).as_array();

        model::Game game;

        ::boost::json::value const* const p_value{ value.as_object().if_contains(DEFAULT_DOG_SPEED) };
        if (p_value)
        {
            game.AddDefaultDogSpeed(value.as_object().at(DEFAULT_DOG_SPEED).as_uint64());
        }

        LoadMaps(maps, game);
        return game;
    }
    else
    {
        throw std::logic_error(INVALID_FILE_PATH);
    }
}

}  // namespace json_loader
