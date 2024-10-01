#pragma once
#include "app.h"
#include "model.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace model {
	template <typename Archive>
	void serialize(Archive& ar, model::Position& pos, [[maybe_unused]] const unsigned version) {
		ar& pos.x;
		ar& pos.y;
	}

	template <typename Archive>
	void serialize(Archive& ar, model::Speed& spd, [[maybe_unused]] const unsigned version) {
		ar& spd.vx;
		ar& spd.vy;
	}

	template <typename Archive>
	void serialize(Archive& ar, model::LostObject& lobj, [[maybe_unused]] const unsigned version) {
		ar& lobj.x_;
		ar& lobj.y_;
		ar& lobj.id_;
		ar& lobj.type_;
		ar& lobj.score_per_obj_;
	}
}

namespace app {
    template <typename Archive>
    void serialize(Archive& ar, app::Players::PlayerInfo& pinfo, [[maybe_unused]] const unsigned version) {
        ar& pinfo.dog_id;
        ar& pinfo.map_id;
    }
}

namespace serialization {
    class DogRepr {
    public:
        DogRepr() = default;

        explicit DogRepr(const model::Dog& dog)
            : name_(dog.GetName())
            , id_(dog.GetId())
            , pos_(dog.GetPos())
            , spd_(dog.GetSpd())
            , dir_(dog.GetDir())
            , bag_(dog.GetBag())
            , score_(dog.GetScore()), map_(dog.GetRoad().GetMap()), road_id_(*dog.GetRoad().GetId()) {
        }

        [[nodiscard]] model::Dog Restore(const model::Game& game) const {
            model::Dog dog{name_, id_};
            dog.Move(pos_);
            const model::Map* map = game.FindMap(model::Map::Id{ map_ });
            const model::Road* road = map->FindRoad(model::Road::Id{ road_id_ });
            dog.SetRoad(road);
            dog.SetSpeed(spd_.vx, spd_.vy);
            dog.SetDir(dir_);
            for (const auto& item : bag_) {
                dog.AddLoot(item);
            }
            dog.SetScore(score_);
            return dog;
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar& name_;
            ar& id_;
            ar& pos_;
            ar& spd_;
            ar& dir_;
            ar& bag_;
            ar& score_;
            ar& map_;
            ar& road_id_;
        }

    private:
        std::string name_ = "";
        std::uint64_t id_ = 0;
        model::Position pos_;
        model::Speed spd_;
        std::string dir_ = "U";
        std::vector<model::LostObject> bag_;
        std::uint64_t score_ = 0;
        std::string map_;
        std::uint64_t road_id_ = 0;
    };

    class SessionRepr {
    public:
        SessionRepr() = default;

        explicit SessionRepr(const model::GameSession& session) 
            : id_(session.GetId()),
            lost_objects_(session.GetLostObjects()),
            loot_count_(session.GetLootCount()), map_(*session.GetMap().GetId())
        {
            for (const auto& dog : session.GetDogs())
                dogs_.emplace_back(dog);
        }

        [[nodiscard]] model::GameSession Restore(const model::Game& game) const {  
            const model::Map* map = game.FindMap(model::Map::Id{ map_ });
            model::GameSession session{ map, id_ };
            for (const auto& dog : dogs_)
            {
                session.AddDog(dog.Restore(game));
            }
            for (auto it = lost_objects_.begin(); it != lost_objects_.end(); it++)
            {
                session.AddLostObject(it->first, it->second);
            }
            session.AddLootCount(loot_count_);
            return session;
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar& id_;
            ar& lost_objects_;
            ar& loot_count_;
            ar& map_;
            ar& dogs_;
        }

    private:
        std::uint64_t id_ = 0;
        std::unordered_map<std::uint64_t, model::LostObject> lost_objects_;
        int loot_count_ = 0;
        std::string map_;
        std::vector<DogRepr> dogs_;
    };

    class PlayerRepr {
    public:
        PlayerRepr() = default;

