#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

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
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
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

StringResponse HandleRequest(StringRequest&& req) {
    const auto text_response = [&req](http::status status, std::string_view text, int x) {
        return MakeStringResponse(status, text, x, req.version(), req.keep_alive());
        };
    const auto text_invalid_response = [&req](http::status status, std::string_view text, int x) {
        return MakeStringInvalidResponse(status, text, x, req.version(), req.keep_alive());
        };
    if (req.method() == http::verb::get || req.method() == http::verb::head)
    {
        auto str_form = static_cast<std::string>(req.target());
        auto last_slash = str_form.find_last_of('/');
        std::string_view resres(std::string("Hello, ") + str_form.substr(last_slash + 1));
        int x = resres.size();
        if (req.method() == http::verb::get)
        {
            return text_response(http::status::ok, resres, x);
        }
        else
        {
            return text_response(http::status::ok, ""sv, x);
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

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, {address, port}, [](auto&& req, auto&& sender) {
        sender(HandleRequest(std::forward<decltype(req)>(req)));
    });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
}
