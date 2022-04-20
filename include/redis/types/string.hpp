#ifndef REDIS_TYPES_STRING_H
#define REDIS_TYPES_STRING_H

#include <string>
#include <ostream>

namespace redis::types
{
// simple string or a bulk string
// + string
// $ (number of bytes) bulk strings | $-1 is null
class string
{
  std::string s_;
  // for partial reads
  size_t expected_length_;
  bool is_null_;

public:
  string();

  string(double d);

  string(const std::string& s);

  size_t expected_length() const;

  size_t parse(const char* s, size_t n);

  void serialize(std::ostream& os, bool is_bulk = true) const;

  operator bool() const;

  const std::string& operator*() const;

  std::string& operator*();

  bool is_null() const;

  inline string& operator=(const std::string& s);

  size_t size() const;

  std::string::value_type operator[](size_t pos);

private:
  size_t parse_simple(const char* s, size_t n);

  void serialize_simple(std::ostream& os) const;

  size_t parse_bulk(const char* s, size_t n);

  void serialize_bulk(std::ostream& os) const;
};

}  // namespace redis::types

#endif
