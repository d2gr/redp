#include <redis/types/string.hpp>

namespace redis::types
{
string::string()
    : is_null_(false)
{
}

string::string(double d)
    : is_null_(false)
    , s_(std::move(std::to_string(d)))
{
}

string::string(const std::string& s)
    : is_null_(false)
    , s_(s)
{
}

size_t string::expected_length() const
{
  return expected_length_;
}

size_t string::parse(const char* s, size_t n)
{
  size_t i = 0;

  s_.clear();
  expected_length_ = 0;
  is_null_         = false;

  while (i < n && s[i] != '+' && s[i] != '$')
    i++;  // advance to the + or $

  if (s[i++] == '+')
    i += parse_simple(&s[i], n - i);
  else
    i += parse_bulk(&s[i], n - i);

  return i;
}

void string::serialize(std::ostream& os, bool is_bulk) const
{
  is_bulk ? serialize_bulk(os) : serialize_simple(os);
}

string::operator bool() const
{
  return not is_null_ && not s_.empty();
}

const std::string& string::operator*() const
{
  return s_;
}

std::string& string::operator*()
{
  return s_;
}

bool string::is_null() const
{
  return is_null_;
}

string& string::operator=(const std::string& s)
{
  s_ = s;
  return *this;
}

size_t string::size() const
{
  return s_.size();
}

std::string::value_type string::operator[](size_t pos)
{
  return s_[pos];
}

// private:
size_t string::parse_simple(const char* s, size_t n)
{
  size_t i = 0;
  while (i < n && s[++i] != '\r')
    ;
  if (i < n)
  {
    s_.assign(s, i);
  }
  // advance the cursor
  while (i < n && s[i++] != '\n')
    ;

  return i;
}

void string::serialize_simple(std::ostream& os) const
{
  os << '+' << s_ << "\r\n";
}

size_t string::parse_bulk(const char* s, size_t n)
{
  size_t i = 0;
  is_null_ = s[0] == '-';

  while (i < n && s[i++] != '\n')
    ;
  if (is_null_)
    return i;

  expected_length_ = std::atoll(s);

  size_t start = i;
  size_t len   = 0;
  while (i < n && s[i++] != '\r')
    len++;
  if (len == expected_length_)
  {
    s_.assign(&s[start], len);
    // advance the cursor
    while (i < n && s[i++] != '\n')
      ;
  }

  return i;
}

void string::serialize_bulk(std::ostream& os) const
{
  os << '$' << s_.size() << "\r\n" << s_ << "\r\n";
}

}  // namespace redis::types
