#ifndef REDIS_BASIC_STREAM_H
#define REDIS_BASIC_STREAM_H

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>

namespace redis
{
class basic_stream
{
public:
  using on_connect_cb       = std::function<void(boost::system::error_code)>;
  using on_stream_closed_cb = std::function<void(boost::system::error_code)>;
  using on_reconnect_cb     = std::function<void()>;

  using asio_stream = boost::asio::ip::tcp::socket;

public:
  basic_stream()               = delete;
  basic_stream(basic_stream&)  = delete;
  basic_stream(basic_stream&&) = delete;  // TODO: allow it in the future

  basic_stream(boost::asio::io_context& ioc);

  asio_stream::executor_type get_executor();

  void connect(const std::string& hostport);
  void connect(const std::string& hostport, boost::system::error_code& ec);

  void connect(const std::string& host, const std::string& port);
  void connect(const std::string& host, const std::string& port,
               boost::system::error_code& ec);

  void async_connect(const std::string& hostport, on_connect_cb cb);

  void async_connect(const std::string& host, const std::string& port,
                     on_connect_cb cb);

  template<typename MutableBuffer, typename Callback>
  void async_read_some(MutableBuffer&& buffer, Callback cb)
  {
    stream_.async_read_some(buffer,
                            [this, cb](auto&& ec, auto&& bytes_read)
                            {
                              if (ec)
                              {
                                reconnect_report(ec);
                              }

                              // TODO: report the error?
                              cb(ec, bytes_read);
                            });
  }

  template<typename ConstBuffer, typename Callback>
  void async_write_some(ConstBuffer&& buffer, Callback cb)
  {
    stream_.async_write_some(buffer,
                             [this, cb](auto&& ec, auto bytes_written)
                             {
                               if (ec)
                               {
                                 reconnect_report(ec);
                               }

                               cb(ec, bytes_written);
                             });
  }

  template<typename ConstBuffer, typename Callback>
  void async_write(ConstBuffer&& buffer, Callback&& cb)
  {
    boost::asio::async_write(stream_, buffer,
                             [this, cb](auto&& ec, auto bytes_written)
                             {
                               if (ec)
                               {
                                 reconnect_report(ec);
                               }

                               cb(ec, bytes_written);
                             });
  }

  void set_on_stream_closed(on_stream_closed_cb cb)
  {
    on_stream_closed_cb_ = cb;
  }

  void set_on_reconnect(on_reconnect_cb cb)
  {
    on_reconnect_cb_ = cb;
  }

  operator bool()
  {
    return stream_.is_open();
  }

  inline void close()
  {
    is_closed_ = true;
    stream_.close();
  }

private:
  void reconnect_report(boost::system::error_code);
  void reconnect();

private:
  // TODO: boost::asio::ssl::stream support SSL
  // implementing ssl will require a std::optional.
  // the SSL can't reset so optional is the best option
  asio_stream stream_;

  on_stream_closed_cb on_stream_closed_cb_;
  on_reconnect_cb on_reconnect_cb_;

  std::string original_host_;
  std::string original_port_;

  // is_closed is used to avoid reconnecting because the client closes on
  // purpose.
  bool is_closed_;
};

}  // namespace redis

#endif
