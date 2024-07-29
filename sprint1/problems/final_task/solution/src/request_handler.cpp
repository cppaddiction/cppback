#include "request_handler.h"

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
    
    RequestHandler::StringResponse RequestHandler::HandleRequest(StringRequest&& req, const model::Game& gm) const {
        const auto text_response = [&req, this](http::status status, std::string_view text, int x) {
            return this->MakeStringResponse(status, text, x, req.version(), req.keep_alive());
            };
        const auto text_invalid_response = [&req, this](http::status status, std::string_view text, int x) {
            return this->MakeStringInvalidResponse(status, text, x, req.version(), req.keep_alive());
            };

        json::Builder builder;

        if (req.method() == http::verb::get)
        {
            auto str_form = static_cast<std::string>(req.target());
            if (str_form == MAPS_PATH_WITHOUT_SLASH || str_form == MAPS_PATH_WITH_SLASH)
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
            else
            {
                auto result = BadRequest(builder);
                return text_response(http::status::bad_request, result, result.size());
            }
        }
        else
        {
            return text_invalid_response(http::status::method_not_allowed, INVALID_METHOD, 14);
        }
        // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
    }
}  // namespace http_handler
