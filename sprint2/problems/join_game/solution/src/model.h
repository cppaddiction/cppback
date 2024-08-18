#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <sstream>
#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    /*–азве это конструктор без параметров? Rectangle bounds же передаЄтс€.
    Ќо в любом случае код в этом файле - код игровой модели, вз€тый целиком из учебника, так что не думаю, что это критично.*/
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    //std::vector<GameSession> active_sessions_;
};

class Dog {
public:
    Dog() = default;
    Dog(std::string name);
    Dog(std::string name, std::uint64_t id) : name_(name), id_(id) {}
    bool operator==(const Dog& other) const { return name_ == other.name_ && id_ == other.id_; }
    const std::string& GetName() const { return name_; }
    std::uint64_t GetId() const { return id_; }
private:
    std::string name_="";
    std::uint64_t id_=0;
};


class GameSession {
public:
    GameSession(const Map* map, std::uint64_t id) : map_(map), id_(id) {}
    void AddDog(Dog d) { dogs_.push_back(d); }
    const Dog& FindDog(std::string name, std::uint64_t id) const {
        Dog temp(name, id);
        if (auto search = std::find(dogs_.begin(), dogs_.end(), temp); search != dogs_.end())
        {
            return dogs_[static_cast<int>(std::distance(dogs_.begin(), search))];
        }
        else
        {
            throw std::logic_error("No such dog");
        }
    }
    const Map& GetMap() const { return *map_; }
    std::uint64_t GetId() const { return id_; }
    const std::vector<Dog>& GetDogs() const { return dogs_; }
    std::uint64_t GetPlayersAmount() const { return dogs_.size(); }
private:
    const Map* map_;
    std::uint64_t id_;
    std::vector<Dog> dogs_;
};

class SessionManager {
public:
    SessionManager(const std::vector<Map>& maps)
    {
        for (const auto& map : maps)
        {
            active_sessions_.emplace_back(std::make_shared<GameSession>(&map, 0));
        }
    }
    std::shared_ptr<GameSession> FindSession(const Map* m, uint64_t session_id) const
    {
        for (auto session : active_sessions_)
        {
            if (m->GetId() == (session->GetMap()).GetId() && session_id == session->GetId())
            {
                return session;
            }
        }
    }
private:
    std::vector<std::shared_ptr<GameSession>> active_sessions_;
};

}  // namespace model
