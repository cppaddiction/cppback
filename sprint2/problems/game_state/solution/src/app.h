#pragma once
#include "model.h"
#include <random>
#include <unordered_map>

namespace app {
    using Token = util::Tagged<std::string, detail::TokenTag>;

    class PlayerToken {
	const int max_token_length = 32;
    public:
	PlayerToken() {
    		strm_ << std::hex << generator1_() << generator2_();
    		int x = strm_.str().size();
    		if (x < max_token_length)
    		{
        		for (int i = 0; i < max_token_length - x; i++)
        		{
            			strm_ << '0';
        		}
    		}
	}
        Token GetToken() const { return Token{ strm_.str() }; }
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
        // ����� ������������� �����, �������� �� generator1_ � generator2_
        // ��� 64-��������� ����� �, �������� �� � hex-������, ������� � ����.
        // �� ������ �������������������� � ���������� ������������� �������,
        // ����� ������� �� ������ ��� ����� ���������������
        std::ostringstream strm_;
    };

	class Player {
    public:
        Player() : token_(std::make_unique<PlayerToken>()->GetToken()) {}
        Player(model::Dog dog, std::shared_ptr<model::GameSession> session, Token token) : dog_(dog), session_(session), token_(token) {}
        const model::Dog& GetDog() const { return dog_; }
        const model::GameSession& GetSession() const { return *session_; }
        Token GetAuthToken() const { return token_; }
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
        Player& Addplayer(model::Dog dog, std::shared_ptr<model::GameSession> session)
        {
            auto token = std::make_unique<PlayerToken>()->GetToken();
            players_by_info_.try_emplace(PlayerInfo{ dog.GetId(), *((session->GetMap()).GetId()) }, dog, session, token);
            players_by_token_.try_emplace(token, dog, session, token);
            return players_by_token_[token];
        }
        Player& FindByDogIdAndMapId(std::uint64_t dog_id, std::string map_id)
        {
            if (auto search = players_by_info_.find(PlayerInfo{ dog_id, map_id }); search != players_by_info_.end())
            {
                return players_by_token_[(search->second).GetAuthToken()];
            }
            else
            {
                throw std::logic_error("No such player");
            }
        }
        Player& FindByToken(Token token)
        {
            if (auto search = players_by_token_.find(token); search != players_by_token_.end())
            {
                return players_by_token_[search->first];
            }
            else
            {
                throw std::logic_error("No such player");
            }
        }
    private:
        std::unordered_map<PlayerInfo, Player, PlayerInfoHasher> players_by_info_;
        std::unordered_map<Token, Player, util::TaggedHasher<Token>> players_by_token_;
	};
}
