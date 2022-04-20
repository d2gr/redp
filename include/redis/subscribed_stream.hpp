#ifndef REDIS_SUBSCRIBED_STREAM_H
#define REDIS_SUBSCRIBED_STREAM_H

#include <boost/algorithm/string/predicate.hpp>
#include <redis/basic_stream.hpp>
#include <redis/parser.hpp>
#include <redis/types.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

#ifndef DEFAULT_READ_SIZE
#define DEFAULT_READ_SIZE 1024
#endif

namespace redis
{
class subscribed_stream
{
public:
  using message_cb = std::function<void(std::string, std::string)>;

private:
  struct message_parser
  {
    std::string channel;
    std::string target_channel;
    std::string message;

    operator bool() const
    {
      return !channel.empty() && !message.empty();
    }

    // the array it's called next
    void operator()(const redis::types::vector& v)
    {
      auto const& vs = *v;

      auto&& message_type = boost::variant2::get<0>(vs[0]);
      if (!boost::algorithm::iends_with(*message_type, "message"))
      {
        return;
      }

      channel = *boost::variant2::get<0>(vs[1]);  // get the string from vs[1]

      // normal pubsub
      if (vs.size() < 4)
      {
        target_channel = channel;
        message        = *boost::variant2::get<0>(vs[2]);
      }
      else  // subscription using regex
      {
        target_channel = *boost::variant2::get<0>(vs[2]);
        message        = *boost::variant2::get<0>(vs[3]);
      }
    }

    void operator()(const redis::types::string& v)
    {
    }

    void operator()(const redis::types::integer& v)
    {
    }

    void operator()(const redis::types::error& v)
    {
    }
  };

public:
  /**
   * Should always be initialised with the io_context.
   **/
  subscribed_stream(boost::asio::io_context& ioc);

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
   * Subscribes to a topic.
   *
   * @param topic Is the topic to subscribe to.
   * @param cb Is the callback that will get called after every message.
   **/
  void subscribe(const std::string& topic, message_cb cb);

  /**
   * Subscribe to a topic using a RegEX.
   *
   * @see https://redis.io/commands/psubscribe
   *
   * @param topic Is the topic to subscribe to.
   * @param cb Is the callback that will get called after every message.
   **/
  void psubscribe(const std::string& topic, message_cb cb);

  /**
   * Unsubscribe from a topic.
   *
   * @param topic Is the topic to unsubscribe from.
   **/
  bool unsubscribe(const std::string& topic);

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
   * Note that this call will also disable reconnection. If the user calls
   *`close` with the intention to reconnect the connection bear in mind to call
   *async_connect afterwards.
   **/
  inline void close()
  {
    stream_.close();
  }

private:
  void unsubscribe(std::string_view, const std::string&);

  void on_connect(boost::system::error_code const& ec,
                  basic_stream::on_connect_cb cb);

  void subscribe(std::string_view command, const std::string& topic,
                 message_cb);

  void on_subscribed(boost::system::error_code const& ec, size_t bytes_written);

  void read();
  void write();

  void on_read(boost::system::error_code const& ec, size_t read_bytes);

  void resubscribe();

private:
  enum subscription_type
  {
    normal,
    regex
  };

private:
  redis::basic_stream stream_;
  redis::parser parser_;

  message_parser message_parser_;

  std::unordered_map<std::string, message_cb> subscriptions_;
  std::unordered_map<std::string, subscription_type> subscription_meta_;

  boost::asio::streambuf read_buffer_;
  boost::asio::streambuf write_buffer_;

  bool is_reading_;
  bool is_writing_;
};
}  // namespace redis

#endif
