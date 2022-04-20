#include <redis/basic_stream.hpp>

namespace redis
{
basic_stream::basic_stream(boost::asio::io_context& ioc)
    : stream_(ioc)
    , is_closed_(false)
{
}

auto basic_stream::get_executor() -> asio_stream::executor_type
{
  return stream_.get_executor();
}

void basic_stream::connect(const std::string& hostport)
{
  boost::system::error_code ec;

  connect(hostport, ec);
  if (ec)
    throw ec;
}

void basic_stream::connect(const std::string& hostport,
                           boost::system::error_code& ec)
{
  std::vector<std::string> v{2};
  boost::split(v, hostport, boost::is_any_of(":"));
  connect(v[0], v[1], ec);
}

void basic_stream::connect(const std::string& host, const std::string& port)
{
  boost::system::error_code ec;

  connect(host, port, ec);
  if (ec)
    throw ec;
}

void basic_stream::connect(const std::string& host, const std::string& port,
                           boost::system::error_code& ec)
{
  boost::asio::ip::tcp::resolver resolver(stream_.get_executor());
  auto const results = resolver.resolve(host, port, ec);
  if (ec)
    return;

  if (stream_.is_open())
    close();

  original_host_ = host;
  original_port_ = port;

  boost::asio::connect(stream_, results.begin(), results.end(), ec);
  if (ec)
    return;

  is_closed_ = false;
  stream_.non_blocking(true);
}

void basic_stream::async_connect(const std::string& hostport, on_connect_cb cb)
{
  std::vector<std::string> v{2};
  boost::split(v, hostport, boost::is_any_of(":"));
  async_connect(v[0], v[1], cb);
}

void basic_stream::async_connect(const std::string& host,
                                 const std::string& port, on_connect_cb cb)
{
  if (stream_.is_open())
    close();

  auto resolver =
      std::make_shared<boost::asio::ip::tcp::resolver>(stream_.get_executor());

  original_host_ = host;
  original_port_ = port;

  resolver->async_resolve(original_host_, original_port_,
                          [this, resolver, cb](auto&& ec, auto&& results)
                          {
                            if (ec)
                            {
                              return cb(ec);
                            }

                            boost::asio::async_connect(
                                stream_, results,
                                [this, cb](auto&& ec, auto&& _)
                                {
                                  is_closed_ = false;

                                  stream_.non_blocking(true);

                                  cb(ec);
                                });
                          });
}

void basic_stream::reconnect_report(boost::system::error_code ec)
{
  stream_.close();

  if (on_stream_closed_cb_)
    on_stream_closed_cb_(ec);

  reconnect();
}

void basic_stream::reconnect()
{
  if (is_closed_)
    return;

  async_connect(original_host_, original_port_,
                [this](auto&& ec)
                {
                  if (ec)
                  {
                    // ... retry
                    auto timer = std::make_shared<boost::asio::deadline_timer>(
                        stream_.get_executor());
                    timer->expires_from_now(boost::posix_time::seconds(1));
                    timer->async_wait([this, timer](auto&& _) { reconnect(); });
                  }
                  else
                  {
                    if (on_reconnect_cb_)
                      on_reconnect_cb_();
                  }
                });
}
}  // namespace redis
