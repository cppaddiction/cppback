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

    bool operator==(const Road& other) const {
        return GetStart() == other.GetStart() && GetEnd() == other.GetEnd();
    }

private:
    Point start_;
    Point end_;
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

    void AddSpecificMapDogSpeed(double speed) {
        specific_map_dog_speed_ = speed;
    }

    void AddSpecificBagCapacity(std::uint64_t bag_capacity) {
        bag_capacity_ = bag_capacity;
    }

    double GetSpecificMapDogSpeed() const {
        return specific_map_dog_speed_;
    }

    std::uint64_t GetSpecificBagCapacity() const {
        return bag_capacity_;
    }

    std::pair<const Road*, Position> GetRandomPosition(bool randomize_spawn_points) const;

    void CreateRoadGrid();

    const std::vector<Point>& GetRoadCrossses(const Road& road) const {
        return roads_to_crosses_.at(road);
    }

    const std::pair<Road, Road>& GetNeighbourRoad(const Point& cross) const {
        return grid_.at(cross);
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

    void AddDefaultDogSpeed(double speed) {
        default_dog_speed_ = speed;
    }

    void AddDefaultBagCapacity(std::uint64_t def_bag_capacity) {
        def_bag_capacity_ = def_bag_capacity;
    }

    double GetDefaultDogSpeed() const {
        return default_dog_speed_;
    }

    std::uint64_t GetDefaultBagCapacity() const {
        return def_bag_capacity_;
    }

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
    double default_dog_speed_=1.0;
    std::uint64_t def_bag_capacity_ = 3;
};

class LostObject {
public:
    LostObject() = default;
    LostObject(double x, double y, std::uint64_t id, int type) : x_(x), y_(y), id_(id), type_(type) {}
    double GetX() const { return x_; }
    double GetY() const { return y_; }
    std::uint64_t GetId() const { return id_; }
    int GetType() const { return type_; }
private:
    double x_ = 0.0;
    double y_ = 0.0;
    std::uint64_t id_ = 0;
    int type_ = 0;
};

class Dog {
public:
    Dog() = default;
    Dog(std::string name, Position pos, const Road* r);
    Dog(std::string name, std::uint64_t id) : name_(name), id_(id) {}
    bool operator==(const Dog& other) const { return name_ == other.name_ && id_ == other.id_; }
    const std::string& GetName() const { return name_; }
    std::uint64_t GetId() const { return id_; }
    Position GetPos() const { return pos_; }
    Speed GetSpd() const { return spd_; }
    const std::string& GetDir() const { return dir_; }
    void SetDir(std::string dir) { dir_ = dir; }
    void SetSpeed(double vxx, double vyy) { spd_.vx = vxx; spd_.vy = vyy; }
    void Move(Position pos) { pos_ = pos; }
    const Road& GetRoad() const { return *r_; }
    void SetRoad(const Road* r) { r_ = r; }
    void AddLoot(const LostObject& obj) { bag_.push_back(obj); }
    const std::vector<LostObject>& GetBag() const { return bag_; }
    void DropLoot() { bag_.clear(); }
    bool CanLoot(std::uint64_t max) { return bag_.size() < max; }
private:
    std::string name_ = "";
    std::uint64_t id_ = 0;
    Position pos_;
    const Road* r_ = nullptr;
    Speed spd_;
    std::string dir_ = "U";
    std::vector<LostObject> bag_;
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
private:
    std::vector<std::shared_ptr<GameSession>> active_sessions_;
};

}  // namespace model
