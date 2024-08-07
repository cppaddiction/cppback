#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {
    void ReadFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
	std::ostringstream mssg;
	mssg << rec[logging::expressions::smessage];
	json::Builder helpbuilder;
	helpbuilder.StartDict().Key("message").Value(mssg.str()).Key("timestamp").Value(to_iso_extended_string(*rec[timestamp])).Key("data");
	json::Builder help_helpbuilder;
	json::Dict data_obj = help_helpbuilder.StartDict().Key("code").Value(*rec[error_code]).Key("text").Value(*rec[exception]).Key("where").Value("read").EndDict().Build().AsMap();
	strm<<json::Print(helpbuilder.Value(data_obj).EndDict().Build());
}


    void WriteFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
	std::ostringstream mssg;
	mssg << rec[logging::expressions::smessage];
	json::Builder helpbuilder;
	helpbuilder.StartDict().Key("message").Value(mssg.str()).Key("timestamp").Value(to_iso_extended_string(*rec[timestamp])).Key("data");
	json::Builder help_helpbuilder;
	json::Dict data_obj = help_helpbuilder.StartDict().Key("code").Value(*rec[error_code]).Key("text").Value(*rec[exception]).Key("where").Value("write").EndDict().Build().AsMap();
	strm<<json::Print(helpbuilder.Value(data_obj).EndDict().Build());
}


    void AcceptFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
	std::ostringstream mssg;
	mssg << rec[logging::expressions::smessage];
	json::Builder helpbuilder;
	helpbuilder.StartDict().Key("message").Value(mssg.str()).Key("timestamp").Value(to_iso_extended_string(*rec[timestamp])).Key("data");
	json::Builder help_helpbuilder;
	json::Dict data_obj = help_helpbuilder.StartDict().Key("code").Value(*rec[error_code]).Key("text").Value(*rec[exception]).Key("where").Value("accept").EndDict().Build().AsMap();
	strm<<json::Print(helpbuilder.Value(data_obj).EndDict().Build());
}

    void InitLogging(void (*foo) (logging::record_view const &, logging::formatting_ostream &))
    {
	logging::core::get()->flush();
    	logging::core::get()->remove_all_sinks();
	logging::add_common_attributes();
	logging::add_console_log(std::cout, logging::keywords::format=foo, logging::keywords::auto_flush=true);
    }

    void ReportError(beast::error_code ec, std::string_view what)
    {
        using namespace std::literals;
        std::cerr << what << ": "sv << ec.message() << std::endl;
    }
    void SessionBase::Run() {
        // �������� ����� Read, ��������� executor ������� stream_.
        // ����� ������� ��� ������ �� stream_ ����� �����������, ��������� ��� executor
        net::dispatch(stream_.get_executor(),
            beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }
    void SessionBase::Read() {
        using namespace std::literals;
        // ������� ������ �� �������� �������� (����� Read ����� ���� ������ ��������� ���)
        request_ = {};
        stream_.expires_after(30s);
        // ��������� request_ �� stream_, ��������� buffer_ ��� �������� ��������� ������
        http::async_read(stream_, buffer_, request_,
            // �� ��������� �������� ����� ������ ����� OnRead
            beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    }
    void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        using namespace std::literals;
        if (ec == http::error::end_of_stream) {
            // ���������� �������� - ������ ������ ����������
            return Close();
        }
        if (ec) {
	    InitLogging(&ReadFormatter);
	    std::ostringstream temp; temp<<ec.message();
	    BOOST_LOG_TRIVIAL(info)<<logging::add_value(error_code, ec.value())<<logging::add_value(exception, temp.str())<<"error"sv;
	    return;
	    //return ReportError(ec, "read"sv);
        }
        HandleRequest(std::move(request_));
    }
    void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        using namespace std::literals;
        if (ec) {
	    InitLogging(&WriteFormatter);
	    std::ostringstream temp; temp<<ec.message();
	    BOOST_LOG_TRIVIAL(info)<<logging::add_value(error_code, ec.value())<<logging::add_value(exception, temp.str())<<"error"sv;
            return;
	    //return ReportError(ec, "write"sv);
        }

        if (close) {
            // ��������� ������ ������� ������� ����������
            return Close();
        }

        // ��������� ��������� ������
        Read();
    }
    void SessionBase::Close() {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }
} // namespace http_server

