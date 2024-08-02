#pragma once
#include <filesystem>
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

namespace http_handler {
namespace fs = std::filesystem;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
using namespace std::literals;

class RequestHandler {
constexpr static const char* ID = "id";
constexpr static const char* NAME = "name";
constexpr static const char* ROADS = "roads";
constexpr static const char* BUILDINGS = "buildings";
constexpr static const char* OFFICES = "offices";

constexpr static const char* CODE = "code";
constexpr static const char* MESSAGE = "message";

constexpr static const char* BAD_REQUEST_CODE = "badRequest";
constexpr static const char* BAD_REQUEST_MESSAGE = "Bad request";

constexpr static const char* MAP_NOT_FOUND_CODE = "mapNotFound";
constexpr static const char* MAP_NOT_FOUND_MESSAGE = "Map not found";

constexpr static const char* FILE_NOT_FOUND_CODE = "fileNotFound";
constexpr static const char* FILE_NOT_FOUND_MESSAGE = "File not found";

constexpr static const char* X = "x";
constexpr static const char* Y = "y";
constexpr static const char* X0 = "x0";
constexpr static const char* Y0 = "y0";
constexpr static const char* X1 = "x1";
constexpr static const char* Y1 = "y1";
constexpr static const char* W = "w";
constexpr static const char* H = "h";
constexpr static const char* OffsetX = "offsetX";
constexpr static const char* OffsetY = "offsetY";

constexpr static const char* MAPS_PATH_WITHOUT_SLASH = "/api/v1/maps";
constexpr static const char* MAPS_PATH_WITH_SLASH = "/api/v1/maps/";

constexpr static const char* ROOT = "/";

constexpr static std::string_view INVALID_METHOD = "Invalid method"sv;

using StringRequest = http::request<http::string_body>;

using StringResponse = http::response<http::string_body>;

using FileResponse = http::response<http::file_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "application/json"sv;
};

struct Allow {
    Allow() = delete;
    constexpr static std::string_view GET = "GET"sv;
};

enum class FileResponseErrors {
    SUB_PATH_FAIL,
    OPEN_FAIL
};
public:
    explicit RequestHandler(model::Game& game, const std::string& static_path)
        : game_{ game }, static_abs_path_{ static_path } {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto response = HandleRequest(std::forward<decltype(req)>(req), game_);
        try {
            send(get<FileResponse>(response));
        }
        catch (...)
        {

        }
        try {
            send(get<StringResponse>(response));
        }
        catch (...)
        {

        }
    }

    StringResponse MakeStringResponse(http::status status, std::string_view body, int x, unsigned http_version,
        bool keep_alive,
        std::string_view content_type = ContentType::TEXT_HTML) const;

    StringResponse MakeStringInvalidResponse(http::status status, std::string_view body, int x, unsigned http_version,
        bool keep_alive,
        std::string_view content_type = ContentType::TEXT_HTML, std::string_view allow = Allow::GET) const;

    std::variant<FileResponse, FileResponseErrors> MakeFileResponse(fs::path path) const;

    std::string GetMaps(json::Builder& builder, const model::Game& gm) const;
    std::string GetMapWithSpecificId(json::Builder& builder, const model::Map* m) const;
    std::string BadRequest(json::Builder& builder) const;
    std::string MapNotFound(json::Builder& builder) const;
    std::string FileNotFound(json::Builder& builder) const;
    json::Array CollectRoads(const model::Map* m) const;
    json::Array CollectBuildings(const model::Map* m) const;
    json::Array CollectOffices(const model::Map* m) const;
    std::variant<RequestHandler::StringResponse, RequestHandler::FileResponse> HandleRequest(StringRequest&& req, const model::Game& gm) const;
    std::string DetectContentType(const std::string& ext) const;
    bool IsSubPath(fs::path path, fs::path base) const;
    std::string urlDecode(const std::string& SRC) const;

private:
    model::Game& game_;
    fs::path static_abs_path_;
};

}
