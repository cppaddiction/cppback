#include "model.h"
#include "loottypes.h"
#include <stdexcept>
#include <algorithm>

namespace model {
std::uint64_t dogs_ids = 0;
std::uint64_t lost_obj_ids = 0;
const double road_offset = 0.4;

const double item_width = 0.0;
const double gatherer_width = 0.3;
const double base_width = 0.25;

using namespace std::literals;

std::optional<Point> IsIntersect(const Road& r1, const Road& r2) {
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

int GetRandomNumber(int max_number) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist_int(0, max_number);
    return dist_int(rd);
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

std::pair<const Road*, Position> Map::GetRandomPosition(bool randomize_spawn_points) const {
    const auto& roads = GetRoads();
    if (randomize_spawn_points)
    {
        std::random_device rd;
        std::uniform_int_distribution<int> dist_int(0, roads.size() - 1);
        const auto& road = roads[dist_int(rd)];
        auto road_start = road.GetStart(); auto road_end = road.GetEnd();
        int start, end; start = road.IsHorizontal() ? road_start.x : road_start.y; end = road.IsHorizontal() ? road_end.x : road_end.y;
        std::uniform_real_distribution<> dist_double(start, end);
        auto value = std::round(dist_double(rd) * 10) / 10;
        return std::make_pair(&road, road.IsHorizontal() ? Position{ value, static_cast<double>(road_start.y) } : Position{ static_cast<double>(road_start.x), value });
    }
    else
    {
        auto temp = roads[0].GetStart();
        return std::make_pair(&roads[0], Position{ static_cast<double>(temp.x), static_cast<double>(temp.y) });
    }

}

void Map::CreateRoadGrid() {
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

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

Dog::Dog(std::string name, Position pos, const Road* r) : name_(name), id_(dogs_ids++), pos_(pos), r_(r) {}

GameSession::GameSession(const Map* map, std::uint64_t id) : map_(map), id_(id) {}
void GameSession::AddDog(Dog d) { dogs_.push_back(d); }
int GameSession::GetLootCount() const { return loot_count_; }
const std::unordered_map<std::uint64_t, LostObject>& GameSession::GetLostObjects() const { return lost_objects_; }
void GameSession::AddLostObject(std::uint64_t id, LostObject obj) { lost_objects_[id] = obj; }
void GameSession::AddLootCount(int loot_count) { loot_count_ = loot_count; }
Dog& GameSession::FindDog(std::string name, std::uint64_t id) {
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
void GameSession::UpdateSession(std::uint64_t time, loot_gen::LootGenerator lg) {
    //movement implementation

    auto different_items_to_generate_amount = loot_types::to_frontend_loot_type_data[*GetMap().GetId()].size();
    auto items_to_generate = const_cast<loot_gen::LootGenerator&>(lg).Generate(std::chrono::milliseconds(time), GetLootCount(), GetPlayersAmount());

    for (int i = 0; i < items_to_generate; i++)
    {
        auto random_pos = GetMap().GetRandomPosition(true).second;
        auto lost_obj_id = lost_obj_ids++;
        auto lost_obj_type = GetRandomNumber(different_items_to_generate_amount - 1);
        AddLostObject(lost_obj_id, LostObject{random_pos.x, random_pos.y, lost_obj_id, lost_obj_type, static_cast<int>(loot_types::to_frontend_loot_type_data[*GetMap().GetId()][lost_obj_type].as_object().at(VALUE).as_int64())});
    }
    loot_count_ += items_to_generate;

    std::vector<std::pair<Position, Position>> gatherers_moves_per_tick(GetPlayersAmount());

    const auto& m = GetMap();    
    for (auto& d : dogs_)
    {
        auto pos = d.GetPos();
        auto old_pos = pos;
        auto spd = d.GetSpd();
        auto dir = d.GetDir();
        double time_sec = time * 1.0 / 1000;
        const auto& current_road = d.GetRoad();

        if (current_road.IsHorizontal())
        {
            ProcessHorizontalRoad(d, current_road, pos, spd, dir, time_sec, m);
        }
        else
        {
            ProcessVerticalRoad(d, current_road, pos, spd, dir, time_sec, m);
        }

        d.Move(pos);
        gatherers_moves_per_tick[d.GetId()] = std::make_pair(old_pos, pos);
    }

    auto offices_in_items = prov.Initialize(GetLostObjects(), gatherers_moves_per_tick, m.GetOffices());
    auto events = collision_detector::FindGatherEvents(prov);

    for (const auto& event : events)
    {
        if (lost_objects_.find(event.item_id) != lost_objects_.end() && dogs_[event.gatherer_id].CanLoot(m.GetSpecificBagCapacity()))
        {
            dogs_[event.gatherer_id].AddLoot(lost_objects_[event.item_id]);
            lost_objects_.erase(event.item_id);
            loot_count_ -= 1;
        }
        if (std::find(offices_in_items.begin(), offices_in_items.end(), event.item_id) != offices_in_items.end())
        {
            dogs_[event.gatherer_id].DropLoot();
        }
    }
    
    prov.Clear();
}
void GameSession::ProcessHorizontalRoad(model::Dog& d, const model::Road current_road, model::Position& pos, const model::Speed spd, const std::string& dir, double time_sec, const model::Map& m)
{
    auto r1_st = current_road.GetStart();
    auto r1_fn = current_road.GetEnd();
    double lesser_r1_x = (r1_st.x < r1_fn.x ? r1_st.x : r1_fn.x) - road_offset;
    double greater_r1_x = (r1_st.x > r1_fn.x ? r1_st.x : r1_fn.x) + road_offset;
    if (dir == "L")
    {
        if (pos.x + spd.vx * time_sec <= lesser_r1_x)
        {
            pos.x = lesser_r1_x;
            d.SetSpeed(0.0, 0.0);
        }
        else
        {
            pos.x += (spd.vx * time_sec);
        }
    }
    else if (dir == "R")
    {
        if (pos.x + spd.vx * time_sec >= greater_r1_x)
        {
            pos.x = greater_r1_x;
            d.SetSpeed(0.0, 0.0);
        }
        else
        {
            pos.x += (spd.vx * time_sec);
        }
    }
    else if (dir == "U" || dir == "D")
    {
        bool in_cross = false;
        const auto& road_crosses = m.GetRoadCrossses(current_road);
        for (const auto& cross : road_crosses)
        {
            double lb_x = cross.x - road_offset;
            double rb_x = cross.x + road_offset;
            double lb_y = cross.y - road_offset;
            double rb_y = cross.y + road_offset;
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
                double lesser_r2_y = (r2_st.y < r2_fn.y ? r2_st.y : r2_fn.y) - road_offset;
                double greater_r2_y = (r2_st.y > r2_fn.y ? r2_st.y : r2_fn.y) + road_offset;
                if (dir == "U")
                {
                    if (pos.y + spd.vy * time_sec <= lesser_r2_y)
                    {
                        pos.y = lesser_r2_y;
                        d.SetSpeed(0.0, 0.0);
                    }
                    else
                    {
                        pos.y += (spd.vy * time_sec);
                    }
                }
                else if (dir == "D")
                {
                    if (pos.y + spd.vy * time_sec >= greater_r2_y)
                    {
                        pos.y = greater_r2_y;
                        d.SetSpeed(0.0, 0.0);
                    }
                    else
                    {
                        pos.y += (spd.vy * time_sec);
                    }
                }

                in_cross = true;
                break;
            }
        }
        if (!in_cross)
        {
            double upper_bound = r1_st.y - road_offset;
            double lower_bound = r1_st.y + road_offset;
            if (dir == "U")
            {
                if (pos.y + spd.vy * time_sec <= upper_bound)
                {
                    pos.y = upper_bound;
                    d.SetSpeed(0.0, 0.0);
                }
                else
                {
                    pos.y += (spd.vy * time_sec);
                }
            }
            else
            {
                if (pos.y + spd.vy * time_sec >= lower_bound)
                {
                    pos.y = lower_bound;
                    d.SetSpeed(0.0, 0.0);
                }
                else
                {
                    pos.y += (spd.vy * time_sec);
                }
            }
        }
    }
}
void GameSession::ProcessVerticalRoad(model::Dog& d, const model::Road current_road, model::Position& pos, const model::Speed spd, const std::string& dir, double time_sec, const model::Map& m)
{
    auto r2_st = current_road.GetStart();
    auto r2_fn = current_road.GetEnd();
    double lesser_r2_y = (r2_st.y < r2_fn.y ? r2_st.y : r2_fn.y) - road_offset;
    double greater_r2_y = (r2_st.y > r2_fn.y ? r2_st.y : r2_fn.y) + road_offset;
    if (dir == "U")
    {
        if (pos.y + spd.vy * time_sec <= lesser_r2_y)
        {
            pos.y = lesser_r2_y;
            d.SetSpeed(0.0, 0.0);
        }
        else
        {
            pos.y += (spd.vy * time_sec);
        }
    }
    else if (dir == "D")
    {
        if (pos.y + spd.vy * time_sec >= greater_r2_y)
        {
            pos.y = greater_r2_y;
            d.SetSpeed(0.0, 0.0);
        }
        else
        {
            pos.y += (spd.vy * time_sec);
        }
    }
    else if (dir == "L" || dir == "R")
    {
        bool in_cross = false;
        const auto& road_crosses = m.GetRoadCrossses(current_road);
        for (const auto& cross : road_crosses)
        {
            double lb_x = cross.x - road_offset;
            double rb_x = cross.x + road_offset;
            double lb_y = cross.y - road_offset;
            double rb_y = cross.y + road_offset;
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

                auto r1_st = crossed_road.GetStart();
                auto r1_fn = crossed_road.GetEnd();
                double lesser_r1_x = (r1_st.x < r1_fn.x ? r1_st.x : r1_fn.x) - road_offset;
                double greater_r1_x = (r1_st.x > r1_fn.x ? r1_st.x : r1_fn.x) + road_offset;
                if (dir == "L")
                {
                    if (pos.x + spd.vx * time_sec <= lesser_r1_x)
                    {
                        pos.x = lesser_r1_x;
                        d.SetSpeed(0.0, 0.0);
                    }
                    else
                    {
                        pos.x += (spd.vx * time_sec);
                    }
                }
                else if (dir == "R")
                {
                    if (pos.x + spd.vx * time_sec >= greater_r1_x)
                    {
                        pos.x = greater_r1_x;
                        d.SetSpeed(0.0, 0.0);
                    }
                    else
                    {
                        pos.x += (spd.vx * time_sec);
                    }
                }

                in_cross = true;
                break;
            }
        }
        if (!in_cross)
        {
            double left_bound = r2_st.x - road_offset;
            double right_bound = r2_st.x + road_offset;
            if (dir == "L")
            {
                if (pos.x + spd.vx * time_sec <= left_bound)
                {
                    pos.x = left_bound;
                    d.SetSpeed(0.0, 0.0);
                }
                else
                {
                    pos.x += (spd.vx * time_sec);
                }
            }
            else
            {
                if (pos.x + spd.vx * time_sec >= right_bound)
                {
                    pos.x = right_bound;
                    d.SetSpeed(0.0, 0.0);
                }
                else
                {
                    pos.x += (spd.vx * time_sec);
                }
            }
        }
    }
}
const Map& GameSession::GetMap() const { return *map_; }
std::uint64_t GameSession::GetId() const { return id_; }
const std::vector<Dog>& GameSession::GetDogs() const { return dogs_; }
std::uint64_t GameSession::GetPlayersAmount() const { return dogs_.size(); }

SessionManager::SessionManager(const std::vector<Map>& maps)
{
    for (const auto& map : maps)
    {
        active_sessions_.push_back(std::make_shared<GameSession>(&map, 0));
    }
}
std::shared_ptr<GameSession> SessionManager::FindSession(const Map* m, uint64_t session_id) const
{
    for (auto session : active_sessions_)
    {
        if (m->GetId() == (session->GetMap()).GetId() && session_id == session->GetId())
        {
            return session;
        }
    }
}
void SessionManager::UpdateAllSessions(std::uint64_t time, loot_gen::LootGenerator lg) const {
    for (auto session_p : active_sessions_)
    {
        session_p->UpdateSession(time, lg);
    }
}

const std::vector<std::shared_ptr<GameSession>>& SessionManager::GetAllSessions() const { return active_sessions_; }

void SessionManager::ClearSessions() { active_sessions_.clear(); }
void SessionManager::AddSession(std::shared_ptr<GameSession> sptr) { active_sessions_.push_back(sptr); }

TestItemGathererProvider::TestItemGathererProvider() {
}

std::vector<int> TestItemGathererProvider::Initialize(const std::unordered_map<std::uint64_t, model::LostObject>& items, const std::vector<std::pair<Position, Position>>& gatherers, const std::vector<model::Office>& offices) {
    std::vector<int> offices_in_items;
    items_.resize(items.size() + offices.size());
    for (auto it = items.begin(); it != items.end(); it++)
    {
        items_[it->first] = collision_detector::Item{ geom::Point2D{ it->second.GetX(), it->second.GetY() }, item_width };
    }
    for (int i = items.size(), j = 0; i < items_.size(); i++, j++)
    {
        auto office_pos = offices[j].GetPosition();
        items_[i] = collision_detector::Item{ geom::Point2D{ static_cast<double>(office_pos.x), static_cast<double>(office_pos.y)}, base_width};
        offices_in_items.push_back(i);
    }
    for (int i=0; i<gatherers.size(); i++)
    {
        gatherers_.emplace_back(geom::Point2D{gatherers[i].first.x, gatherers[i].first.y}, geom::Point2D{ gatherers[i].second.x, gatherers[i].second.y }, gatherer_width);
    }
    return offices_in_items;
}

void TestItemGathererProvider::Clear() {
    items_.clear();
    gatherers_.clear();
}

size_t TestItemGathererProvider::ItemsCount() const {
    return items_.size();
}

collision_detector::Item TestItemGathererProvider::GetItem(size_t idx) const {
    return items_.at(idx);
}

size_t TestItemGathererProvider::GatherersCount() const {
    return gatherers_.size();
}

collision_detector::Gatherer TestItemGathererProvider::GetGatherer(size_t idx) const {
    return gatherers_.at(idx);
}

Road::Road(HorizontalTag, Point start, Coord end_x, std::string map, Id id) noexcept
    : start_{ start }
    , end_{ end_x, start.y }, map_(map), id_(id) {
}

Road::Road(VerticalTag, Point start, Coord end_y, std::string map, Id id) noexcept
    : start_{ start }
    , end_{ start.x, end_y }, map_(map), id_(id) {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

bool Road::operator==(const Road& other) const {
    return GetStart() == other.GetStart() && GetEnd() == other.GetEnd();
}

const std::string& Road::GetMap() const {
    return map_;
}
Road::Id Road::GetId() const {
    return id_;
}

Building::Building(Rectangle bounds) noexcept
    : bounds_{ bounds } {
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

Office::Office(Id id, Point position, Offset offset) noexcept
    : id_{ std::move(id) }
    , position_{ position }
    , offset_{ offset } {
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

Point Office::GetPosition() const noexcept {
    return position_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}

Map::Map(Id id, std::string name) noexcept
    : id_(std::move(id))
    , name_(std::move(name)) {
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddSpecificMapDogSpeed(double speed) {
    specific_map_dog_speed_ = speed;
}

void Map::AddSpecificBagCapacity(std::uint64_t bag_capacity) {
    bag_capacity_ = bag_capacity;
}

double Map::GetSpecificMapDogSpeed() const {
    return specific_map_dog_speed_;
}

std::uint64_t Map::GetSpecificBagCapacity() const {
    return bag_capacity_;
}

const std::vector<Point>& Map::GetRoadCrossses(const Road& road) const {
    return roads_to_crosses_.at(road);
}

const std::pair<Road, Road>& Map::GetNeighbourRoad(const Point& cross) const {
    return grid_.at(cross);
}

const Road* Map::FindRoad(Road::Id id) const {
    for (const auto& road : roads_)
    {
        if (road.GetId() == id)
        {
            return &road;
        }
    }
    return nullptr;
}

void Game::AddDefaultDogSpeed(double speed) {
    default_dog_speed_ = speed;
}

void Game::AddDefaultBagCapacity(std::uint64_t def_bag_capacity) {
    def_bag_capacity_ = def_bag_capacity;
}

double Game::GetDefaultDogSpeed() const {
    return default_dog_speed_;
}

std::uint64_t Game::GetDefaultBagCapacity() const {
    return def_bag_capacity_;
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

LostObject::LostObject(double x, double y, std::uint64_t id, int type, int score_per_obj) : x_(x), y_(y), id_(id), type_(type), score_per_obj_(score_per_obj) {}
double LostObject::GetX() const { return x_; }
double LostObject::GetY() const { return y_; }
std::uint64_t LostObject::GetId() const { return id_; }
int LostObject::GetType() const { return type_; }
int LostObject::GetScorePerObj() const { return score_per_obj_; }

Dog::Dog(std::string name, std::uint64_t id) : name_(name), id_(id) {}
bool Dog::operator==(const Dog& other) const { return name_ == other.name_ && id_ == other.id_; }
const std::string& Dog::GetName() const { return name_; }
std::uint64_t Dog::GetId() const { return id_; }
std::uint64_t Dog::GetScore() const { return score_; }
Position Dog::GetPos() const { return pos_; }
Speed Dog::GetSpd() const { return spd_; }
const std::string& Dog::GetDir() const { return dir_; }
void Dog::SetDir(std::string dir) { dir_ = dir; }
void Dog::SetSpeed(double vxx, double vyy) { spd_.vx = vxx; spd_.vy = vyy; }
void Dog::Move(Position pos) { pos_ = pos; }
const Road& Dog::GetRoad() const { return *r_; }
void Dog::SetRoad(const Road* r) { r_ = r; }
void Dog::AddLoot(const LostObject& obj) { bag_.push_back(obj); }
const std::vector<LostObject>& Dog::GetBag() const { return bag_; }
void Dog::DropLoot() {
    for (const auto& item : bag_)
    {
        score_ += item.GetScorePerObj();
    }
    bag_.clear();
}
bool Dog::CanLoot(std::uint64_t max) { return bag_.size() < max; }

void Dog::SetScore(std::uint64_t score) { score_ = score; }

}  // namespace model
