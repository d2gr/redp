#include <redis/types/array.hpp>

namespace redis::types
{
// array
// * arrays
vector::vector()
    : is_null_(false)
    , expected_length_(0)
    , processed_(0)
{
}

vector::vector(std::initializer_list<any_type> list)
    : vs_(list)
    , expected_length_(0)
    , processed_(0)
{
}

void vector::reset()
{
  vs_.clear();
  expected_length_ = 0;
  processed_       = 0;
  is_null_         = false;
}

size_t vector::parse(const char* s, size_t n)
{
  size_t i = 0;

  if (vs_.empty())
  {
    while (i < n && s[i++] != '*')
      ;

    if (!(is_null_ = (s[i] == '-')))
      expected_length_ = atoll(&s[i]);

    while (i < n && s[i++] != '\n')
      ;

    if (is_null_)
      return i;
  }

  bool need_more = false;
  while (!need_more && i < n && processed_ < expected_length_)
  {
    switch (s[i])
    {
      case '$':
      case '+':
      {
        string v;
        size_t pr = v.parse(&s[i], n);
        if (!(need_more = !v))
        {
          i += pr;
          vs_.push_back(v);
          processed_++;
        }
      }
      break;
      case '-':
      {
        error v;
        size_t pr = v.parse(&s[i], n);
        if (!(need_more = !v))
        {
          i += pr;
          vs_.push_back(v);
          processed_++;
        }
      }
      break;
      case ':':
      {
        integer v;
        size_t pr = v.parse(&s[i], n);
        if (!(need_more = !v))
        {
          i += pr;
          vs_.push_back(v);
          processed_++;
        }
      }
      break;
      case '*':
      {
        vector v;
        size_t pr = v.parse(&s[i], n);
        if (!(need_more = !v))
        {
          i += pr;
          vs_.push_back(v);
          processed_++;
        }
      }
      break;
      default:
        i++;
        break;
    }
  }

  return i;
}

void vector::serialize(std::ostream& os) const
{
  os << '*' << vs_.size() << "\r\n";

  for (auto& v : vs_)
  {
    boost::variant2::visit([&](auto const& e) { e.serialize(os); }, v);
  }
}

size_t vector::expected_length() const
{
  return expected_length_;
}

size_t vector::processed() const
{
  return processed_;
}

vector::container& vector::operator*()
{
  return vs_;
}

const vector::container& vector::operator*() const
{
  return vs_;
}

vector::operator bool() const
{
  return not is_null_ && processed_ == expected_length_ && not vs_.empty();
}

void vector::clear()
{
  vs_.clear();
}

}  // namespace redis::types
