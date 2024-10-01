#include "app.h"

namespace app {
    PlayerToken::PlayerToken() {
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
    Token PlayerToken::GetToken() const { return Token{ strm_.str() }; }

    Player::Player() : token_(std::make_unique<PlayerToken>()->GetToken()) {}
    Player::Player(model::Dog dog, std::shared_ptr<model::GameSession> session, Token token) : dog_(dog), session_(session), token_(token) {}
    const model::Dog& Player::GetDog() const { return dog_; }
    void Player::SetDir(std::string dir) { dog_.SetDir(dir); (session_->FindDog(dog_.GetName(), dog_.GetId())).SetDir(dir); }
    void Player::SetSpeed(int vX, int vY, double dds)
    {
        auto map = session_->GetMap();
        double vxx = vX * (map.GetSpecificMapDogSpeed() ? map.GetSpecificMapDogSpeed() : dds);
        double vyy = vY * (map.GetSpecificMapDogSpeed() ? map.GetSpecificMapDogSpeed() : dds);
        dog_.SetSpeed(vxx, vyy);
        (session_->FindDog(dog_.GetName(), dog_.GetId())).SetSpeed(vxx, vyy);
    }
    void Player::SetSpeed(std::string dir, double game_default_speed) {
        if (dir == "U")
        {
            SetSpeed(0, -1, game_default_speed);
        }
        else if (dir == "D")
        {
            SetSpeed(0, 1, game_default_speed);
        }
        else if (dir == "L")
        {
            SetSpeed(-1, 0, game_default_speed);
        }
        else if (dir == "R")
        {
            SetSpeed(1, 0, game_default_speed);
        }
        else
        {
            SetSpeed(0, 0, game_default_speed);
        }
    }
    void Player::SyncronizeSession() {
        dog_ = (session_->FindDog(dog_.GetName(), dog_.GetId()));
    }
    const model::GameSession& Player::GetSession() const { return *session_; }
    std::shared_ptr<model::GameSession> Player::GetSessionPtr() const { return session_; }
    Token Player::GetAuthToken() const { return token_; }

    Player& Players::Addplayer(model::Dog dog, std::shared_ptr<model::GameSession> session)
    {
        auto token = std::make_unique<PlayerToken>()->GetToken();
        /*players_by_info_.try_emplace(PlayerInfo{dog.GetId(), *((session->GetMap()).GetId())}, dog, session, token);*/
        players_by_token_.try_emplace(token, dog, session, token);
        return players_by_token_[token];
    }
    void Players::Addplayer(Token token, Player player)
    {
        players_by_token_.try_emplace(token, player);
    }
    const std::unordered_map<Token, Player, util::TaggedHasher<Token>>& Players::GetPlayersByToken() const {
        return players_by_token_;
    }
    void Players::SetPlayersByToken(std::unordered_map<Token, Player, util::TaggedHasher<Token>>&& players_by_token) {
        players_by_token_ = std::move(players_by_token);
    }
    Player& Players::FindByToken(Token token)
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
    void Players::SyncronizeSession() {
        for (auto it = players_by_token_.begin(); it != players_by_token_.end(); it++)
        {
            (it->second).SyncronizeSession();
        }
    }
}