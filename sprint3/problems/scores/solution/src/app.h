#pragma once
#include "model.h"
#include <random>
#include <iostream>
#include <unordered_map>

namespace app {
    using Token = util::Tagged<std::string, detail::TokenTag>;

    class PlayerToken {
        const int max_token_length = 32;
    public:
        PlayerToken();
        Token GetToken() const;
    private:
        std::random_device random_device_;
        std::mt19937_64 generator1_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        std::mt19937_64 generator2_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        // To generate a token, get two 64-bit numbers from generator1_ and generator2_
        // and, having converted them to hex strings, glue them together.
        // You can experiment with the token generation algorithm,
        // to make their selection even more difficult
        std::ostringstream strm_;
    };

	class Player {
    public:
        Player();
        Player(model::Dog dog, std::shared_ptr<model::GameSession> session, Token token);
        const model::Dog& GetDog() const;
        void SetDir(std::string dir);
        void SetSpeed(int vX, int vY, double dds);
        void SetSpeed(std::string dir, double game_default_speed);
        void SyncronizeSession();
        const model::GameSession& GetSession() const;
        Token GetAuthToken() const;
	private:
        model::Dog dog_;
        std::shared_ptr<model::GameSession> session_;
		Token token_;
	};

	class Players {
        struct PlayerInfo {
            bool operator==(const PlayerInfo& other) const { return dog_id == other.dog_id && map_id == other.map_id; }
            std::uint64_t dog_id;
            std::string map_id;
        };
        struct PlayerInfoHasher {
            size_t operator()(const PlayerInfo& pi) const {
                return std::hash<std::uint64_t>{}(pi.dog_id) + std::hash<std::string>{}(pi.map_id);
            }
        };
    public:
        Player& Addplayer(model::Dog dog, std::shared_ptr<model::GameSession> session);
        Player& FindByDogIdAndMapId(std::uint64_t dog_id, std::string map_id);
        Player& FindByToken(Token token);
        void SyncronizeSession();
    private:
        std::unordered_map<PlayerInfo, Player, PlayerInfoHasher> players_by_info_;
        std::unordered_map<Token, Player, util::TaggedHasher<Token>> players_by_token_;
	};
}
