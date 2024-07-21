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

} // namespace http_server
