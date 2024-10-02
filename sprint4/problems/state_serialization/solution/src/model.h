#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <sstream>
#include <random>
#include <cmath>
#include <optional>
#include <iostream>
#include "tagged.h"
#include "loot_generator.h"
#include "collision_detector.h"

namespace model {

struct Position {
    double x = 0.0;
    double y = 0.0;
};

struct Speed {
    double vx = 0.0;
    double vy = 0.0;
};

using Dimension = int;
using Coord = Dimension;

struct Point {
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    Coord x, y;
};

struct PointHasher {
    size_t operator()(const Point& p) const {
        return std::hash<int>{}(p.x) + std::hash<int>{}(p.y);
    }
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
    using Id = util::Tagged<std::uint64_t, Road>;
    Road(HorizontalTag, Point start, Coord end_x, std::string map, Id id) noexcept;
    Road(VerticalTag, Point start, Coord end_y, std::string map, Id id) noexcept;
    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;
    bool operator==(const Road& other) const;
    std::string GetMap() const { return map_; }
    Id GetId() const { return id_; }

private:
    Point start_;
    Point end_;
    std::string map_;
    Id id_;
};

struct RoadHasher {
    size_t operator()(const Road& r) const {
        auto start = r.GetStart();
        auto end = r.GetEnd();
        return std::hash<int>{}(start.x) + std::hash<int>{}(start.y) + std::hash<int>{}(end.x) + std::hash<int>{}(end.y);
    }
};

std::optional<Point> IsIntersect(const Road& r1, const Road& r2);

class Building {
public:
    explicit Building(Rectangle bounds) noexcept;
    const Rectangle& GetBounds() const noexcept;

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept;
    const Id& GetId() const noexcept;
    Point GetPosition() const noexcept;
    Offset GetOffset() const noexcept;

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

    Map(Id id, std::string name) noexcept;
    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);
    void AddSpecificMapDogSpeed(double speed);
    void AddSpecificBagCapacity(std::uint64_t bag_capacity);
    double GetSpecificMapDogSpeed() const;
    std::uint64_t GetSpecificBagCapacity() const;
    std::pair<const Road*, Position> GetRandomPosition(bool randomize_spawn_points) const;
    void CreateRoadGrid();
    const std::vector<Point>& GetRoadCrossses(const Road& road) const;
    const std::pair<Road, Road>& GetNeighbourRoad(const Point& cross) const;
    const Road* FindRoad(model::Road::Id id) const {
        for (const auto& road : roads_)
        {
            if (road.GetId() == id)
            {
                return &road;
            }
        }
        return nullptr;
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    std::unordered_map<Point, std::pair<Road, Road>, PointHasher> grid_;
    std::unordered_map<Road, std::vector<Point>, RoadHasher> roads_to_crosses_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    double specific_map_dog_speed_=0.0;
    std::uint64_t bag_capacity_ = 0;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);
    void AddDefaultDogSpeed(double speed);
    void AddDefaultBagCapacity(std::uint64_t def_bag_capacity);
    double GetDefaultDogSpeed() const;
    std::uint64_t GetDefaultBagCapacity() const;
    const Maps& GetMaps() const noexcept;
    const Map* FindMap(const Map::Id& id) const noexcept;

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    double default_dog_speed_=1.0;
    std::uint64_t def_bag_capacity_ = 3;
};

struct LostObject {
public:
    LostObject() = default;
    LostObject(double x, double y, std::uint64_t id, int type, int score_per_obj);
    double GetX() const;
    double GetY() const;
    std::uint64_t GetId() const;
    int GetType() const;
    int GetScorePerObj() const;

/*private:*/
    double x_ = 0.0;
    double y_ = 0.0;
    std::uint64_t id_ = 0;
    int type_ = 0;
    int score_per_obj_ = 0;
};

class Dog {
public:
    Dog() = default;
    Dog(std::string name, Position pos, const Road* r);
    Dog(std::string name, std::uint64_t id);
    bool operator==(const Dog& other) const;
    const std::string& GetName() const;
    std::uint64_t GetId() const;
    std::uint64_t GetScore() const;
    Position GetPos() const;
    Speed GetSpd() const;
    const std::string& GetDir() const;
    void SetDir(std::string dir);
    void SetSpeed(double vxx, double vyy);
    void Move(Position pos);
    const Road& GetRoad() const;
    void SetRoad(const Road* r);
    void AddLoot(const LostObject& obj);
    const std::vector<LostObject>& GetBag() const;
    void DropLoot();
    bool CanLoot(std::uint64_t max);
    void SetScore(std::uint64_t score) { score_ = score; }

private:
    std::string name_ = "";
    std::uint64_t id_ = 0;
    Position pos_;
    const Road* r_ = nullptr;
    Speed spd_;
    std::string dir_ = "U";
    std::vector<LostObject> bag_;
    std::uint64_t score_ = 0;
};

class TestItemGathererProvider : public collision_detector::ItemGathererProvider {
public:
    TestItemGathererProvider();
    std::vector<int> Initialize(const std::unordered_map<std::uint64_t, model::LostObject>& items, const std::vector<std::pair<Position, Position>>& gatherers, const std::vector<model::Office>& offices);
    void Clear();
    size_t ItemsCount() const override;
    collision_detector::Item GetItem(size_t idx) const override;
    size_t GatherersCount() const override;
    collision_detector::Gatherer GetGatherer(size_t idx) const override;
private:
    std::vector< collision_detector::Item> items_;
    std::vector< collision_detector::Gatherer> gatherers_;
};

class GameSession {

    const std::string VALUE = "value";

public:
    GameSession(const Map* map, std::uint64_t id);
    void AddDog(Dog d);
    Dog& FindDog(std::string name, std::uint64_t id);
    void UpdateSession(std::uint64_t time, loot_gen::LootGenerator lg);
    const Map& GetMap() const;
    std::uint64_t GetId() const;
    const std::vector<Dog>& GetDogs() const;
    const std::unordered_map<std::uint64_t, LostObject>& GetLostObjects() const;
    void AddLostObject(std::uint64_t id, LostObject obj);
    std::uint64_t GetPlayersAmount() const;
    int GetLootCount() const;
    void ProcessHorizontalRoad(model::Dog& d, const model::Road current_road, model::Position& pos, const model::Speed spd, const std::string& dir, double time_sec, const model::Map& m);
    void ProcessVerticalRoad(model::Dog& d, const model::Road current_road, model::Position& pos, const model::Speed spd, const std::string& dir, double time_sec, const model::Map& m);
    void SetLootCount(int loot_count) { loot_count_ = loot_count; }
private:
    const Map* map_;
    std::uint64_t id_;
    std::vector<Dog> dogs_;
    std::unordered_map<std::uint64_t, LostObject> lost_objects_;
    int loot_count_ = 0;
    TestItemGathererProvider prov{};
};

class SessionManager {
public:
    SessionManager(const std::vector<Map>& maps);
    std::shared_ptr<GameSession> FindSession(const Map* m, uint64_t session_id) const;
    void UpdateAllSessions(std::uint64_t time, loot_gen::LootGenerator lg) const;
    std::vector<std::shared_ptr<GameSession>> GetAllSessions() const { return active_sessions_; }
    void ClearSessions() { active_sessions_.clear(); }
    void AddSession(std::shared_ptr<GameSession> session) { active_sessions_.push_back(session); }
private:
    std::vector<std::shared_ptr<GameSession>> active_sessions_;
};

}  // namespace model
