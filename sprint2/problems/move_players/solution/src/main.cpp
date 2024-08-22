#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <mutex>
#include <thread>
#include <vector>

#include "json_loader.h"
#include "request_handler.h"

#include "app.h"

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;
namespace logging = boost::log;

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    strm << *rec[additional_data];
}

namespace {

namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

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
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static>"sv << std::endl;
        return EXIT_FAILURE;
    }
    http_server::InitLogging(&MyFormatter);
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);
        app::Players players;
        model::SessionManager sm(game.GetMaps());
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        auto api_strand = net::make_strand(ioc);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                auto tick = boost::posix_time::microsec_clock::local_time();
                boost::json::value custom_data{ {"message", "server exited"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"code", 0}}} };
                BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
                return 0;
            }
            });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>( game, argv[2], api_strand, players, sm );
        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_handler::LoggingRequestHandler handler_cover([handler](auto&& req, auto&& send) {
            // Обрабатываем запрос
            (*handler)(
            std::forward<decltype(req)>(req),
            std::forward<decltype(send)>(send));
            });
        http_server::ServeHttp(ioc, { address, port }, [&handler_cover](auto&& req, auto&& sender, std::string client_ip) {
            handler_cover(std::forward<decltype(req)>(req), std::forward<decltype(sender)>(sender), client_ip);
            });
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        //std::cout << "Server has started..."sv << std::endl;
        auto tick = boost::posix_time::microsec_clock::local_time();
        boost::json::value custom_data{ {"message", "server started"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"port", 8080}, {"address", "0.0.0.0"}}}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
        // 6. Запускаем обработку асинхронных операций
        RunWorkers(num_threads, [&ioc] {
            ioc.run();
            });
    }
    catch (const std::exception& ex) {
        std::ostringstream temp; temp << ex.what();
        auto tick = boost::posix_time::microsec_clock::local_time();
        boost::json::value custom_data{ {"message", "server exited"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"code", EXIT_FAILURE}, {"exception", temp.str()}}}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
        //std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}