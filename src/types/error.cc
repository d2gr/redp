#include <redis/types/error.hpp>

namespace redis::types
{
// error
// - error
size_t error::parse(const char* s, size_t n)
{
  size_t i   = 0;
  size_t len = 0;

  expected_length_ = 0;
  e_.clear();

  while (i < n && s[i++] != '-')
    ;  // advance to the -
  size_t start = i;
  while (i < n && s[i++] != '\r')
    len++;

  if (i < n)
  {
    e_.assign(&s[start], len);
  }
  // advance the cursor
  while (i < n && s[i++] != '\n')
    ;

  return i;
}

void error::serialize(std::ostream& os) const
{
  os << '-' << e_ << "\r\n";
}

error::operator bool() const
{
  return not e_.empty();
}

const std::string& error::operator*() const
{
  return e_;
}

error& error::operator=(const std::string& e)
{
  e_ = e;
  return *this;
}

}  // namespace redis::types
