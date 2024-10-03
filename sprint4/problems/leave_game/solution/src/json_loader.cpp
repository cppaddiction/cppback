#include "json_loader.h"
#include "loottypes.h"

namespace json = boost::json;
using namespace std::literals;

namespace json_loader {

    std::uint64_t road_ids = 0;

void LoadMaps(const json::array& maps, model::Game& game)
{
    for (const auto& m : maps)
    {
        model::Map map(model::Map::Id(static_cast<std::string>(m.as_object().at(ID).as_string())), static_cast<std::string>(m.as_object().at(NAME).as_string()));
        ::boost::json::value const* const p_value{ m.as_object().if_contains(MAP_DOG_SPEED) };
        if (p_value)
        {
            map.AddSpecificMapDogSpeed(m.as_object().at(MAP_DOG_SPEED).as_double());
        }
        else
        {
            map.AddSpecificMapDogSpeed(game.GetDefaultDogSpeed());
        }
        ::boost::json::value const* const p_value2{ m.as_object().if_contains(MAP_BAG_CAPACITY) };
        if (p_value2)
        {
            map.AddSpecificBagCapacity(m.as_object().at(MAP_BAG_CAPACITY).as_int64());
        }
        else
        {
            map.AddSpecificBagCapacity(game.GetDefaultBagCapacity());
        }
        LoadRoads(m.as_object().at(ROADS).as_array(), map);
        LoadBuildings(m.as_object().at(BUILDINGS).as_array(), map);
        LoadOffices(m.as_object().at(OFFICES).as_array(), map);
        game.AddMap(map);
        
        auto loot_info = m.as_object().at(LOOT_TYPES).as_array();
        json::array arr;
        
        for (const auto& item : loot_info)
        {
            boost::json::object obj;
            ::boost::json::value const* const p_value_name{ item.as_object().if_contains(NAME) };
            if (p_value_name)
            {
                obj.emplace(NAME, static_cast<std::string>(item.as_object().at(NAME).as_string()));
            }
            ::boost::json::value const* const p_value_file{ item.as_object().if_contains(FILE) };
            if (p_value_file)
            {
                obj.emplace(FILE, static_cast<std::string>(item.as_object().at(FILE).as_string()));
            }
            ::boost::json::value const* const p_value_type{ item.as_object().if_contains(TYPE) };
            if (p_value_type)
            {
                obj.emplace(TYPE, static_cast<std::string>(item.as_object().at(TYPE).as_string()));
            }
            ::boost::json::value const* const p_value_rot{ item.as_object().if_contains(ROTATION) };
            if (p_value_rot)
            {
                obj.emplace(ROTATION, item.as_object().at(ROTATION).as_int64());
            }
            ::boost::json::value const* const p_value_col{ item.as_object().if_contains(COLOR) };
            if (p_value_col)
            {
                obj.emplace(COLOR, static_cast<std::string>(item.as_object().at(COLOR).as_string()));
            }
            ::boost::json::value const* const p_value_scale{ item.as_object().if_contains(SCALE) };
            if (p_value_scale)
            {
                obj.emplace(SCALE, item.as_object().at(SCALE).as_double());
            }
            ::boost::json::value const* const p_value_value{ item.as_object().if_contains(VALUE) };
            if (p_value_scale)
            {
                obj.emplace(VALUE, item.as_object().at(VALUE).as_int64());
            }
            arr.emplace_back(obj);
        }

        loot_types::to_frontend_loot_type_data[static_cast<std::string>(m.as_object().at(ID).as_string())] = arr;
    }
}
void LoadRoads(const json::array& roads, model::Map& map)
{
    for (const auto& r : roads)
    {
        ::boost::json::value const* const p_value{ r.as_object().if_contains(X1) };
        if (p_value)
        {
            map.AddRoad(model::Road{ model::Road::HORIZONTAL, 
                model::Point{static_cast<int>(r.as_object().at(X0).as_int64()),
                static_cast<int>(r.as_object().at(Y0).as_int64())},  
                static_cast<int>(r.as_object().at(X1).as_int64()), 
                *map.GetId(), model::Road::Id{road_ids++} });
        }
        else
        {
            map.AddRoad(model::Road{ model::Road::VERTICAL,
                model::Point{static_cast<int>(r.as_object().at(X0).as_int64()),
                static_cast<int>(r.as_object().at(Y0).as_int64())},
                static_cast<int>(r.as_object().at(Y1).as_int64()),
                *map.GetId(), model::Road::Id{road_ids++} });
        }
    }
    map.CreateRoadGrid();
}
void LoadBuildings(const json::array& buildings, model::Map& map)
{
    for (const auto& b : buildings)
    {
        map.AddBuilding(model::Building{ model::Rectangle{
            model::Point{static_cast<int>(b.as_object().at(X).as_int64()),
            static_cast<int>(b.as_object().at(Y).as_int64())},
            model::Size{static_cast<int>(b.as_object().at(W).as_int64()),
            static_cast<int>(b.as_object().at(H).as_int64())}} });
    }
}
void LoadOffices(const json::array& offices, model::Map& map)
{
    for (const auto& o : offices)
    {
        map.AddOffice(model::Office{ 
            model::Office::Id(static_cast<std::string>(o.as_object().at(ID).as_string())),
            model::Point{static_cast<int>(o.as_object().at(X).as_int64()),
            static_cast<int>(o.as_object().at(Y).as_int64())},
            model::Offset{static_cast<int>(o.as_object().at(OffsetX).as_int64()),
            static_cast<int>(o.as_object().at(OffsetY).as_int64())} });
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
            game.AddDefaultDogSpeed(value.as_object().at(DEFAULT_DOG_SPEED).as_double());
        }

        ::boost::json::value const* const p_value2{ value.as_object().if_contains(DEFAULT_BAG_CAPACITY) };
        if (p_value2)
        {
            game.AddDefaultBagCapacity(value.as_object().at(DEFAULT_BAG_CAPACITY).as_int64());
        }

        ::boost::json::value const* const p_value3{ value.as_object().if_contains(DEFAULT_RETIREMENT_TIME) };
        if (p_value3)
        {
            game.AddRetTime(value.as_object().at(DEFAULT_RETIREMENT_TIME).as_double());
        }

        LoadMaps(maps, game);
        in.close();
        return game;
    }
    else
    {
        throw std::logic_error(INVALID_FILE_PATH);
    }
}

loot_gen::LootGenerator LoadLootGenerator(const std::filesystem::path& json_path)
{
    if (std::ifstream in{ json_path })
    {
        std::string file, line;

        while (getline(in, line))
        {
            file += line;
        }

        auto value = json::parse(file);
        auto lg_config = value.as_object().at(LOOTGENCONFIG).as_object();
        int period = lg_config.at(PERIOD).as_double() * 1000;
        double probability = lg_config.at(PROBABILITY).as_double();
        loot_gen::LootGenerator lg(std::chrono::milliseconds(period), probability);
        in.close();
        return lg;
    }
    else
    {
        throw std::logic_error(INVALID_FILE_PATH);
    }
}

}  // namespace json_loader
