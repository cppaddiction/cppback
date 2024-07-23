#include "json_loader.h"
#include <boost/json.hpp>

namespace json = boost::json;
using namespace std::literals;

namespace json_loader {

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
        auto maps=value.as_object().at("maps").as_array();

        model::Game game;

        for (const auto& m : maps)
        {
            model::Map mm(model::Map::Id(static_cast<std::string>(m.as_object().at("id").as_string())), static_cast<std::string>(m.as_object().at("name").as_string()));

            auto roads = m.as_object().at("roads").as_array();
            for (const auto& r : roads)
            {
                ::boost::json::value const* const p_value{ r.as_object().if_contains("x1")};
                if (!p_value)
                {
                    mm.AddRoad(model::Road{model::Road::HORIZONTAL, model::Point{static_cast<int>(r.as_object().at("x0").as_int64()), static_cast<int>(r.as_object().at("y0").as_int64())},  static_cast<int>(r.as_object().at("x1").as_int64()) });
                }
                else
                {
                    mm.AddRoad(model::Road{ model::Road::HORIZONTAL, model::Point{static_cast<int>(r.as_object().at("x0").as_int64()), static_cast<int>(r.as_object().at("y0").as_int64())},  static_cast<int>(r.as_object().at("y1").as_int64()) });
                }
            }
            auto buildings = m.as_object().at("buildings").as_array();
            for (const auto& b : buildings)
            {
                mm.AddBuilding(model::Building{ model::Rectangle{model::Point{static_cast<int>(b.as_object().at("x").as_int64()), static_cast<int>(b.as_object().at("y").as_int64())}, model::Size{static_cast<int>(b.as_object().at("w").as_int64()), static_cast<int>(b.as_object().at("h").as_int64())}}});
            }
            auto offices = m.as_object().at("offices").as_array();
            for (const auto& o : offices)
            {
                mm.AddOffice(model::Office{ model::Office::Id(static_cast<std::string>(o.as_object().at("id").as_string())), model::Point{static_cast<int>(o.as_object().at("x").as_int64()), static_cast<int>(o.as_object().at("y").as_int64())}, model::Offset{static_cast<int>(o.as_object().at("offsetX").as_int64()), static_cast<int>(o.as_object().at("offsetY").as_int64())} });
            }

            game.AddMap(mm);
        }

        return game;
    }
    else
    {
        throw std::logic_error("Invalid file path");
    }
}

}  // namespace json_loader
