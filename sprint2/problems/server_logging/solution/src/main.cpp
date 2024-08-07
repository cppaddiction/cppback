#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <mutex>
#include <thread>
#include <vector>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;
namespace js = boost::json;
namespace logging=boost::log;


void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
	std::ostringstream mssg;
	mssg << rec[logging::expressions::smessage];
	json::Builder helpbuilder;
	helpbuilder.StartDict().Key("message").Value(mssg.str()).Key("timestamp").Value(to_iso_extended_string(*rec[timestamp])).Key("data");
	json::Builder help_helpbuilder;
	json::Dict data_obj = help_helpbuilder.StartDict().Key("port").Value(8080).Key("address").Value("0.0.0.0").EndDict().Build().AsMap();
	strm<<json::Print(helpbuilder.Value(data_obj).EndDict().Build());
}


void MyExitNoExcFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
	std::ostringstream mssg;
	mssg << rec[logging::expressions::smessage];
	json::Builder helpbuilder;
	helpbuilder.StartDict().Key("message").Value(mssg.str()).Key("timestamp").Value(to_iso_extended_string(*rec[timestamp])).Key("data");
	json::Builder help_helpbuilder;
	json::Dict data_obj = help_helpbuilder.StartDict().Key("code").Value(0).EndDict().Build().AsMap();
	strm<<json::Print(helpbuilder.Value(data_obj).EndDict().Build());
}


void MyExitExcFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
	std::ostringstream mssg;
	mssg << rec[logging::expressions::smessage];
	json::Builder helpbuilder;
	helpbuilder.StartDict().Key("message").Value(mssg.str()).Key("timestamp").Value(to_iso_extended_string(*rec[timestamp])).Key("data");
	json::Builder help_helpbuilder;
	json::Dict data_obj = help_helpbuilder.StartDict().Key("code").Value(EXIT_FAILURE).Key("exception").Value(*rec[exception]).EndDict().Build().AsMap();
	strm<<json::Print(helpbuilder.Value(data_obj).EndDict().Build());
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
		//logging::core::get()->remove_all_sinks();
		//logging::add_common_attributes();
		//logging::add_console_log(std::cout, logging::keywords::format=&MyExitNoExcFormatter, logging::keywords::auto_flush=true);
		http_server::InitLogging(&MyExitNoExcFormatter);
		BOOST_LOG_TRIVIAL(info) <<"server exited"sv;
            }
            });
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{ game, argv[2] };
        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
	http_handler::LoggingRequestHandler<http_handler::RequestHandler> handler_cover(handler);
        http_server::ServeHttp(ioc, { address, port }, [&](auto&& req, auto&& sender, std::string client_ip) {
            handler_cover(std::forward<decltype(req)>(req), std::forward<decltype(sender)>(sender), client_ip);
            });
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        //std::cout << "Server has started..."sv << std::endl;
	//logging::core::get()->remove_all_sinks();
	//logging::add_common_attributes();
	//logging::add_console_log(std::cout, logging::keywords::format=&MyFormatter, logging::keywords::auto_flush=true);
	http_server::InitLogging(&MyFormatter);
	BOOST_LOG_TRIVIAL(info) <<"server started"sv;
        // 6. Запускаем обработку асинхронных операций
        RunWorkers(num_threads, [&ioc] {
            ioc.run();
            });
    }
    catch (const std::exception& ex) {
	//logging::core::get()->remove_all_sinks();
	//logging::add_common_attributes();
	//logging::add_console_log(std::cout, logging::keywords::format=&MyExitExcFormatter, logging::keywords::auto_flush=true);
	http_server::InitLogging(&MyExitExcFormatter);
	std::ostringstream temp; temp<<ex.what();
	BOOST_LOG_TRIVIAL(info) <<logging::add_value(exception, temp.str())<<"server exited"sv;
        //std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

}