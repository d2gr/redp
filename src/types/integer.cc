#include <redis/types/integer.hpp>

namespace redis::types
{
// integer
// : integer
integer::integer(int64_t n)
    : n_(n)
{
}

integer::integer(int n)
    : n_(static_cast<int64_t>(n))
{
}

size_t integer::parse(const char* s, size_t n)
{
  size_t i   = 0;
  n_         = 0;
  has_value_ = false;

  while (i < n && s[i++] != ':')
    ;  // advance to the -
  size_t start = i;
  while (i < n && s[++i] != '\n')
    ;
  if (i++ < n)
  {
    has_value_ = true;
    n_         = atoll(&s[start]);
  }

  return i;
}

void integer::serialize(std::ostream& os) const
{
  os << ':' << n_ << "\r\n";
}

int64_t integer::operator*() const
{
  return n_;
}

integer::operator bool() const
{
  return has_value_;
}

integer& integer::operator=(int64_t n)
{
  n_ = n;
  return *this;
}

}  // namespace redis::types
