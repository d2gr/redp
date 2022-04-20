#include <redis/stream.hpp>

namespace redis
{
stream::stream(boost::asio::io_context& ioc)
    : stream_(ioc)
    , is_sending_(false)
{
}

auto stream::get_executor() -> basic_stream::asio_stream::executor_type
{
  return stream_.get_executor();
}

void stream::connect(const std::string& hostport)
{
  stream_.connect(hostport);
}

void stream::connect(const std::string& hostport, boost::system::error_code& ec) noexcept
{
  stream_.connect(hostport, ec);
}

void stream::connect(const std::string& host, const std::string& port)
{
  stream_.connect(host, port);
}

void stream::connect(const std::string& host, const std::string& port,
                     boost::system::error_code& ec) noexcept
{
  stream_.connect(host, port, ec);
}

void stream::async_connect(const std::string& hostport,
                           basic_stream::on_connect_cb cb) noexcept
{
  stream_.async_connect(hostport, cb);
}

void stream::async_connect(const std::string& host, const std::string& port,
                           basic_stream::on_connect_cb cb) noexcept
{
  stream_.async_connect(host, port, cb);
}

}  // namespace redis
