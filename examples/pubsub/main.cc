#include <iostream>
#include <redis/definitions.hpp>
#include <redis/subscribed_stream.hpp>

int main()
{
  boost::asio::io_context ioc;
  redis::subscribed_stream redis(ioc);

  std::cout << "Using Redis C++ " REDIS_VERSION << std::endl;

  redis.connect("127.0.0.1:6379");

  redis.subscribe("x", [](auto&& topic, auto&& message)
                  { std::cout << topic << ": " << message << std::endl; });

  ioc.run();

  return 0;
}
