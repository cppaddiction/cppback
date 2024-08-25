#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <sstream>
#include <random>
#include <cmath>
#include <optional>
#include "tagged.h"

namespace model {

struct Position {
    double x = 0.0;
    double y = 0.0;
};

struct Speed {
    double vx = 0;
    double vy = 0;
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

class Building {
public:
    /*                                    ? Rectangle bounds             .
                                       -                   ,                           ,                 ,                 .*/
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

    double GetSpecificMapDogSpeed() const {
        return specific_map_dog_speed_;
    }

    std::pair<const Road*, Position> GetRandomPosition() const {
        const auto& roads = GetRoads();
        std::random_device rd;
        std::uniform_int_distribution<int> dist_int(0, roads.size() - 1);
        const auto& road = roads[dist_int(rd)];
        auto road_start = road.GetStart(); auto road_end = road.GetEnd();
        int start, end; start = road.IsHorizontal() ? road_start.x : road_start.y; end = road.IsHorizontal() ? road_end.x : road_end.y;
        std::uniform_real_distribution<> dist_double(start, end);
        auto value = std::round(dist_double(rd) * 10) / 10;
	auto temp = roads[0].GetStart();
        return std::make_pair(&roads[0], Position{temp.x, temp.y});
        //return std::make_pair(&road, road.IsHorizontal() ? Position{value, std::round(static_cast<double>(road_start.y) * 10) / 10} : Position{std::round(static_cast<double>(road_start.x) * 10) / 10, value});
    }

    void CreateRoadGrid() {
        for (int i = 0; i < roads_.size(); i++)
        {
            for (int j = 0; j < roads_.size(); j++)
            {
                if (i != j)
                {
                    if (auto p = IsIntersect(roads_[i], roads_[j]))
                    {
                        grid_.try_emplace(*p, roads_[i], roads_[j]);
                        auto& vi = roads_to_crosses_[roads_[i]];
                        auto& vj = roads_to_crosses_[roads_[j]];
                        vi.push_back(*p);
                        vj.push_back(*p);
                    }
                }
            }
        }
    }

    std::optional<Point> IsIntersect(const Road& r1, const Road& r2) const {
        if (r1.IsHorizontal() && r2.IsVertical())
        {
            auto r1_st = r1.GetStart();
            auto r1_fn = r1.GetEnd();
            auto r2_st = r2.GetStart();
            auto r2_fn = r2.GetEnd();
            auto lesser_r1_x = r1_st.x < r1_fn.x ? r1_st.x : r1_fn.x;
            auto greater_r1_x = r1_st.x > r1_fn.x ? r1_st.x : r1_fn.x;
            auto lesser_r2_y = r2_st.y < r2_fn.y ? r2_st.y : r2_fn.y;
            auto greater_r2_y = r2_st.y > r2_fn.y ? r2_st.y : r2_fn.y;
            if (lesser_r1_x <= r2_st.x && r2_st.x <= greater_r1_x && lesser_r2_y <= r1_st.y && r1_st.y <= greater_r2_y)
            {
                return Point{ r2_st.x, r1_st.y };
            }
        }
        if (r1.IsVertical() && r2.IsHorizontal())
        {
            auto r1_st = r2.GetStart();
            auto r1_fn = r2.GetEnd();
            auto r2_st = r1.GetStart();
            auto r2_fn = r1.GetEnd();
            auto lesser_r1_x = r1_st.x < r1_fn.x ? r1_st.x : r1_fn.x;
            auto greater_r1_x = r1_st.x > r1_fn.x ? r1_st.x : r1_fn.x;
            auto lesser_r2_y = r2_st.y < r2_fn.y ? r2_st.y : r2_fn.y;
            auto greater_r2_y = r2_st.y > r2_fn.y ? r2_st.y : r2_fn.y;
            if (lesser_r1_x <= r2_st.x && r2_st.x <= greater_r1_x && lesser_r2_y <= r1_st.y && r1_st.y <= greater_r2_y)
            {
                return Point{ r2_st.x, r1_st.y };
            }
        }
        return std::nullopt;
    }

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
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    void AddDefaultDogSpeed(double speed) {
        default_dog_speed_ = speed;
    }

