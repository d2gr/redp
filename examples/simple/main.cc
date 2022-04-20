#include <redis/stream.hpp>
#include <redis/definitions.hpp>

#include <iostream>


void on_redis_set(redis::any_type resp)
{
  switch (resp.index())
  {
    case 0:
    {
      auto&& str = boost::variant2::get<0>(resp);
      std::cout << "Response: " << *str << std::endl;
      break;
    }
    case 1:
    {
      auto&& err = boost::variant2::get<1>(resp);
      std::cout << "error setting value: " << *err << std::endl;
      break;
    }
  }
}

void on_redis_connect(redis::stream& redis, boost::system::error_code& ec)
{
  if (ec)
  {
    std::cerr << "error connecting: " << ec.message() << std::endl;
    return;
  }

  redis.async_write(std::bind(&on_redis_set, std::placeholders::_1), "SET",
                    "my_key", "my_value");
}

int main()
{
  boost::asio::io_context ioc;
  redis::stream redis(ioc);

  std::cout << "Using Redis C++ " REDIS_VERSION << std::endl;

  redis.set_on_reconnect([&redis]()
                         { std::cout << "Reconnected!" << std::endl; });
  redis.set_on_stream_closed(
      [](auto&& ec)
      { std::cout << "Stream closed: " << ec.message() << std::endl; });

  redis.async_connect("127.0.0.1:6379",
                      [&redis](auto&& ec) { on_redis_connect(redis, ec); });

  ioc.run();

  return 0;
}
