#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <syncstream>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
namespace sys = boost::system;
namespace ph = std::placeholders;
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;

class ThreadChecker {
public:
    explicit ThreadChecker(std::atomic_int& counter)
        : counter_{ counter } {
    }

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Order : public std::enable_shared_from_this<Order> {
    constexpr static Clock::duration BreadDuration = HotDog::MIN_BREAD_COOK_DURATION + Milliseconds{ 1 };
    constexpr static Clock::duration SausageDuration = HotDog::MIN_SAUSAGE_COOK_DURATION + Milliseconds{ 1 };

public:
    Order(net::io_context& io, int id, std::shared_ptr<Bread> b, std::shared_ptr<Sausage> s, std::shared_ptr<GasCooker> gc, HotDogHandler handler)
        : io_{ io }
        , id_{ id }
        , bread_{ std::move(b) }
        , sausage_{ std::move(s) }
        , gas_cooker_{std::move(gc) }
        , handler_{ std::move(handler) }{
    }

    // Запускает асинхронное выполнение заказа
    void Execute() {
        BakeBread();
        FrySausage();
    }
private:
    void BakeBread() {
        bread_->StartBake(*gas_cooker_, net::bind_executor(strand_, [self = shared_from_this()]() {
            self->bread_timer_.expires_after(BreadDuration);
            self->bread_timer_.async_wait([self](sys::error_code ec) {
                self->OnBaked(ec);
                });
            }));
    }

    void OnBaked(sys::error_code ec) {
        ThreadChecker checker{ counter_ };
        bread_->StopBaking();
        bread_baked_ = true;
        CheckReadiness(ec);
    }

    void FrySausage() {
        sausage_->StartFry(*gas_cooker_, net::bind_executor(strand_, [self = shared_from_this()]() {
            self->sausage_timer_.expires_after(SausageDuration);
            self->sausage_timer_.async_wait([self](sys::error_code ec) {
                self->OnFried(ec);
                });
            }));
    }

    void OnFried(sys::error_code ec) {
        ThreadChecker checker{ counter_ };
        sausage_->StopFry();
        sausage_fried_ = true;
        CheckReadiness(ec);
    }

    void CheckReadiness(sys::error_code ec) {
        // Если все компоненты хот-дога готовы, собираем его
        if (sausage_fried_ && bread_baked_) {
            HotDog hot_dog(id_, sausage_, bread_);
            handler_(Result(std::move(hot_dog)));
        }
    }

    net::io_context& io_;
    int id_;
    std::shared_ptr<Bread>     bread_;
    std::shared_ptr<Sausage>   sausage_;
    std::shared_ptr<GasCooker> gas_cooker_;
    HotDogHandler handler_;
    bool bread_baked_ = false;
    bool sausage_fried_ = false;
    Timer bread_timer_{ io_, BreadDuration };
    Timer sausage_timer_{ io_, SausageDuration};
    net::strand<net::io_context::executor_type> strand_{ net::make_strand(io_) };
    std::atomic_int counter_{ 0 };
};


// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        m.lock();
        // TODO: Реализуйте метод самостоятельно
        // При необходимости реализуйте дополнительные классы
        std::make_shared<Order>(io_, next_order_id_++, store_.GetBread(), store_.GetSausage(), gas_cooker_, std::move(handler))->Execute();
        m.unlock();
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    int next_order_id_ = 0;
    std::mutex m;
};
