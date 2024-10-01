#pragma once
#include <filesystem>
#include <sstream>
#include <variant>
#include "model.h"
#include "json.h"
#include "json_builder.h"
#include "http_server.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/json.hpp>
#include "app.h"

namespace http_handler {
    namespace net = boost::asio;
    namespace fs = std::filesystem;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;
    namespace logging = boost::log;
    using namespace std::literals;

    template<class SomeRequestHandler>
    class LoggingRequestHandler {
        template <typename Body, typename Allocator>
        void LogRequest(http::request<Body, http::basic_fields<Allocator>>&& req, std::string ip_string) {
            auto tick = boost::posix_time::microsec_clock::local_time(); 
            boost::json::value custom_data{ {"message", "request received"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"ip", ip_string}, {"URI", static_cast<std::string>(req.target())}, {"method", static_cast<std::string>(req.method_string())}}} };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
        }
    public:
        LoggingRequestHandler(SomeRequestHandler&& handler) : decorated_(std::forward<SomeRequestHandler>(handler)) {}
        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, std::string ip_string)
        {
            LogRequest(std::forward<decltype(req)>(req), ip_string);
            decorated_(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        }
    private:
        SomeRequestHandler decorated_;
    };


    class DurationMeasure {
    public:
        DurationMeasure(int& time_var) :total_time_(time_var) {}
        ~DurationMeasure() {
            std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();
            total_time_ = (end_ts - start_ts_).count();
        }
    private:
        std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();
        int& total_time_;
    };

    class ApiHandler {

        const std::string ID = "id";
        const std::string NAME = "name";
        const std::string ROADS = "roads";
        const std::string BUILDINGS = "buildings";
        const std::string OFFICES = "offices";

        const std::string CODE = "code";
        const std::string MESSAGE = "message";

        const std::string BAD_REQUEST_CODE = "badRequest";
        const std::string BAD_REQUEST_MESSAGE = "Bad request";

        const std::string MAP_NOT_FOUND_CODE = "mapNotFound";
        const std::string MAP_NOT_FOUND_MESSAGE = "Map not found";

        const std::string FILE_NOT_FOUND_CODE = "fileNotFound";
        const std::string FILE_NOT_FOUND_MESSAGE = "File not found";

        const std::string X = "x";
        const std::string Y = "y";
        const std::string X0 = "x0";
        const std::string Y0 = "y0";
        const std::string X1 = "x1";
        const std::string Y1 = "y1";
        const std::string W = "w";
        const std::string H = "h";
        const std::string OffsetX = "offsetX";
        const std::string OffsetY = "offsetY";

        const std::string MAPS_PATH_WITHOUT_SLASH = "/api/v1/maps";
        const std::string MAPS_PATH_WITH_SLASH = "/api/v1/maps/";

        const std::string API = "/api";

        const std::string ROOT = "/";

        const std::string INVALID_METHOD = "invalidMethod";
        const std::string GET = "Allowed: GET";
        const std::string POST = "Allowed: POST";
        const std::string GET_HEAD = "Allowed: GET, HEAD";

        const std::string JOIN_GAME_WITHOUT_SLASH = "/api/v1/game/join";
        const std::string JOIN_GAME_WITH_SLASH = "/api/v1/game/join/";

        const std::string GET_PLAYERS_WITHOUT_SLASH = "/api/v1/game/players";
        const std::string GET_PLAYERS_WITH_SLASH = "/api/v1/game/players/";

        const std::string GAME_STATE_WITHOUT_SLASH = "/api/v1/game/state";
        const std::string GAME_STATE_WITH_SLASH = "/api/v1/game/state/";

        const std::string MAP_ID = "mapId";
        const std::string USER_NAME = "userName";
        const std::string INVALID_ARGUEMENT = "invalidArgument";
        const std::string INVALID_NAME = "Invalid name";
        const std::string JOIN_GAME_PARSE_ERROR = "Join game request parse error";
        const std::string AUTH_TOKEN = "authToken";
        const std::string PLAYER_ID = "playerId";

        const std::string ALLOWED_TOKEN_SYMBOLS = "0123456789abcdefABCDEF";
        const std::string INVALID_TOKEN = "invalidToken";
        const std::string AUTH_FAILED_MESSAGE="Authorization header is missing/contains invalid value";
        const std::string UNKNOWN_TOKEN="unknownToken";
        const std::string PLAYER_NOT_FOUND_MESSAGE="Player token has not been found";
        const std::string PLAYERS = "players";

        const std::string POS = "pos";
        const std::string SPEED = "speed";
        const std::string DIR = "dir";

        const std::string ACTION_WITHOUT_SLASH = "/api/v1/game/player/action";
        const std::string ACTION_WITH_SLASH = "/api/v1/game/player/action/";

        const std::string GAME_ACTION_PARSE_ERROR_OR_CONTENT_TYPE_ERROR = "Failed to parse action/Invalid content type";
        const std::string MOVE = "move";

        const std::string GAME_TICK_WITHOUT_SLASH = "/api/v1/game/tick";
        const std::string GAME_TICK_WITH_SLASH = "/api/v1/game/tick/";
        const std::string GAME_TICK_PARSE_ERROR_OR_CONTENT_TYPE_ERROR = "Failed to parse tick/Invalid content type";
        const std::string TIME_DELTA = "timeDelta";
        const std::string INVALID_ENDPOINT = "Invalid endpoint";

        const std::string LOOT_TYPES = "\"lootTypes\"";
        const std::string LOST_OBJECTS = "lostObjects";
        const std::string TYPE = "type";

        const std::string BAG = "bag";
        const std::string SCORE = "score";

        //const std::string MAP_DOG_SPEED = "\"dogSpeed\"";
        //const std::string MAP_BAG_CAPACITY = "\"bagCapacity\"";

        using StringRequest = http::request<http::string_body>;

        using StringResponse = http::response<http::string_body>;

        struct ContentType {
            ContentType() = delete;
            constexpr static std::string_view JSON = "application/json"sv;
        };

        struct Allow {
            Allow() = delete;
            constexpr static std::string_view GET = "GET"sv;
            constexpr static std::string_view POST = "POST"sv;
            constexpr static std::string_view GET_HEAD = "GET, HEAD"sv;
        };

        struct Cache {
            Cache() = delete;
            constexpr static std::string_view NO_CACHE = "no-cache"sv;
        };

    public:
        explicit ApiHandler(model::Game& game, app::Players& players, const model::SessionManager& sm, std::uint64_t tick_period, bool randomize_spawn_points, loot_gen::LootGenerator lg, app::ApplicationListener& listener)
            : game_{ game }, players_(players), sm_(sm), tick_period_(tick_period), randomize_spawn_points_(randomize_spawn_points), lg_(lg), listener_(&listener) {
        }

        bool IsApiRequest(StringRequest request) const;

        StringResponse MakeStringResponse(http::status status, std::string_view body, int x, unsigned http_version,
            bool keep_alive,
            std::string_view content_type = ContentType::JSON) const;

        StringResponse MakeStringCacheResponse(http::status status, std::string_view body, int x, unsigned http_version,
            bool keep_alive,
            std::string_view cache, std::string_view content_type = ContentType::JSON) const;

        StringResponse MakeStringInvalidResponse(http::status status, std::string_view body, int x, unsigned http_version,
            bool keep_alive,
            std::string_view allow, std::string_view content_type = ContentType::JSON) const;

        StringResponse MakeStringInvalidCacheResponse(http::status status, std::string_view body, int x, unsigned http_version,
            bool keep_alive,
            std::string_view allow, std::string_view cache, std::string_view content_type = ContentType::JSON) const;

        std::string GetMaps(json::Builder& builder, const model::Game& gm) const;
        std::string GetMapWithSpecificId(json::Builder& builder, const model::Map* m) const;
        std::string BadRequest() const;
        std::string MapNotFound(json::Builder& builder) const;
        json::Array CollectRoads(const model::Map* m) const;
        json::Array CollectBuildings(const model::Map* m) const;
        json::Array CollectOffices(const model::Map* m) const;

        StringResponse HandleRequest(StringRequest&& request, const model::Game& gm) const;

        std::string urlDecode(const std::string& SRC) const;

        std::string InvalidMethod(json::Builder& builder, const std::string& msg) const;
        std::string BadRequest(const std::string& code, const std::string& msg) const;
        std::string AuthRequest(json::Builder& builder, const std::string& auth_token, int player_id) const;
        bool IsValid(const std::string& auth_token) const;
        std::string AuthFailed(json::Builder& builder) const;
        std::string PlayerNotFound(json::Builder& builder) const;
        std::string GetPlayers(json::Builder& builder, const std::vector<model::Dog>& dogs) const;
        std::string GameState(json::Builder& builder, const std::vector<model::Dog>& dogs, const std::unordered_map<std::uint64_t, model::LostObject>& lost_objects) const;
        //std::string MoveRequestOrTimeTickRequest(json::Builder& builder) const;
        void Tick(std::uint64_t time) const;

    private:
        model::Game& game_;
        app::Players& players_;
        const model::SessionManager& sm_;
        std::uint64_t tick_period_;
        bool randomize_spawn_points_;
        loot_gen::LootGenerator lg_;
        app::ApplicationListener* listener_=nullptr;
    };

    class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
        const std::string ID = "id";
        const std::string NAME = "name";
        const std::string ROADS = "roads";
        const std::string BUILDINGS = "buildings";
        const std::string OFFICES = "offices";

        const std::string CODE = "code";
        const std::string MESSAGE = "message";

        const std::string BAD_REQUEST_CODE = "badRequest";
        const std::string BAD_REQUEST_MESSAGE = "Bad request";

        const std::string MAP_NOT_FOUND_CODE = "mapNotFound";
        const std::string MAP_NOT_FOUND_MESSAGE = "Map not found";

        const std::string FILE_NOT_FOUND_CODE = "fileNotFound";
        const std::string FILE_NOT_FOUND_MESSAGE = "File not found";

        const std::string X = "x";
        const std::string Y = "y";
        const std::string X0 = "x0";
        const std::string Y0 = "y0";
        const std::string X1 = "x1";
        const std::string Y1 = "y1";
        const std::string W = "w";
        const std::string H = "h";
        const std::string OffsetX = "offsetX";
        const std::string OffsetY = "offsetY";

        const std::string MAPS_PATH_WITHOUT_SLASH = "/api/v1/maps";
        const std::string MAPS_PATH_WITH_SLASH = "/api/v1/maps/";

        const std::string API = "/api";

        const std::string ROOT = "/";

        const std::string INVALID_METHOD = "invalidMethod";
        const std::string GET_HEAD = "Allowed: GET, HEAD";

        using StringRequest = http::request<http::string_body>;

        using StringResponse = http::response<http::string_body>;

        using FileResponse = http::response<http::file_body>;

        using Strand = net::strand<net::io_context::executor_type>;

        struct ContentType {
            ContentType() = delete;
            constexpr static std::string_view JSON = "application/json"sv;
            constexpr static std::string_view TEXT = "text/plain"sv;
        };

        struct Allow {
            Allow() = delete;
            constexpr static std::string_view GET_HEAD = "GET, HEAD"sv;
        };

        enum class FileResponseErrors {
            SUB_PATH_FAIL,
            OPEN_FAIL
        };
    public:
        explicit RequestHandler(model::Game& game, const std::string& static_path, Strand api_strand, ApiHandler& api_handler)
            : game_{ game }, static_abs_path_{ static_path }, api_strand_{ api_strand }, api_handler_{api_handler} {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            if (api_handler_.IsApiRequest(req))
            {
                auto handle = [self = shared_from_this(), send, req = std::forward<decltype(req)>(req)] {
                    try {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(self->api_strand_.running_in_this_thread());
                        int time;
                        DurationMeasure* dm = new DurationMeasure(time);
                        StringResponse response = self->HandleApiRequest(const_cast<http::request<Body, http::basic_fields<Allocator>> && >(req), self->game_);
                        dm->~DurationMeasure();
                        std::string CT;
                        for (auto& field : response.base())
                        {
                            std::ostringstream temp; temp << field.name();
                            if (temp.str() == "Content-Type")
                            {
                                CT = field.value();
                                break;
                            }
                        }
                        auto tick = boost::posix_time::microsec_clock::local_time();
                        boost::json::value custom_data{ {"message", "response sent"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"response_time", time}, {"code", response.result_int()}, {"content_type", CT}}} };
                        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
                        return send(response);
                    }
                    catch (...) {
                    }
                    };
                return net::dispatch(api_strand_, handle);
            }
            int time;
            DurationMeasure* dm = new DurationMeasure(time);
            auto response = HandleRequest(std::forward<decltype(req)>(req), game_);
            dm->~DurationMeasure();
            if (std::holds_alternative<FileResponse>(response))
            {
                auto response_specified = std::move(get<FileResponse>(response));
                std::string CT;
                for (auto& field : response_specified.base())
                {
                    std::ostringstream temp; temp << field.name();
                    if (temp.str() == "Content-Type")
                    {
                        CT = field.value();
                        break;
                    }
                }
                auto tick = boost::posix_time::microsec_clock::local_time();
                boost::json::value custom_data{ {"message", "response sent"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"response_time", time}, {"code", response_specified.result_int()}, {"content_type", CT}}} };
                BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
                return send(response_specified);
            }
            else
            {
                auto response_specified = std::move(get<StringResponse>(response));
                std::string CT;
                for (auto& field : response_specified.base())
                {
                    std::ostringstream temp; temp << field.name();
                    if (temp.str() == "Content-Type")
                    {
                        CT = field.value();
                        break;
                    }
                }
                auto tick = boost::posix_time::microsec_clock::local_time();
                boost::json::value custom_data{ {"message", "response sent"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"response_time", time}, {"code", response_specified.result_int()}, {"content_type", CT}}} };
                BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
                return send(response_specified);
            }
        }

        StringResponse MakeStringResponse(http::status status, std::string_view body, int x, unsigned http_version,
            bool keep_alive,
            std::string_view content_type = ContentType::JSON) const;

        StringResponse MakeStringInvalidResponse(http::status status, std::string_view body, int x, unsigned http_version,
            bool keep_alive,
            std::string_view allow, std::string_view content_type = ContentType::JSON) const;

        std::variant<FileResponse, FileResponseErrors> MakeFileResponse(fs::path path) const;

        std::string BadRequest() const;
        std::string FileNotFound(json::Builder& builder) const;
        std::variant<RequestHandler::StringResponse, RequestHandler::FileResponse> HandleRequest(StringRequest&& req, const model::Game& gm) const;
        RequestHandler::StringResponse HandleApiRequest(StringRequest&& req, const model::Game& gm) const;
        std::string DetectContentType(const std::string& ext) const;
        bool IsSubPath(fs::path path, fs::path base) const;
        std::string urlDecode(const std::string& SRC) const;
        std::string InvalidMethod(json::Builder& builder, const std::string& msg) const;

    private:
        model::Game& game_;
        fs::path static_abs_path_;
        Strand api_strand_;
        ApiHandler& api_handler_;
    };

}