    double GetDefaultDogSpeed() const {
        return default_dog_speed_;
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
    double default_dog_speed_=0.0;
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
private:
    std::string name_="";
    std::uint64_t id_=0;
    Position pos_;
    const Road* r_=nullptr;
    Speed spd_;
    std::string dir_ = "U";
};


class GameSession {
public:
    GameSession(const Map* map, std::uint64_t id) : map_(map), id_(id) {}
    void AddDog(Dog d) { dogs_.push_back(d); }
    Dog& FindDog(std::string name, std::uint64_t id) {
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
    void UpdateSession(std::uint64_t time) { 
        //movement implementation

        const auto& m = GetMap();
        for (auto& d : dogs_)
        {
            auto pos = d.GetPos();
            auto spd = d.GetSpd();
            auto dir = d.GetDir();
            const auto& current_road = d.GetRoad();

            if (current_road.IsHorizontal())
            {
                auto r1_st = current_road.GetStart();
                auto r1_fn = current_road.GetEnd();
                double lesser_r1_x = (r1_st.x < r1_fn.x ? r1_st.x : r1_fn.x) - 0.4;
                double greater_r1_x = (r1_st.x > r1_fn.x ? r1_st.x : r1_fn.x) + 0.4;
                if (dir == "L")
                {
                    if (pos.x + spd.vx * time <= lesser_r1_x)
                    {
                        pos.x = lesser_r1_x;
                    }
                    else
                    {
                        pos.x += (spd.vx * time);
                    }
                }
                else if (dir == "R")
                {
                    if (pos.x + spd.vx * time >= greater_r1_x)
                    {
                        pos.x = greater_r1_x;
                    }
                    else
                    {
                        pos.x += (spd.vx * time);
                    }
                }
                else if (dir == "U" || dir == "D")
                {
                    bool in_cross = false;
                    const auto& road_crosses = m.GetRoadCrossses(current_road);
                    for (const auto& cross : road_crosses)
                    {
                        double lb_x = cross.x - 0.4;
                        double rb_x = cross.x + 0.4;
                        double lb_y = cross.y - 0.4;
                        double rb_y = cross.y + 0.4;
                        if (lb_x <= pos.x && pos.x <= rb_x && lb_y <= pos.y && pos.y <= rb_y)
                        {
                            const auto& crossed_roads = m.GetNeighbourRoad(cross);
                            const Road* r1 = &crossed_roads.first;
                            const Road* r2 = &crossed_roads.second;
                            if (current_road == crossed_roads.first)
                            {
                                d.SetRoad(r2);
                            }
                            else
                            {
                                d.SetRoad(r1);
                            }

                            const auto& crossed_road = d.GetRoad();

                            auto r2_st = crossed_road.GetStart();
                            auto r2_fn = crossed_road.GetEnd();
                            double lesser_r2_y = (r2_st.y < r2_fn.y ? r2_st.y : r2_fn.y) - 0.4;
                            double greater_r2_y = (r2_st.y > r2_fn.y ? r2_st.y : r2_fn.y) + 0.4;
                            if (dir == "U")
                            {
                                if (pos.y + spd.vy * time <= lesser_r2_y)
                                {
                                    pos.y = lesser_r2_y;
                                }
                                else
                                {
                                    pos.y += (spd.vy * time);
                                }
                            }
                            else if (dir == "D")
                            {
                                if (pos.y + spd.vy * time >= greater_r2_y)
                                {
                                    pos.y = greater_r2_y;
                                }
                                else
                                {
                                    pos.y += (spd.vy * time);
                                }
                            }

                            in_cross = true;
                            break;
                        }
                    }
                    if (!in_cross)
                    {
                        double upper_bound = r1_st.y - 0.4;
                        double lower_bound = r1_st.y + 0.4;
                        if (dir == "U")
                        {
                            if (pos.y + spd.vy * time <= upper_bound)
                            {
                                pos.y = upper_bound;
                            }
                            else
                            {
                                pos.y += (spd.vy * time);
                            }
                        }
                        else
                        {
                            if (pos.y + spd.vy * time >= lower_bound)
                            {
                                pos.y = lower_bound;
                            }
                            else
                            {
                                pos.y += (spd.vy * time);
                            }
                        }
                    }
                }
                else
                {

                }
            }
            else
            {
                auto r2_st = current_road.GetStart();
                auto r2_fn = current_road.GetEnd();
                double lesser_r2_y = (r2_st.y < r2_fn.y ? r2_st.y : r2_fn.y) - 0.4;
                double greater_r2_y = (r2_st.y > r2_fn.y ? r2_st.y : r2_fn.y) + 0.4;
                if (dir == "U")
                {
                    if (pos.y + spd.vy * time <= lesser_r2_y)
                    {
                        pos.y = lesser_r2_y;
                    }
                    else
                    {
                        pos.y += (spd.vy * time);
                    }
                }
                else if (dir == "D")
                {
                    if (pos.y + spd.vy * time >= greater_r2_y)
                    {
                        pos.y = greater_r2_y;
                    }
                    else
                    {
                        pos.y += (spd.vy * time);
                    }
                }
                else if (dir == "L" || dir == "R")
                {
                    bool in_cross = false;
                    const auto& road_crosses = m.GetRoadCrossses(current_road);
                    for (const auto& cross : road_crosses)
                    {
                        double lb_x = cross.x - 0.4;
                        double rb_x = cross.x + 0.4;
                        double lb_y = cross.y - 0.4;
                        double rb_y = cross.y + 0.4;
                        if (lb_x <= pos.x && pos.x <= rb_x && lb_y <= pos.y && pos.y <= rb_y)
                        {
                            const auto& crossed_roads = m.GetNeighbourRoad(cross);
                            const Road* r1 = &crossed_roads.first;
                            const Road* r2 = &crossed_roads.second;
                            if (current_road == crossed_roads.first)
                            {
                                d.SetRoad(r2);
                            }
                            else
                            {
                                d.SetRoad(r1);
                            }

                            const auto& crossed_road = d.GetRoad();

                            auto r1_st = current_road.GetStart();
                            auto r1_fn = current_road.GetEnd();
                            double lesser_r1_x = (r1_st.x < r1_fn.x ? r1_st.x : r1_fn.x) - 0.4;
                            double greater_r1_x = (r1_st.x > r1_fn.x ? r1_st.x : r1_fn.x) + 0.4;
                            if (dir == "L")
                            {
                                if (pos.x + spd.vx * time <= lesser_r1_x)
                                {
                                    pos.x = lesser_r1_x;
                                }
                                else
                                {
                                    pos.x += (spd.vx * time);
                                }
                            }
                            else if (dir == "R")
                            {
                                if (pos.x + spd.vx * time >= greater_r1_x)
                                {
                                    pos.x = greater_r1_x;
                                }
                                else
                                {
                                    pos.x += (spd.vx * time);
                                }
                            }

                            in_cross = true;
                            break;
                        }
                    }
                    if (!in_cross)
                    {
                        double left_bound = r2_st.x - 0.4;
                        double right_bound = r2_st.x + 0.4;
                        if (dir == "L")
                        {
                            if (pos.x + spd.vx * time <= left_bound)
                            {
                                pos.x = left_bound;
                            }
                            else
                            {
                                pos.x += (spd.vx * time);
                            }
                        }
                        else
                        {
                            if (pos.x + spd.vx * time >= right_bound)
                            {
                                pos.x = right_bound;
                            }
                            else
                            {
                                pos.x += (spd.vx * time);
                            }
                        }
                    }
                }
                else
                {

                }
            }
            d.Move(pos);
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
    void UpdateAllSessions(std::uint64_t time) const {
        for (auto session_p : active_sessions_)
        {
            session_p->UpdateSession(time);
        }
    }
private:
    std::vector<std::shared_ptr<GameSession>> active_sessions_;
};

}  // namespace model
