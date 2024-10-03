#include "request_handler.h"
#include "loottypes.h"

namespace http_handler {
    RequestHandler::StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body, int x, unsigned http_version,
        bool keep_alive,
        std::string_view content_type) const
    {
        RequestHandler::StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(x);
        response.keep_alive(keep_alive);
        return response;
    }

    RequestHandler::StringResponse RequestHandler::MakeStringInvalidResponse(http::status status, std::string_view body, int x, unsigned http_version,
        bool keep_alive,
        std::string_view allow, std::string_view content_type) const
    {
        RequestHandler::StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.set(http::field::allow, allow);
        response.body() = body;
        response.content_length(x);
        response.keep_alive(keep_alive);
        return response;
    }

    std::string RequestHandler::DetectContentType(const std::string& ext) const
    {
        std::string ContentType;
        std::string ModifiedExt = ext;
        boost::algorithm::to_upper(ModifiedExt);
        boost::algorithm::to_lower(ModifiedExt);
        if (ModifiedExt == "htm" || ModifiedExt == "html")
        {
            ContentType = "text/html";
        }
        else if (ModifiedExt == "css")
        {
            ContentType = "text/css";
        }
        else if (ModifiedExt == "txt")
        {
            ContentType = "text/plain";
        }
        else if (ModifiedExt == "js")
        {
            ContentType = "text/javascript";
        }
        else if (ModifiedExt == "json")
        {
            ContentType = "application/json";
        }
        else if (ModifiedExt == "xml")
        {
            ContentType = "application/xml";
        }
        else if (ModifiedExt == "png")
        {
            ContentType = "image/png";
        }
        else if (ModifiedExt == "jpg" || ModifiedExt == "jpe" || ModifiedExt == "jpeg")
        {
            ContentType = "image/jpeg";
        }
        else if (ModifiedExt == "gif")
        {
            ContentType = "image/gif";
        }
        else if (ModifiedExt == "bmp")
        {
            ContentType = "image/bmp";
        }
        else if (ModifiedExt == "ico")
        {
            ContentType = "image/vnd.microsoft.icon";
        }
        else if (ModifiedExt == "tiff" || ModifiedExt == "tif")
        {
            ContentType = "image/tiff";
        }
        else if (ModifiedExt == "svg" || ModifiedExt == "svgz")
        {
            ContentType = "image/svg+xml";
        }
        else if (ModifiedExt == "mp3")
        {
            ContentType = "audio/mpeg";
        }
        else
        {
            ContentType = "unknown";
        }
        return ContentType;
    }

    std::variant<RequestHandler::FileResponse, RequestHandler::FileResponseErrors> RequestHandler::MakeFileResponse(fs::path path) const
    {
        std::variant<RequestHandler::FileResponse, RequestHandler::FileResponseErrors> r;
        fs::path file_path;
        file_path += static_abs_path_;
        file_path += path;
        if (!IsSubPath(fs::weakly_canonical(file_path), static_abs_path_))
        {
            r = RequestHandler::FileResponseErrors::SUB_PATH_FAIL;
            return r;
        }
        http::file_body::value_type file;
        std::string temp = file_path.string();
        const char* p = &temp[0];
        if (sys::error_code ec; file.open(p, beast::file_mode::read, ec), ec) {
            r = RequestHandler::FileResponseErrors::OPEN_FAIL;
            return r;
        }
        RequestHandler::FileResponse res;
        res.version(11);  // HTTP/1.1
        res.result(http::status::ok);
        res.insert(http::field::content_type, DetectContentType(temp.substr(temp.find_last_of(".") + 1)));
        res.body() = std::move(file);
        //       prepare_payload                     Content-Length   Transfer-Encoding
        //                                        
        res.prepare_payload();
        r = std::move(res);
        return r;
    }

    bool RequestHandler::IsSubPath(fs::path path, fs::path base) const {
        //                                      (    .   ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        //          ,                    base                   path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

    std::string RequestHandler::BadRequest() const
    {
        boost::property_tree::ptree pt;
        pt.put(CODE, BAD_REQUEST_CODE);
        pt.put(MESSAGE, BAD_REQUEST_MESSAGE);
        std::ostringstream strm;
        boost::property_tree::json_parser::write_json(strm, pt);
        return strm.str();
    }

    std::string RequestHandler::FileNotFound(json::Builder& builder) const
    {
        builder.StartDict().Key(CODE).Value(FILE_NOT_FOUND_CODE).Key(MESSAGE).Value(FILE_NOT_FOUND_MESSAGE);
        return json::Print(builder.EndDict().Build());
    }

    std::string RequestHandler::InvalidMethod(json::Builder& builder, const std::string& msg) const
    {
        builder.StartDict().Key(CODE).Value(INVALID_METHOD).Key(MESSAGE).Value(msg);
        return json::Print(builder.EndDict().Build());
    }

    std::string RequestHandler::urlDecode(const std::string& SRC) const {
        std::string res;
        char ch;
        int string_index, symbol_code;
        for (string_index = 0; string_index < SRC.length(); string_index++) {
            if (SRC[string_index] == '%') {
                std::istringstream strm(SRC.substr(string_index + 1, 2)); strm >> std::hex >> symbol_code;
                ch = static_cast<char>(symbol_code);
                res += ch;
                string_index = string_index + 2;
            }
            else {
                res += SRC[string_index];
            }
        }
        return (res);
    }

    std::variant<RequestHandler::StringResponse, RequestHandler::FileResponse> RequestHandler::HandleRequest(StringRequest&& req, const model::Game& gm) const {
        const auto text_response = [&req, this](http::status status, std::string_view text, int x, std::string_view content_type) {
            return this->MakeStringResponse(status, text, x, req.version(), req.keep_alive(), content_type);
            };
        const auto text_invalid_response = [&req, this](http::status status, std::string_view text, int x, std::string_view allowed) {
            return this->MakeStringInvalidResponse(status, text, x, req.version(), req.keep_alive(), allowed);
            };
        const auto file_response = [&req, this](fs::path path) {
            return this->MakeFileResponse(path);
            };

        json::Builder builder;

        if (req.method() == http::verb::get || req.method() == http::verb::head)
        {
            auto str_form = urlDecode(static_cast<std::string>(req.target()));
            //url-decode str
            if (str_form == ROOT)
            {
                fs::path request_path{ "/index.html" };
                auto response = file_response(request_path);
                auto response_value = std::move(get<FileResponse>(response));
                return response_value;
            }
            else
            {
                fs::path request_path{ str_form };
                auto response = file_response(request_path);
                if (std::holds_alternative<FileResponse>(response))
                {
                    auto response_value = std::move(get<FileResponse>(response));
                    return response_value;
                }
                else if (std::holds_alternative<FileResponseErrors>(response))
                {
                    auto response_value = get<FileResponseErrors>(response);
                    if (response_value == FileResponseErrors::SUB_PATH_FAIL)
                    {
                        auto result = BadRequest();
                        return text_response(http::status::bad_request, result, result.size(), ContentType::JSON);
                    }
                    else
                    {
                        auto result = FileNotFound(builder);
                        return text_response(http::status::not_found, result, result.size(), ContentType::TEXT);
                    }
                }
                else
                {
                    auto result = BadRequest();
                    return text_response(http::status::bad_request, result, result.size(), ContentType::JSON);
                }
            }
        }
        else
        {
            auto result = InvalidMethod(builder, GET_HEAD);
            return text_invalid_response(http::status::method_not_allowed, result, result.size(), Allow::GET_HEAD);
        }
        //                                                   ,                        : Hello
    }

    RequestHandler::StringResponse RequestHandler::HandleApiRequest(StringRequest&& req, const model::Game& gm) const
    {
        return api_handler_.HandleRequest(std::forward<decltype(req)>(req), gm);
    }

    bool ApiHandler::IsApiRequest(StringRequest request) const
    {
        auto str_form = urlDecode(static_cast<std::string>(request.target()));
        return str_form.substr(0, 4) == API;
    }

    ApiHandler::StringResponse ApiHandler::MakeStringResponse(http::status status, std::string_view body, int x, unsigned http_version,
        bool keep_alive,
        std::string_view content_type) const
    {
        ApiHandler::StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(x);
        response.keep_alive(keep_alive);
        return response;
    }

    ApiHandler::StringResponse ApiHandler::MakeStringCacheResponse(http::status status, std::string_view body, int x, unsigned http_version,
        bool keep_alive,
        std::string_view cache, std::string_view content_type) const
    {
        ApiHandler::StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.set(http::field::cache_control, cache);
        response.body() = body;
        response.content_length(x);
        response.keep_alive(keep_alive);
        return response;
    }

    ApiHandler::StringResponse ApiHandler::MakeStringInvalidResponse(http::status status, std::string_view body, int x, unsigned http_version,
        bool keep_alive,
        std::string_view allow, std::string_view content_type) const
    {
        ApiHandler::StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.set(http::field::allow, allow);
        response.body() = body;
        response.content_length(x);
        response.keep_alive(keep_alive);
        return response;
    }

    ApiHandler::StringResponse ApiHandler::MakeStringInvalidCacheResponse(http::status status, std::string_view body, int x, unsigned http_version,
        bool keep_alive,
        std::string_view allow, std::string_view cache, std::string_view content_type) const
    {
        ApiHandler::StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.set(http::field::allow, allow);
        response.set(http::field::cache_control, cache);
        response.body() = body;
        response.content_length(x);
        response.keep_alive(keep_alive);
        return response;
    }

    std::string ApiHandler::GetMaps(json::Builder& builder, const model::Game& gm) const
    {
        builder.StartArray();
        const auto& maps = gm.GetMaps();
        for (const auto& m : maps)
        {
            builder.StartDict().Key(ID).Value(*(m.GetId())).Key(NAME).Value(m.GetName()).EndDict();
        }
        return json::Print(builder.EndArray().Build());
    }

    std::string ApiHandler::GetMapWithSpecificId(json::Builder& builder, const model::Map* m) const
    {
        builder.StartDict().Key(ID).Value(*(m->GetId())).Key(NAME).Value(m->GetName()).Key(ROADS);
        builder.Value(CollectRoads(m)).Key(BUILDINGS);
        builder.Value(CollectBuildings(m)).Key(OFFICES);
        builder.Value(CollectOffices(m));

        std::ostringstream strm; strm << loot_types::to_frontend_loot_type_data[*(m->GetId())];
        auto maps_parsed = json::Print(builder.EndDict().Build());
        auto maps_to_string = maps_parsed.substr(0, maps_parsed.size() - 1) + "," + LOOT_TYPES + ":" + strm.str() + "}";

        return maps_to_string;
    }

    std::string ApiHandler::BadRequest() const
    {
        boost::property_tree::ptree pt;
        pt.put(CODE, BAD_REQUEST_CODE);
        pt.put(MESSAGE, BAD_REQUEST_MESSAGE);
        std::ostringstream strm;
        boost::property_tree::json_parser::write_json(strm, pt);
        return strm.str();
    }

    std::string ApiHandler::MapNotFound(json::Builder& builder) const
    {
        builder.StartDict().Key(CODE).Value(MAP_NOT_FOUND_CODE).Key(MESSAGE).Value(MAP_NOT_FOUND_MESSAGE);
        return json::Print(builder.EndDict().Build());
    }

    std::string ApiHandler::InvalidMethod(json::Builder& builder, const std::string& msg) const
    {
        builder.StartDict().Key(CODE).Value(INVALID_METHOD).Key(MESSAGE).Value(msg);
        return json::Print(builder.EndDict().Build());
    }

    std::string ApiHandler::BadRequest(const std::string& code, const std::string& msg) const
    {
        boost::property_tree::ptree pt;
        pt.put(CODE, code);
        pt.put(MESSAGE, msg);
        std::ostringstream strm;
        boost::property_tree::json_parser::write_json(strm, pt);
        return strm.str();
    }

    std::string ApiHandler::AuthRequest(json::Builder& builder, const std::string& auth_token, int player_id) const
    {
        builder.StartDict().Key(AUTH_TOKEN).Value(auth_token).Key(PLAYER_ID).Value(player_id);
        return json::Print(builder.EndDict().Build());
    }

    std::string ApiHandler::AuthFailed(json::Builder& builder) const
    {
        builder.StartDict().Key(CODE).Value(INVALID_TOKEN).Key(MESSAGE).Value(AUTH_FAILED_MESSAGE);
        return json::Print(builder.EndDict().Build());
    }

    std::string ApiHandler::PlayerNotFound(json::Builder& builder) const
    {
        builder.StartDict().Key(CODE).Value(UNKNOWN_TOKEN).Key(MESSAGE).Value(PLAYER_NOT_FOUND_MESSAGE);
        return json::Print(builder.EndDict().Build());
    }

    std::string ApiHandler::MoveRequestOrTimeTickRequest(json::Builder& builder) const
    {
        builder.StartDict();
        return json::Print(builder.EndDict().Build());
    }

    std::string ApiHandler::GetPlayers(json::Builder& builder, const std::vector<model::Dog>& dogs) const
    {
        builder.StartDict();
        for (const auto& dog : dogs)
        {
            json::Builder helpbuilder;
            builder.Key(std::to_string(dog.GetId())).Value(helpbuilder.StartDict().Key(NAME).Value(dog.GetName()).EndDict().Build().AsMap());
        }
        return json::Print(builder.EndDict().Build());
    }

    std::string ApiHandler::GameState(json::Builder& builder, const std::vector<model::Dog>& dogs, const std::unordered_map<std::uint64_t, model::LostObject>& lost_objects) const
    {
        builder.StartDict().Key(PLAYERS);
        json::Builder helper;
        helper.StartDict();
        for (const auto& dog : dogs)
        {
            json::Builder helpbuilder;
            auto ps = dog.GetPos();
            auto spd = dog.GetSpd();
            json::Array pos{ json::Node(ps.x), json::Node(ps.y) };
            json::Array speed{ json::Node(spd.vx), json::Node(spd.vy) };
            json::Array bag{};
            for (const auto& item : dog.GetBag())
            {
                json::Builder help_build_bag;
                help_build_bag.StartDict().Key(ID).Value(static_cast<int>(item.GetId())).Key(TYPE).Value(item.GetType());
                bag.push_back(json::Node{ help_build_bag.EndDict().Build().AsMap() });
            }
            helper.Key(std::to_string(dog.GetId())).Value(helpbuilder.StartDict().Key(NAME).Value(dog.GetName()).Key(POS).Value(pos).Key(SPEED).Value(speed).Key(DIR).Value(dog.GetDir()).Key(BAG).Value(bag).Key(SCORE).Value(static_cast<int>(dog.GetScore())).EndDict().Build().AsMap());
        }
        builder.Value(helper.EndDict().Build().AsMap());
        json::Builder build_lost_objects;
        build_lost_objects.StartDict();
        for (auto it = lost_objects.begin(); it!=lost_objects.end(); it++)
        {
            json::Array point_info = json::Array{json::Node((it->second).GetX()), json::Node((it->second).GetY())};
            json::Builder helpbuilder;
            helpbuilder.StartDict();
            helpbuilder.Key(TYPE).Value((it->second).GetType()).Key(POS).Value(point_info);
            build_lost_objects.Key(std::to_string((it->second).GetId())).Value(helpbuilder.EndDict().Build().AsMap());
        }
        builder.Key(LOST_OBJECTS).Value(build_lost_objects.EndDict().Build().AsMap());
        return json::Print(builder.EndDict().Build());
    }
    
    json::Array ApiHandler::CollectRoads(const model::Map* m) const {
        json::Builder helpbuilder;
        helpbuilder.StartArray();
        const auto& roads = m->GetRoads();
        for (const auto& r : roads)
        {
            auto start = r.GetStart();
            auto end = r.GetEnd();
            if (r.IsHorizontal())
            {
                helpbuilder.StartDict().Key(X0).Value(start.x).Key(Y0).Value(start.y).Key(X1).Value(end.x).EndDict();
            }
            else
            {
                helpbuilder.StartDict().Key(X0).Value(start.x).Key(Y0).Value(start.y).Key(Y1).Value(end.y).EndDict();
            }
        }
        return helpbuilder.EndArray().Build().AsArray();
    }

    json::Array ApiHandler::CollectBuildings(const model::Map* m) const {
        json::Builder helpbuilder;
        helpbuilder.StartArray();
        const auto& buildings = m->GetBuildings();
        for (const auto& b : buildings)
        {
            auto rect = b.GetBounds();
            helpbuilder.StartDict().Key(X).Value(rect.position.x).Key(Y).Value(rect.position.y).Key(W).Value(rect.size.width).Key(H).Value(rect.size.height).EndDict();
        }
        return helpbuilder.EndArray().Build().AsArray();
    }

    json::Array ApiHandler::CollectOffices(const model::Map* m) const {
        json::Builder helpbuilder;
        helpbuilder.StartArray();
        const auto& offices = m->GetOffices();
        for (const auto& o : offices)
        {
            auto pos = o.GetPosition();
            auto offset = o.GetOffset();
            helpbuilder.StartDict().Key(ID).Value(*(o.GetId())).Key(X).Value(pos.x).Key(Y).Value(pos.y).Key(OffsetX).Value(offset.dx).Key(OffsetY).Value(offset.dy).EndDict();
        }
        return helpbuilder.EndArray().Build().AsArray();
    }

    bool ApiHandler::IsValid(const std::string& auth_token) const
    {
        for (const auto& ch : auth_token)
        {
            if (std::find(ALLOWED_TOKEN_SYMBOLS.begin(), ALLOWED_TOKEN_SYMBOLS.end(), ch)==ALLOWED_TOKEN_SYMBOLS.end())
            {
                return false;
            }
        }
        if (auth_token.size() == 32)
        {
            return true;
        }
        
        return false;
    }

    ApiHandler::StringResponse ApiHandler::HandleRequest(StringRequest&& req, const model::Game& gm) {
        const auto text_response = [&req, this](http::status status, std::string_view text, int x) {
            return this->MakeStringResponse(status, text, x, req.version(), req.keep_alive());
            };
        const auto text_cache_response = [&req, this](http::status status, std::string_view text, int x, std::string_view cache) {
            return this->MakeStringCacheResponse(status, text, x, req.version(), req.keep_alive(), cache);
            };
        const auto text_invalid_response = [&req, this](http::status status, std::string_view text, int x, std::string_view allowed) {
            return this->MakeStringInvalidResponse(status, text, x, req.version(), req.keep_alive(), allowed);
            };
        const auto text_invalid_cache_response = [&req, this](http::status status, std::string_view text, int x, std::string_view allowed, std::string_view cache) {
            return this->MakeStringInvalidCacheResponse(status, text, x, req.version(), req.keep_alive(), allowed, cache);
            };

        json::Builder builder;

        auto str_form = urlDecode(static_cast<std::string>(req.target()));
        //url-decode str
        if (str_form == MAPS_PATH_WITH_SLASH || str_form == MAPS_PATH_WITHOUT_SLASH)
        {
            if (req.method() == http::verb::get)
            {
                auto result = GetMaps(builder, gm);
                return text_cache_response(http::status::ok, result, result.size(), Cache::NO_CACHE);
            }

            auto result = InvalidMethod(builder, GET);
            return text_invalid_cache_response(http::status::method_not_allowed, result, result.size(), Allow::GET, Cache::NO_CACHE);
        }
        else if (str_form.substr(0, 13) == MAPS_PATH_WITH_SLASH && str_form != MAPS_PATH_WITH_SLASH)
        {
            if (req.method() == http::verb::get || req.method() == http::verb::head)
            {
                auto id = str_form.substr(13);
                if (id[id.size() - 1] == '/')
                {
                    id = id.substr(0, id.size() - 1);
                }
                if (auto m = gm.FindMap(model::Map::Id(id)))
                {
                    auto result = GetMapWithSpecificId(builder, m);
                    return text_cache_response(http::status::ok, result, result.size(), Cache::NO_CACHE);
                }

                auto result = MapNotFound(builder);
                return text_cache_response(http::status::not_found, result, result.size(), Cache::NO_CACHE);
            }

            auto result = InvalidMethod(builder, GET_HEAD);
            return text_invalid_cache_response(http::status::method_not_allowed, result, result.size(), Allow::GET_HEAD, Cache::NO_CACHE);
        }
        else if (str_form == JOIN_GAME_WITH_SLASH || str_form == JOIN_GAME_WITHOUT_SLASH)
        {
            if (req.method() == http::verb::post)
            {
                try {
                    namespace js = boost::json;
                    auto req_body = req.body();
                    auto value = js::parse(req_body).as_object();
                    auto user_name = static_cast<std::string>(value.at(USER_NAME).as_string());
                    if (user_name == "")
                    {
                        auto result = BadRequest(INVALID_ARGUEMENT, INVALID_NAME);
                        return text_cache_response(http::status::bad_request, result, result.size(), Cache::NO_CACHE);
                    }
                    auto map_id = static_cast<std::string>(value.at(MAP_ID).as_string());
                    model::Map::Id tagged_map_id{ map_id };
                    auto search = game_.FindMap(tagged_map_id);
                    if (search == nullptr)
                    {
                        auto result = MapNotFound(builder);
                        return text_cache_response(http::status::not_found, result, result.size(), Cache::NO_CACHE);
                    }
                    auto rp = search->GetRandomPosition(randomize_spawn_points_);
                    model::Dog dog(user_name, rp.second, rp.first);
                    std::shared_ptr<model::GameSession> session = sm_.FindSession(search, 0);
                    session->AddDog(dog);
                    auto& player = players_.Addplayer(dog, session);
                    auto result = AuthRequest(builder, *player.GetAuthToken(), player.GetDog().GetId());
                    return text_cache_response(http::status::ok, result, result.size(), Cache::NO_CACHE);
                }
                catch (const boost::system::system_error& ex) // failed json parse
                {
                    auto result = BadRequest(INVALID_ARGUEMENT, JOIN_GAME_PARSE_ERROR);
                    return text_cache_response(http::status::bad_request, result, result.size(), Cache::NO_CACHE);
                }
            }

            auto result = InvalidMethod(builder, POST);
            return text_invalid_cache_response(http::status::method_not_allowed, result, result.size(), Allow::POST, Cache::NO_CACHE);
        }
        else if (str_form == GET_PLAYERS_WITH_SLASH || str_form == GET_PLAYERS_WITHOUT_SLASH)
        {
            if (req.method() == http::verb::get || req.method() == http::verb::head)
            {
                std::string auth_token = "";
                for (auto& field : req.base())
                {
                    std::ostringstream temp; temp << field.name();
                    if (temp.str() == "Authorization")
                    {
                        auth_token = field.value();
                        try {
                            auth_token = auth_token.substr(7, auth_token.size() - 7);
                        }
                        catch (const std::out_of_range& ex)
                        {
                            auth_token = "";
                        }
                        break;
                    }
                }
                if (auth_token == "" || !IsValid(auth_token))
                {
                    auto result = AuthFailed(builder);
                    return text_cache_response(http::status::unauthorized, result, result.size(), Cache::NO_CACHE);
                }
                try {
                    app::Token tagged_token{ auth_token };
                    auto player = players_.FindByToken(tagged_token);
                    auto result = GetPlayers(builder, player.GetSession().GetDogs());
                    return text_cache_response(http::status::ok, result, result.size(), Cache::NO_CACHE);
                }
                catch (const std::logic_error& ex)
                {
                    auto result = PlayerNotFound(builder);
                    return text_cache_response(http::status::unauthorized, result, result.size(), Cache::NO_CACHE);
                }
            }

            auto result = InvalidMethod(builder, GET_HEAD);
            return text_invalid_cache_response(http::status::method_not_allowed, result, result.size(), Allow::GET_HEAD, Cache::NO_CACHE);
        }
        else if (str_form == GAME_STATE_WITH_SLASH || str_form == GAME_STATE_WITHOUT_SLASH)
        {
            if (req.method() == http::verb::get || req.method() == http::verb::head)
            {
                std::string auth_token = "";
                for (auto& field : req.base())
                {
                    std::ostringstream temp; temp << field.name();
                    if (temp.str() == "Authorization")
                    {
                        auth_token = field.value();
                        try {
                            auth_token = auth_token.substr(7, auth_token.size() - 7);
                        }
                        catch (const std::out_of_range& ex)
                        {
                            auth_token = "";
                        }
                        break;
                    }
                }
                if (auth_token == "" || !IsValid(auth_token))
                {
                    auto result = AuthFailed(builder);
                    return text_cache_response(http::status::unauthorized, result, result.size(), Cache::NO_CACHE);
                }
                try {
                    app::Token tagged_token{ auth_token };
                    auto player = players_.FindByToken(tagged_token);
                    auto session = player.GetSession();    
                    auto result = GameState(builder, session.GetDogs(), session.GetLostObjects());
                    return text_cache_response(http::status::ok, result, result.size(), Cache::NO_CACHE);
                }
                catch (const std::logic_error& ex)
                {
                    auto result = PlayerNotFound(builder);
                    return text_cache_response(http::status::unauthorized, result, result.size(), Cache::NO_CACHE);
                }
            }

            auto result = InvalidMethod(builder, GET_HEAD);
            return text_invalid_cache_response(http::status::method_not_allowed, result, result.size(), Allow::GET_HEAD, Cache::NO_CACHE);
        }
        else if (str_form == ACTION_WITH_SLASH || str_form == ACTION_WITHOUT_SLASH)
        {
            if (req.method() == http::verb::post)
            {
                std::string auth_token = "";
                for (auto& field : req.base())
                {
                    std::ostringstream temp; temp << field.name();
                    if (temp.str() == "Authorization")
                    {
                        auth_token = field.value();
                        try {
                            auth_token = auth_token.substr(7, auth_token.size() - 7);
                        }
                        catch (const std::out_of_range& ex)
                        {
                            auth_token = "";
                        }
                        break;
                    }
                }
                if (auth_token == "" || !IsValid(auth_token))
                {
                    auto result = AuthFailed(builder);
                    return text_cache_response(http::status::unauthorized, result, result.size(), Cache::NO_CACHE);
                }
                try {
                    app::Token tagged_token{ auth_token };
                    auto player = players_.FindByToken(tagged_token);
                    try {
                        std::string content_type = "";
                        for (auto& field : req.base())
                        {
                            std::ostringstream temp; temp << field.name();
                            if (temp.str() == "Content-Type")
                            {
                                content_type = field.value();
                                break;
                            }
                        }
                        if (content_type != "application/json")
                        {
                            throw std::invalid_argument(BAD_REQUEST_MESSAGE);
                        }

                        namespace js = boost::json;
                        auto req_body = req.body();
                        auto value = js::parse(req_body).as_object();
                        auto move_dir = static_cast<std::string>(value.at(MOVE).as_string());
                        player.SetDir(move_dir);
                        player.SetSpeed(move_dir, game_.GetDefaultDogSpeed());
                        auto result = MoveRequestOrTimeTickRequest(builder);
                        return text_cache_response(http::status::ok, result, result.size(), Cache::NO_CACHE);
                    }
                    catch (const boost::system::system_error& ex) // failed json parse
                    {
                        auto result = BadRequest(INVALID_ARGUEMENT, GAME_ACTION_PARSE_ERROR_OR_CONTENT_TYPE_ERROR);
                        return text_cache_response(http::status::bad_request, result, result.size(), Cache::NO_CACHE);
                    }
                }
                catch (const std::logic_error& ex)
                {
                    auto result = PlayerNotFound(builder);
                    return text_cache_response(http::status::unauthorized, result, result.size(), Cache::NO_CACHE);
                }
            }

            auto result = InvalidMethod(builder, POST);
            return text_invalid_cache_response(http::status::method_not_allowed, result, result.size(), Allow::POST, Cache::NO_CACHE);
        }
        else if (str_form == GAME_TICK_WITH_SLASH || str_form == GAME_TICK_WITHOUT_SLASH)
        {
            if (tick_period_)
            {
                auto result = BadRequest(BAD_REQUEST_CODE, INVALID_ENDPOINT);
                return text_cache_response(http::status::bad_request, result, result.size(), Cache::NO_CACHE);
            }
            if (req.method() == http::verb::post)
            {
                try {
                    std::string content_type = "";
                    for (auto& field : req.base())
                    {
                        std::ostringstream temp; temp << field.name();
                        if (temp.str() == "Content-Type")
                        {
                            content_type = field.value();
                            break;
                        }
                    }
                    if (content_type != "application/json")
                    {
                        throw std::invalid_argument(BAD_REQUEST_MESSAGE);
                    }

                    namespace js = boost::json;
                    auto req_body = req.body();
                    auto value = js::parse(req_body).as_object();
                    auto time = value.at(TIME_DELTA).as_int64();
                    sm_.UpdateAllSessions(time, lg_);
                    players_.SyncronizeSession();
                    TrySaveRecordsAndRetirePlayers();
                    listener_->OnTick(time, sm_, players_);
                    auto result = MoveRequestOrTimeTickRequest(builder);
                    return text_cache_response(http::status::ok, result, result.size(), Cache::NO_CACHE);
                }
                catch (...) // std::invalid_argument (bad request) or boost::system::system_error (failed json parse)
                {
                    auto result = BadRequest(INVALID_ARGUEMENT, GAME_TICK_PARSE_ERROR_OR_CONTENT_TYPE_ERROR);
                    return text_cache_response(http::status::bad_request, result, result.size(), Cache::NO_CACHE);
                }
            }

            auto result = InvalidMethod(builder, POST);
            return text_invalid_cache_response(http::status::method_not_allowed, result, result.size(), Allow::POST, Cache::NO_CACHE);
        }
        else if (str_form == "api/v1/game/records" || str_form == "api/v1/game/records/")
        {
            std::string content_type = "";
            for (auto& field : req.base())
            {
                std::ostringstream temp; temp << field.name();
                if (temp.str() == "Content-Type")
                {
                    content_type = field.value();
                    break;
                }
            }
            if (content_type != "application/json")
            {
                auto result = BadRequest(INVALID_ARGUEMENT, "Invalid content type");
                return text_cache_response(http::status::bad_request, result, result.size(), Cache::NO_CACHE);
            }
            else
            {
                std::vector<PlayerAndScore> scores;

                auto conn = cp_.GetConnection();
                pqxx::read_transaction tx{ *conn };

                for (auto [name, score, time] : tx.query<std::string, int, int>("SELECT * FROM retired_players;"))
                    scores.emplace_back(name, score, time);

                std::sort(scores.begin(), scores.end(), [](const PlayerAndScore& lhs, const PlayerAndScore& rhs) {
                        if (lhs.score == rhs.score)
                        {
                            if (lhs.time == rhs.time)
                            {
                                return lhs.name < rhs.name;
                            }
                            else
                            {
                                return lhs.time < rhs.time;
                            }
                        }
                        else
                        {
                            return lhs.score > rhs.score;
                        }
                });

                auto result = ScoresRequest(builder, scores);
                return text_cache_response(http::status::ok, result, result.size(), Cache::NO_CACHE);
            }
        }
        else
        {
            auto result = BadRequest();
            return text_response(http::status::bad_request, result, result.size());
        }
        //                                                   ,                        : Hello
    }
    
    std::string ApiHandler::ScoresRequest(json::Builder& builder, const std::vector<PlayerAndScore>& scores) {
        builder.StartDict();
        for (const auto& line : scores)
        {
            builder.StartDict().Key("name").Value(line.name).Key("score").Value(line.score).Key("playTime").Value(line.time).EndDict();
        }
        return json::Print(builder.EndDict().Build());
    }

    void ApiHandler::TrySaveRecordsAndRetirePlayers() {
        auto conn = cp_.GetConnection();
        pqxx::work w{ *conn };
        for (auto erased_dog : players_.EraseRetiredPlayers(game_.GetRetTime()))
        {
            w.exec_prepared("insert_player", erased_dog.GetName(), erased_dog.GetScore(), erased_dog.GetTime());
        }
        w.commit();
    }

    void ApiHandler::Tick(std::uint64_t time) {
        sm_.UpdateAllSessions(time, lg_);
        players_.SyncronizeSession();
        TrySaveRecordsAndRetirePlayers();
        listener_->OnTick(time, sm_, players_);
    }

    std::string ApiHandler::urlDecode(const std::string& SRC) const {
        std::string res;
        char ch;
        int string_index, symbol_code;
        for (string_index = 0; string_index < SRC.length(); string_index++) {
            if (SRC[string_index] == '%') {
                std::istringstream strm(SRC.substr(string_index + 1, 2)); strm >> std::hex >> symbol_code;
                ch = static_cast<char>(symbol_code);
                res += ch;
                string_index = string_index + 2;
            }
            else {
                res += SRC[string_index];
            }
        }
        return (res);
    }
}  // namespace http_handler