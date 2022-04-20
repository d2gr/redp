#ifndef REDIS_TYPES_INTEGER_H
#define REDIS_TYPES_INTEGER_H

#include <string>
#include <ostream>

namespace redis::types
{
// integer
// : integer
class integer
{
  int64_t n_;
  bool has_value_;

public:
  integer() = default;
  integer(int64_t n);

  integer(int n);

  size_t parse(const char* s, size_t n);

  void serialize(std::ostream& os) const;

  inline int64_t operator*() const;

  operator bool() const;

  inline integer& operator=(int64_t n);
};

}  // namespace redis::types

#endif