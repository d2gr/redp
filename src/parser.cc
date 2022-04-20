#include <redis/parser.hpp>

namespace redis
{
redis::parser::parser()
    : need_more_(false)
{
}

size_t parser::parse(const char* s, size_t n)
{
  need_more_ = false;
  size_t i   = 0;

  while (i < n)
  {
    if (s[i] == '+' || s[i] == '$')
    {
      types::string v;
      i += v.parse(&s[i], n - i);
      if (!(need_more_ = !v))
        type_ = v;
      break;
    }

    if (s[i] == ':')
    {
      types::integer v;
      i += v.parse(&s[i], n - i);
      if (!(need_more_ = !v))
        type_ = v;
      break;
    }

    if (s[i] == '-')
    {
      types::error v;
      i += v.parse(&s[i], n - i);
      if (!(need_more_ = !v))
        type_ = v;
      break;
    }

    if (s[i] == '*')
    {
      types::vector v;
      i += v.parse(&s[i], n - i);
      if (!(need_more_ = !v))
        type_ = v;
      break;
    }

    i++;
  }
  // TODO: need_more_ = need_more_ || i == n;

  return i;  // parsed
}

const parser::any_type& parser::operator*() const
{
  return type_;
}

parser::any_type& parser::operator*()
{
  return type_;
}

parser::any_type& parser::get()
{
  return type_;
}

bool parser::need_more() const
{
  return need_more_;
}
}  // namespace redis
