/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <exception>
#include <stacktrace>
#include <type_traits>

#if !defined(DEBUG) || 0 == DEBUG
# define THROW(type,...) throw type (__VA_ARGS__)
#else // defined(DEBUG) && 0 != DEBUG
# define THROW(type,...) throw wrap_stacktrace<type> (__VA_ARGS__)
#endif // DEBUG

namespace utility
{

  class with_stacktrace: public virtual std::exception
    {
    public:
      virtual const std::stacktrace& get_trace () const noexcept = 0;
    protected:
      static std::stacktrace current (size_t skip = 0);
      static std::stacktrace current (size_t skip, size_t max_depth);
    };

  template<typename T>
  concept exception_like = std::is_same_v<T, std::exception>
                        || std::is_base_of_v<std::exception, T>;

  template<exception_like T = std::exception>
  class wrap_stacktrace: public virtual std::exception, public virtual with_stacktrace
    {

      T _excpt;
      std::stacktrace _trace;
    public:

      template<typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
      inline wrap_stacktrace (Args&&... args) noexcept (std::is_nothrow_constructible_v<T, Args...>)
        : _excpt (std::forward<Args> (args) ...), _trace (current (1))
      { }

      const std::stacktrace& get_trace () const noexcept { return _trace; }

      const char* what () const _GLIBCXX_TXN_SAFE_DYN noexcept { return _excpt.what (); };
    };
}