        explicit PlayerRepr(const app::Player& player) : dog_(player.GetDog()), token_(*player.GetAuthToken()), map_(*player.GetSession().GetMap().GetId()), session_id_(player.GetSession().GetId()) {}
        
        [[nodiscard]] app::Player Restore(const model::Game& game, const model::SessionManager& sm) const {
            const model::Map* map = game.FindMap(model::Map::Id{ map_ });
            return app::Player{ dog_.Restore(game), sm.FindSession(map, session_id_), app::Token{token_}};
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar& dog_;
            ar& token_;
            ar& map_;
            ar& session_id_;
        }

    private:
        DogRepr dog_;
        std::string token_;
        std::string map_;
        std::uint64_t session_id_ = 0;
    };

    class PlayersRepr {
    public:
        PlayersRepr() = default;

        explicit PlayersRepr(const app::Players& players) {
            const auto& players_by_token = players.GetPlayersByToken();
            for (auto it = players_by_token.begin(); it != players_by_token.end(); it++)
            {
                players_by_token_.try_emplace(*(it->first), it->second);
            }
        }

        [[nodiscard]] app::Players Restore(const model::Game& game, const model::SessionManager& sm) const {
            app::Players players;
            for (auto it = players_by_token_.begin(); it != players_by_token_.end(); it++)
            {
                players.Addplayer(app::Token{ it->first }, (it->second).Restore(game, sm));
            }
            return players;
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar& players_by_token_;
        }

    private:
        std::unordered_map<std::string, PlayerRepr> players_by_token_;
    };
}

class SerializingListener : public app::ApplicationListener {
    using InputArchive = boost::archive::text_iarchive;
    using OutputArchive = boost::archive::text_oarchive;
public:
	SerializingListener(std::string save_path, std::uint64_t save_period) : save_path_(save_path), save_period_(save_period) {}
	void OnTick(std::uint64_t delta, const model::SessionManager& sm, const app::Players& players) override {
        if (save_path_ != "" && save_period_ != 0)
        {
            time_since_save_ += delta;
            if (time_since_save_ >= save_period_)
            {
                Save(sm, players);
                time_since_save_ = 0;
            }
        }
	}
    
    bool Restore(model::SessionManager& sm, app::Players& players, const model::Game& game) const override {
        std::ifstream in{ save_path_ };
        InputArchive input_archive{ in };
        try {
            model::SessionManager smanager(game.GetMaps());
            smanager.ClearSessions();
            size_t sessions_count; input_archive >> sessions_count;
            for (int i=0; i<sessions_count; i++)
            {
                serialization::SessionRepr srepr;
                input_archive >> srepr;
                smanager.AddSession(std::make_shared<model::GameSession>(std::move(srepr.Restore(game))));
            }
            serialization::PlayersRepr prepr;
            input_archive >> prepr;
            const auto& restored_players = prepr.Restore(game, smanager);
            sm = smanager;
            players.SetPlayersByToken(std::move(const_cast<std::unordered_map<app::Token, app::Player, util::TaggedHasher<app::Token>>&>(restored_players.GetPlayersByToken())));
            in.close();
            return true;
        }
        catch (const boost::archive::archive_exception& ex)
        {
            in.close();
            return false;
        }
    }
    
    void Save(const model::SessionManager& sm, const app::Players& players) const override {
        std::ofstream out{ save_path_ };
        OutputArchive output_archive{ out };
        const auto& sessions = sm.GetAllSessions();
        output_archive << sessions.size();
        for (auto session_ptr : sessions)
        {
            output_archive << serialization::SessionRepr{ *session_ptr };
        }
        output_archive << serialization::PlayersRepr{ players };
        out.close();
        //std::filesystem::rename(temp_path_, save_path_);
    }
private:
	std::string save_path_;
    std::uint64_t save_period_;
    std::uint64_t time_since_save_ = 0;
    //std::string temp_path_ = save_path_.substr(0, save_path_.find_last_of('.')) + "temp" + save_path_.substr(save_path_.find_last_of('.'));
};