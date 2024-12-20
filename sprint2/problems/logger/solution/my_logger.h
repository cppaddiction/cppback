#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

using namespace std::literals;

class Logger {
    std::chrono::system_clock::time_point GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        std::ostringstream temp;
        temp << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return "/var/log/sample_log_" + temp.str() + ".log";
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... vs)
    {
        std::lock_guard<std::mutex> lock(m1_);
        log_file_ << GetTimeStamp() << ": "sv;
        (log_file_ << ... << vs);
        log_file_ << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts)
    {
        std::lock_guard<std::mutex> lock(m2_);
        manual_ts_ = ts;
        if (file_ != GetFileTimeStamp())
        {
            file_ = GetFileTimeStamp();
            if(log_file_.is_open())
                log_file_.close();
	    log_file_.open(file_);
        }
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::mutex m1_;
    std::mutex m2_;
    std::ofstream log_file_;
    std::string file_;
};
