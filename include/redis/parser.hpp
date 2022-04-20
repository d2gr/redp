#ifndef REDIS_PARSER_H
#define REDIS_PARSER_H

#include <redis/types.hpp>

#include <boost/variant2/variant.hpp>

namespace redis
{
class parser
{
public:
  using any_type = redis::types::vector::any_type;

  parser();

  size_t parse(const char* s, size_t n);

  const any_type& operator*() const;

  any_type& operator*();

  any_type& get();

  bool need_more() const;

private:
  any_type type_;
  bool need_more_;
};
};  // namespace redis

#endif
