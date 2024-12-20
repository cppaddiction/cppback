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

#include <boost/program_options.hpp>

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;
namespace logging = boost::log;
namespace sys = boost::system;
namespace http = boost::beast::http;

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
        : strand_{ strand }
        , period_{ period }
        , handler_{ std::move(handler) } {
    }

    void Start() {
        net::dispatch(strand_, [self = shared_from_this()] {
            self->last_tick_ = Clock::now();
            self->ScheduleTick();
            });
    }

private:
    void ScheduleTick() {
        assert(strand_.running_in_this_thread());
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
            });
    }

    void OnTick(sys::error_code ec) {
        using namespace std::chrono;
        assert(strand_.running_in_this_thread());

        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            try {
                handler_(delta);
            }
            catch (...) {
            }
            ScheduleTick();
        }
    }

    using Clock = std::chrono::steady_clock;

    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{ strand_ };
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
};

struct Args {
    std::uint64_t tick_period = 0;
    std::string config_path;
    std::string static_path;
    bool randomize_spawn_points = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{ "Allowed options"s };

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_path)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.static_path)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        // Выводим описание параметров программы
        std::cout << desc;
        return std::nullopt;
    }

    // Проверяем наличие опций config-file и www-root
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path is not specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static dir path is not specified"s);
    }
    if (vm.contains("randomize-spawn-points"s)) {
        args.randomize_spawn_points = true;
    }
    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    strm << *rec[additional_data];
}

namespace {
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
    /*
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static>"sv << std::endl;
        return EXIT_FAILURE;
    }
    */
    try {
        if (auto args = ParseCommandLine(argc, argv)) {
            http_server::InitLogging(&MyFormatter);
            // 1. Загружаем карту из файла и построить модель игры
            model::Game game = json_loader::LoadGame((*args).config_path);
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
                    return EXIT_SUCCESS;
                }
                });

            auto api_handler = http_handler::ApiHandler{ game, players, sm, (*args).tick_period, (*args).randomize_spawn_points };
            // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
            auto handler = std::make_shared<http_handler::RequestHandler>(game, (*args).static_path, api_strand, api_handler);
            // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов

            auto ticker = std::make_shared<Ticker>(api_strand, std::chrono::milliseconds((*args).tick_period),
                [&api_handler](std::chrono::milliseconds delta) {
                api_handler.Tick(delta.count());
            }
            );

            if ((*args).tick_period)
            {
                ticker->Start();
            }
            
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
            boost::json::value custom_data{ {"message", "server started"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"port", 8080}, {"address", "0.0.0.0"}}} };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
            // 6. Запускаем обработку асинхронных операций
            RunWorkers(num_threads, [&ioc] {
                ioc.run();
                });
        }
        return EXIT_SUCCESS;
    }
    catch (const std::exception& ex) {
        std::ostringstream temp; temp << ex.what();
        auto tick = boost::posix_time::microsec_clock::local_time();
        boost::json::value custom_data{ {"message", "server exited"}, {"timestamp", to_iso_extended_string(tick)}, {"data", boost::json::value{{"code", EXIT_FAILURE}, {"exception", temp.str()}}}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data);
        return EXIT_FAILURE;
    }
}