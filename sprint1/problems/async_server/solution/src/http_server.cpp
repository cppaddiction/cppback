#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

    void SessionBase::Run() {
        // �������� ����� Read, ��������� executor ������� stream_.
        // ����� ������� ��� ������ �� stream_ ����� �����������, ��������� ��� executor
        net::dispatch(stream_.get_executor(),
            beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    template <typename RequestHandler>
    void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
        // ��� ������ decay_t �������� ������ �� ���� RequestHandler,
        // ����� Listener ������ RequestHandler �� ��������
        using MyListener = Listener<std::decay_t<RequestHandler>>;

        std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
    }

} // namespace http_server
