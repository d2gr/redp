#ifndef REDIS_TYPES_ERROR_H
#define REDIS_TYPES_ERROR_H

#include <string>
#include <ostream>

namespace redis::types
{
// error
// - error
class error
{
  std::string e_;
  // for partial reads
  size_t expected_length_;

public:
  error() = default;
  size_t parse(const char* s, size_t n);

  void serialize(std::ostream& os) const;

  operator bool() const;

  const std::string& operator*() const;

  error& operator=(const std::string& e);
};

}  // namespace redis::types

#endif
