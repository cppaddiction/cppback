#include "sdk.h"
#include "json.h"
#include "json_builder.h"
//
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;

namespace {
namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

// Структура ContentType задаёт область видимости для констант,
// задающий значения HTTP-заголовка Content-Type
struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

struct Allow {
    Allow() = delete;
    constexpr static std::string_view GET_HEAD = "GET, HEAD"sv;
    // При необходимости внутрь Allow можно добавить и другие типы контента
};


// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, int x, unsigned http_version,
    bool keep_alive,
    std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(x);
    response.keep_alive(keep_alive);
    return response;
}

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringInvalidResponse(http::status status, std::string_view body, int x, unsigned http_version,
    bool keep_alive,
    std::string_view content_type = ContentType::TEXT_HTML, std::string_view allow = Allow::GET_HEAD) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.set(http::field::allow, allow);
    response.body() = body;
    response.content_length(x);
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& req, const model::Game& gm) {
    const auto text_response = [&req](http::status status, std::string_view text, int x) {
        return MakeStringResponse(status, text, x, req.version(), req.keep_alive());
        };
    const auto text_invalid_response = [&req](http::status status, std::string_view text, int x) {
        return MakeStringInvalidResponse(status, text, x, req.version(), req.keep_alive());
        };

    json::Builder builder;

    if (req.method() == http::verb::get)
    {
        auto str_form = static_cast<std::string>(req.target());
        auto last_zero = str_form.find_first_of('0');
        if (str_form.substr(last_zero + 2) == "api/v1/maps" || str_form.substr(last_zero + 2) == "api/v1/maps/")
        {
            builder.StartArray();
            const auto& maps = gm.GetMaps();
            for (const auto& m : maps)
            {
                builder.StartDict().Key("id").Value(*(m.GetId())).Key("name").Value(m.GetName()).EndDict();
            }
            auto result = json::Print(builder.EndArray().Build());
            return text_response(http::status::ok, result, result.size());
        }
        else if (str_form.substr(last_zero + 2, 12) == "api/v1/maps/" && str_form.substr(last_zero + 2) != "api/v1/maps/")
        {
            auto id = str_form.substr(last_zero + 14);
            if (id[id.size() - 1] == '/')
            {
                id = id.substr(0, id.size() - 1);
            }
            if (auto m = gm.FindMap(model::Map::Id(id)))
            {
                builder.StartDict().Key("id").Value(*(m->GetId())).Key("name").Value(m->GetName()).Key("roads");
                json::Builder helpbuilder;
                helpbuilder.StartArray();
                const auto& roads = m->GetRoads();
                for (const auto& r : roads)
                {
                    auto start = r.GetStart();
                    auto end = r.GetEnd();
                    if (!r.IsHorizontal())
                    {
                        helpbuilder.StartDict().Key("x0").Value(start.x).Key("y0").Value(start.y).Key("x1").Value(end.x).EndDict();
                    }
                    else
                    {
                        helpbuilder.StartDict().Key("x0").Value(start.x).Key("y0").Value(start.y).Key("y1").Value(end.y).EndDict();
                    }
                }
                builder.Value(helpbuilder.EndArray().Build().AsArray()).Key("buildings");
                json::Builder helpbuilderr;
                helpbuilderr.StartArray();
                const auto& buildings = m->GetBuildings();
                for (const auto& b : buildings)
                {
                    auto rect = b.GetBounds();
                    helpbuilderr.StartDict().Key("x").Value(rect.position.x).Key("y").Value(rect.position.y).Key("w").Value(rect.size.width).Key("h").Value(rect.size.height).EndDict();
                }
                builder.Value(helpbuilderr.EndArray().Build().AsArray()).Key("offices");
                json::Builder helpbuilderrr;
                helpbuilderrr.StartArray();
                const auto& offices = m->GetOffices();
                for (const auto& o : offices)
                {
                    auto pos = o.GetPosition();
                    auto offset = o.GetOffset();
                    helpbuilderrr.StartDict().Key("id").Value(*(o.GetId())).Key("x").Value(pos.x).Key("y").Value(pos.y).Key("offsetX").Value(offset.dx).Key("offsetY").Value(offset.dy).EndDict();
                }
                builder.Value(helpbuilderrr.EndArray().Build().AsArray());
                auto result = json::Print(builder.EndDict().Build());
                return text_response(http::status::ok, result, result.size());
            }
            else
            {
                builder.StartDict().Key("code").Value("mapNotFound").Key("message").Value("Map not found");
                auto result = json::Print(builder.EndDict().Build());
                return text_response(http::status::not_found, result, result.size());
            }
        }
        else
        {
            builder.StartDict().Key("code").Value("badRequest").Key("message").Value("Bad request");
            auto result = json::Print(builder.EndDict().Build());
            return text_response(http::status::bad_request, result, result.size());
        }
    }
    else
    {
        return text_invalid_response(http::status::method_not_allowed, "Invalid method"sv, 14);
    }
    // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
}


// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
            });
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{ game };
        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, { address, port }, [game](auto&& req, auto&& sender) {
            sender(HandleRequest(std::forward<decltype(req)>(req), game));
            });
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        std::cout << "Server has started..."sv << std::endl;
        // 6. Запускаем обработку асинхронных операций
        RunWorkers(num_threads, [&ioc] {
            ioc.run();
            });
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
