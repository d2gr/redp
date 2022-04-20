#ifndef REDIS_STREAM_H
#define REDIS_STREAM_H

#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>
#include <redis/basic_stream.hpp>
#include <redis/parser.hpp>
#include <utility>

#ifndef DEFAULT_READ_SIZE
#define DEFAULT_READ_SIZE 1024
#endif

namespace redis
{
using any_type = redis::parser::any_type;

/**
 * stream represents a direct stream to redis.
 * The class will automatically reconnect if the connection is lost.
 **/
class stream
{
public:
  using handler = std::function<void(any_type)>;

public:
  stream()         = delete;
  stream(stream&)  = delete;
  stream(stream&&) = delete;

  /**
   * Should always be initialised with the io_context.
   **/
  stream(boost::asio::io_context& ioc);

  /**
   * Returns the io_context passed on the constructor.
   **/
  basic_stream::asio_stream::executor_type get_executor();

  /**
   * Establishes a connection to a redis instance.
   *
   * Note that this function will throw an exception if any
   *boost::system::error_code error is encountered.
   *
   * @param hostport Should be a valid host and port in the following format
   * `host:port`.
   **/
  void connect(const std::string& hostport);

  /**
   * Establishes a connection to a redis instance.
   *
   * This function will not throw any exception.
   *
   * @param hostport Should be a valid host and port in the following format
   *`host:port`.
   * @param ec Is a valid reference to a boost::system::error_code that will be
   *set by the function if any error happens.
   **/
  void connect(const std::string& hostport,
               boost::system::error_code& ec) noexcept;

  /**
   * Establishes a connection to a redis instance.
   *
   * Note that this function will throw an exception if any
   *boost::system::error_code error is encountered.
   *
   * @param host Is a valid hostname or IP.
   * @param port Is a valid port as a string.
   **/
  void connect(const std::string& host, const std::string& port);

  /**
   * Establishes a connection to a redis instance.
   *
   * @param host Is a valid hostname or IP.
   * @param port Is a valid port as a string.
   * @param ec Is a valid reference to a boost::system::error_code that will be
   *set by the function if any error happens.
   **/
  void connect(const std::string& host, const std::string& port,
               boost::system::error_code& ec) noexcept;

  /**
   * Establishes a connection to a redis instance asynchronously.
   *
   * This function will return immediately.
   *
   * @param hostport Should be a valid host and port in the following format
   *`host:port`.
   * @param cb Is the callback that will get called when the operation ends.
   **/
  void async_connect(const std::string& hostport,
                     basic_stream::on_connect_cb cb) noexcept;

  /**
   * Establishes a connection to a redis instance asynchronously.
   *
   * This function will return immediately.
   *
   * @param host Is a valid hostname or IP.
   * @param port Is a valid port as a string.
   * @param cb Is the callback that will get called when the operation ends.
   **/
  void async_connect(const std::string& host, const std::string& port,
                     basic_stream::on_connect_cb cb) noexcept;

  /**
   * Sends a command to the REDIS server.
   *
   * @param cb Is the callback that will get called after the command has been
   *acknowledged by the server.
   * @param args Are the command and arguments to send to the server. For
   *example: "SET", "key", "value". The parameters can be any type convertible
   *to std::string, double, int(32|64) and any of the redis::types.
   **/
  template<class Handler, class... Args>
  stream& async_write(Handler&& cb, Args... args)
  {
    size_t current_buffer_size = write_buffer_.size();

    std::ostream os(&write_buffer_);
    // send as array
    os << '*' << sizeof...(args) << "\r\n";

    write_to(os, std::forward<Args>(args)...);

    queue_.emplace_back(write_buffer_.size() - current_buffer_size, cb);
    if (is_sending_)
      return *this;

    next_request();

    return *this;
  }

  // template<typename Topic>
  // subscribed_stream subscribe(Topic topic)
  // {

  // }

