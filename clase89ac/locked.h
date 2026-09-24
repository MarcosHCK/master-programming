/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <mutex>
#include <type_traits>

template<typename Resource, typename Mutex = std::mutex> class locked
{

  Mutex _mutex;
  Resource _resource;
public:

  typedef Mutex mutex_type;
  typedef Resource value_type;

  locked (const locked&) = delete;

  template<typename U = Resource,
           typename = std::enable_if_t<std::is_move_constructible_v<Mutex>
                                    && std::is_move_constructible_v<U>>>
  inline constexpr locked (locked&& o) noexcept (std::is_nothrow_move_constructible_v<Mutex>
                                              && std::is_nothrow_move_constructible_v<Resource>):
      _mutex (std::move (o._mutex)), _resource (std::move (o._resource))
    { }

  template<typename... Args,
           typename = std::enable_if_t<std::is_default_constructible_v<Mutex>
                                    && std::is_constructible_v<Resource, Args ...>>>
  inline constexpr locked (Args&&... args) noexcept (std::is_nothrow_default_constructible_v<Mutex>
                                                  && std::is_nothrow_constructible_v<Resource, Args ...>):
      _mutex (), _resource (std::forward<Args> (args) ...)
    { }

  inline constexpr auto operator*() const
    { return std::pair<std::lock_guard<Mutex>, const Resource&> { _mutex, _resource }; }

  inline constexpr auto operator*()
    { return std::pair<std::lock_guard<Mutex>, Resource&> { _mutex, _resource }; }
};