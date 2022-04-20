#include <redis/subscribed_stream.hpp>

namespace redis
{
subscribed_stream::subscribed_stream(boost::asio::io_context& ioc)
    : stream_(ioc)
    , is_reading_(false)
    , is_writing_(false)
{
  stream_.set_on_reconnect(
      [this]()
      {
        resubscribe();
        read();
        write();
      });
}

auto subscribed_stream::get_executor() -> basic_stream::asio_stream::executor_type
{
  return stream_.get_executor();
}

void subscribed_stream::connect(const std::string& hostport)
{
  stream_.connect(hostport);
}

void subscribed_stream::connect(const std::string& hostport,
                                boost::system::error_code& ec) noexcept
{
  stream_.connect(hostport, ec);
}

void subscribed_stream::connect(const std::string& host,
                                const std::string& port)
{
  stream_.connect(host, port);
}

void subscribed_stream::connect(const std::string& host,
                                const std::string& port,
                                boost::system::error_code& ec) noexcept
{
  stream_.connect(host, port, ec);
}

void subscribed_stream::async_connect(const std::string& hostport,
                                      basic_stream::on_connect_cb cb) noexcept
{
  stream_.async_connect(hostport, cb);
}

void subscribed_stream::async_connect(const std::string& host,
                                      const std::string& port,
                                      basic_stream::on_connect_cb cb) noexcept
{
  stream_.async_connect(host, port, cb);
}

void subscribed_stream::subscribe(const std::string& topic, message_cb cb)
{
  subscription_meta_.insert({topic, subscription_type::normal});
  subscribe("SUBSCRIBE", topic, cb);
}

void subscribed_stream::psubscribe(const std::string& topic, message_cb cb)
{
  subscription_meta_.insert({topic, subscription_type::regex});
  subscribe("PSUBSCRIBE", topic, cb);
}

bool subscribed_stream::unsubscribe(const std::string& topic)
{
  subscription_type type;
  // erase from metadata
  {
    auto it = subscription_meta_.find(topic);
    if (it == subscription_meta_.end())
      return false;

    type = it->second;

    subscription_meta_.erase(it);
  }

  // erase from subscriptions
  {
    auto it = subscriptions_.find(topic);
    if (it == subscriptions_.end())
      return false;

    subscriptions_.erase(it);
  }

  switch (type)
  {
    case subscription_type::normal:
      unsubscribe("UNSUBSCRIBE", topic);
      break;
    case subscription_type::regex:
      unsubscribe("PUNSUBSCRIBE", topic);
      break;
  }

  return true;
}

void subscribed_stream::unsubscribe(std::string_view command,
                                    const std::string& topic)
{
  std::ostream os(&write_buffer_);

  os << "*2\r\n"
     << "$" << command.size() << "\r\n"
     << command << "\r\n"
     << "$" << topic.size() << "\r\n"
     << topic << "\r\n";

  write();
}

void subscribed_stream::subscribe(std::string_view command,
                                  const std::string& topic, message_cb cb)
{
  auto sub_it = subscriptions_.insert({topic, cb});

  std::ostream os(&write_buffer_);

  os << "*2\r\n"
     << "$" << command.size() << "\r\n"
     << command << "\r\n"
     << "$" << topic.size() << "\r\n"
     << topic << "\r\n";

  write();
  read();
}

void subscribed_stream::resubscribe()
{
  for (auto&& [topic, type] : subscription_meta_)
  {
    auto it = subscriptions_.find(topic);
    if (it == subscriptions_.end())
      continue;

    auto&& cb = it->second;

    switch (type)
    {
      case subscription_type::normal:
        subscribe(topic, cb);
        break;
      case subscription_type::regex:
        psubscribe(topic, cb);
        break;
    }
  }
}

void subscribed_stream::write()
{
  if (write_buffer_.size() == 0)
    return;

  if (is_writing_)
    return;
  is_writing_ = false;

  stream_.async_write(write_buffer_,
                      [this](auto&& ec, size_t bytes_written)
                      {
                        is_writing_ = false;

                        if (!ec)
                        {
                          write_buffer_.consume(bytes_written);
                          write();
                        }
                      });
}

void subscribed_stream::read()
{
  if (is_reading_)
    return;
  is_reading_ = true;

  stream_.async_read_some(read_buffer_.prepare(DEFAULT_READ_SIZE),
                          [this](auto&& ec, size_t read_bytes)
                          { on_read(ec, read_bytes); });
}

void subscribed_stream::on_read(boost::system::error_code const& ec,
                                size_t read_bytes)
{
  is_reading_ = false;

  if (ec)
    return;

  read_buffer_.commit(read_bytes);

  size_t parsed_bytes = parser_.parse((const char*) read_buffer_.data().data(),
                                      read_buffer_.size());
  if (!parser_.need_more())
  {
    read_buffer_.consume(parsed_bytes);

    boost::variant2::visit(message_parser_, *parser_);

    if (message_parser_)
    {
      auto&& it = subscriptions_.find(message_parser_.channel);
      if (it != subscriptions_.end())
      {
        it->second(message_parser_.target_channel, message_parser_.message);
      }
    }
  }

  read();
}

}  // namespace redis