  /**
  * Sets a callback for when the connection is lost with the REDIS server.
  * 
  * @param cb Is the callback that will get called upon the connection is lost.
  **/
  void set_on_stream_closed(basic_stream::on_stream_closed_cb cb)
  {
    stream_.set_on_stream_closed(cb);
  }

  /**
  * Sets a callback for when the connection has been re-established with the REDIS server.
  * 
  * @param cb Is the callback taht will get called once the connection has been recovered.
  **/
  void set_on_reconnect(basic_stream::on_reconnect_cb cb)
  {
    stream_.set_on_reconnect(cb);
  }

  /**
  * Returns whether the socket is open or not.
  **/
  operator bool()
  {
    return !!stream_;
  }

  /**
  * Closes the connection and the underlying socket.
  * 
  * Note that this call will also disable reconnection. If the user calls `close` with
  * the intention to reconnect the connection bear in mind to call async_connect afterwards.
  **/
  inline void close()
  {
    stream_.close();
  }

private:
  // we need to stop forwarding at some point
  void write_to(std::ostream& os)
  {
  }

  template<class... Args>
  void write_to(std::ostream& os, const std::string& s, Args... args)
  {
    redis::types::string rs(s);
    rs.serialize(os);

    write_to(os, std::forward<Args>(args)...);
  }

  // double are strings
  template<class... Args>
  void write_to(std::ostream& os, double s, Args... args)
  {
    redis::types::string rs(s);
    rs.serialize(os);

    write_to(os, std::forward<Args>(args)...);
  }

  template<class... Args>
  void write_to(std::ostream& os, const redis::types::string& rs, Args... args)
  {
    rs.serialize(os);
    write_to(os, std::forward<Args>(args)...);
  }

  template<class... Args>
  void write_to(std::ostream& os, const redis::types::vector& vs, Args... args)
  {
    vs.serialize(os);
    write_to(os, std::forward<Args>(args)...);
  }

  template<class T, class... Args,
           typename = std::enable_if_t<std::is_integral_v<T>>>
  void write_to(std::ostream& os, T n, Args... args)
  {
    redis::types::integer ri(n);
    ri.serialize(os);

    write_to(os, std::forward<Args>(args)...);
  }

  template<class... Args>
  void write_to(std::ostream& os, const redis::types::integer& ri, Args... args)
  {
    ri.serialize(os);
    write_to(os, std::forward<Args>(args)...);
  }

  void next_request()
  {
    is_sending_ = true;

    stream_.async_write(write_buffer_, [this](auto&& ec, size_t bytes_written)
                        { on_write(ec, bytes_written); });
  }

  void on_write(boost::system::error_code const& ec, size_t written)
  {
    if (ec)
    {
      return;
    }

    stream_.async_read_some(read_buffer_.prepare(DEFAULT_READ_SIZE),
                            [this, written](auto&& ec, size_t bytes_read)
                            {
                              if (!ec)
                                write_buffer_.consume(written);
                              on_read(ec, bytes_read);
                            });
  }

  void on_read(boost::system::error_code const& ec, size_t bytes_read)
  {
    if (ec)
    {
      return;
    }

    read_buffer_.commit(bytes_read);

    size_t bytes_parsed = parser_.parse(
        (const char*) read_buffer_.data().data(), read_buffer_.size());
    if (parser_.need_more())
    {
      // TODO: handle partial reads
      stream_.async_read_some(read_buffer_.prepare(DEFAULT_READ_SIZE),
                              [this](auto&& ec, size_t bytes_read)
                              { on_read(ec, bytes_read); });
      return;
    }

    read_buffer_.consume(bytes_parsed);

    auto& e = queue_.front();
    e.second(*parser_);
    queue_.pop_front();

    if ((is_sending_ = !queue_.empty()))
      next_request();
  }

private:
  redis::basic_stream stream_;

  bool is_sending_;
  redis::parser parser_;

  // write buffer
  boost::asio::streambuf write_buffer_;
  // read buffer
  boost::asio::streambuf read_buffer_;

  // readed bytes aka size of the content of read_buffer_
  std::list<std::pair<size_t, handler>> queue_;
};
}  // namespace redis

#endif
