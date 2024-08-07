#include "request_handler.h"

namespace http_handler {
    void RequestFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
		std::ostringstream mssg;
		mssg << rec[logging::expressions::smessage];
		json::Builder helpbuilder;
		helpbuilder.StartDict().Key("message").Value(mssg.str()).Key("timestamp").Value(to_iso_extended_string(*rec[timestamp])).Key("data");
		json::Builder help_helpbuilder;
		json::Dict data_obj = help_helpbuilder.StartDict().Key("ip").Value(*rec[ip]).Key("URI").Value(*rec[target]).Key("method").Value(*rec[method]).EndDict().Build().AsMap();
		strm<<json::Print(helpbuilder.Value(data_obj).EndDict().Build());
    }
   
   void ResponseFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
	std::ostringstream mssg;
	mssg << rec[logging::expressions::smessage];
	json::Builder helpbuilder;
	helpbuilder.StartDict().Key("message").Value(mssg.str()).Key("timestamp").Value(to_iso_extended_string(*rec[timestamp])).Key("data");
	json::Builder help_helpbuilder;
	json::Dict data_obj = help_helpbuilder.StartDict().Key("response_time").Value(*rec[response_time]).Key("code").Value(*rec[response_code]).Key("content_type").Value(*rec[content_type]).EndDict().Build().AsMap();
	strm<<json::Print(helpbuilder.Value(data_obj).EndDict().Build());
    }
 
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
        std::string_view content_type, std::string_view allow) const
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
        // ����� prepare_payload ��������� ��������� Content-Length � Transfer-Encoding
        // � ����������� �� ������� ���� ���������
        res.prepare_payload();
        r = std::move(res);
        return r;
    }

    bool RequestHandler::IsSubPath(fs::path path, fs::path base) const {
        // �������� ��� ���� � ����������� ���� (��� . � ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        // ���������, ��� ��� ���������� base ���������� ������ path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

    std::string RequestHandler::GetMaps(json::Builder& builder, const model::Game& gm) const
    {
        builder.StartArray();
        const auto& maps = gm.GetMaps();
        for (const auto& m : maps)
        {
            builder.StartDict().Key(ID).Value(*(m.GetId())).Key(NAME).Value(m.GetName()).EndDict();
        }
        return json::Print(builder.EndArray().Build());
    }

    std::string RequestHandler::GetMapWithSpecificId(json::Builder& builder, const model::Map* m) const
    {
        builder.StartDict().Key(ID).Value(*(m->GetId())).Key(NAME).Value(m->GetName()).Key(ROADS);
        builder.Value(CollectRoads(m)).Key(BUILDINGS);
        builder.Value(CollectBuildings(m)).Key(OFFICES);
        builder.Value(CollectOffices(m));
        return json::Print(builder.EndDict().Build());
    }

    std::string RequestHandler::BadRequest(json::Builder& builder) const
    {
        builder.StartDict().Key(CODE).Value(BAD_REQUEST_CODE).Key(MESSAGE).Value(BAD_REQUEST_MESSAGE);
        return json::Print(builder.EndDict().Build());
    }

    std::string RequestHandler::MapNotFound(json::Builder& builder) const
    {
        builder.StartDict().Key(CODE).Value(MAP_NOT_FOUND_CODE).Key(MESSAGE).Value(MAP_NOT_FOUND_MESSAGE);
        return json::Print(builder.EndDict().Build());
    }

    std::string RequestHandler::FileNotFound(json::Builder& builder) const
    {
        builder.StartDict().Key(CODE).Value(FILE_NOT_FOUND_CODE).Key(MESSAGE).Value(FILE_NOT_FOUND_MESSAGE);
        return json::Print(builder.EndDict().Build());
    }

    json::Array RequestHandler::CollectRoads(const model::Map* m) const {
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

    json::Array RequestHandler::CollectBuildings(const model::Map* m) const {
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

    json::Array RequestHandler::CollectOffices(const model::Map* m) const {
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

    std::string RequestHandler::urlDecode(const std::string& SRC) const {
        std::string ret;
        char ch;
        int i, ii;
        for (i = 0; i < SRC.length(); i++) {
            if (SRC[i] == '%') {
                sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
                ch = static_cast<char>(ii);
                ret += ch;
                i = i + 2;
            }
            else {
                ret += SRC[i];
            }
        }
        return (ret);
    }

    std::variant<RequestHandler::StringResponse, RequestHandler::FileResponse> RequestHandler::HandleRequest(StringRequest&& req, const model::Game& gm) const {
        const auto text_response = [&req, this](http::status status, std::string_view text, int x, std::string_view content_type=ContentType::JSON) {
            return this->MakeStringResponse(status, text, x, req.version(), req.keep_alive(), content_type);
            };
        const auto text_invalid_response = [&req, this](http::status status, std::string_view text, int x) {
            return this->MakeStringInvalidResponse(status, text, x, req.version(), req.keep_alive());
            };
        const auto file_response = [&req, this](fs::path path) {
            return this->MakeFileResponse(path);
            };

        json::Builder builder;

        if (req.method() == http::verb::get)
        {
            auto str_form = urlDecode(static_cast<std::string>(req.target()));
            //url-decode str
            if (str_form == MAPS_PATH_WITH_SLASH || str_form == MAPS_PATH_WITHOUT_SLASH)
            {
                auto result = GetMaps(builder, gm);
                return text_response(http::status::ok, result, result.size());
            }
            else if (str_form.substr(0, 13) == MAPS_PATH_WITH_SLASH && str_form != MAPS_PATH_WITH_SLASH)
            {
                auto id = str_form.substr(13);
                if (id[id.size() - 1] == '/')
                {
                    id = id.substr(0, id.size() - 1);
                }
                if (auto m = gm.FindMap(model::Map::Id(id)))
                {
                    auto result = GetMapWithSpecificId(builder, m);
                    return text_response(http::status::ok, result, result.size());
                }
                else
                {
                    auto result = MapNotFound(builder);
                    return text_response(http::status::not_found, result, result.size());
                }
            }
            else if (str_form.substr(0, 4) == API)
            {
                auto result = BadRequest(builder);
                return text_response(http::status::bad_request, result, result.size());
            }
            else if (str_form == ROOT)
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
                try {
                    auto response_value = std::move(get<FileResponse>(response));
                    return response_value;
                }
                catch (...)
                {

                }
                try {
                    auto response_value = get<FileResponseErrors>(response);
                    if (response_value == FileResponseErrors::SUB_PATH_FAIL)
                    {
                        auto result = BadRequest(builder);
                        return text_response(http::status::bad_request, result, result.size());
                    }
                    else
                    {
                        auto result = FileNotFound(builder);
                        return text_response(http::status::not_found, result, result.size(), ContentType::TEXT);
                    }
                }
                catch (...)
                {

                }
                auto result = BadRequest(builder);
                return text_response(http::status::bad_request, result, result.size());
            }
        }
        else
        {
            return text_invalid_response(http::status::method_not_allowed, INVALID_METHOD, 14);
        }
        // ����� ����� ���������� ������ � ������������ �����, �� ���� ������ ��������: Hello
    }
}  // namespace http_handler

