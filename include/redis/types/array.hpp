#ifndef REDIS_TYPES_ARRAY_H
#define REDIS_TYPES_ARRAY_H

#include <redis/types/error.hpp>
#include <redis/types/integer.hpp>
#include <redis/types/string.hpp>

#include <algorithm>
#include <boost/variant2/variant.hpp>
#include <initializer_list>
#include <string>
#include <vector>

namespace redis::types
{
// array
// * arrays
class vector
{
public:
  using any_type  = boost::variant2::variant<string, error, integer, vector>;
  using container = std::vector<any_type>;

  vector();
  vector(std::initializer_list<any_type> list);

  void reset();

  size_t parse(const char* s, size_t n);

  void serialize(std::ostream& os) const;

  size_t expected_length() const;

  size_t processed() const;

  container& operator*();

  const container& operator*() const;

  operator bool() const;

  template<class T>
  inline void push_back(const T& v)
  {
    vs_.push_back(v);
  }

  template<class T>
  inline void push_back(T&& v)
  {
    vs_.push_back(v);
  }

  void clear();

private:
  container vs_;
  bool is_null_;
  // for partial reads
  size_t expected_length_;
  size_t processed_;
};
}  // namespace redis::types

#endif
