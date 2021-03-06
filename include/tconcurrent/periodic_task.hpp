#ifndef TCONCURRENT_PERIODIC_TASK_HPP
#define TCONCURRENT_PERIODIC_TASK_HPP

#include <chrono>
#include <mutex>
#include <type_traits>

#include <tconcurrent/future.hpp>

#include <tconcurrent/detail/export.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// remove dll-interface warning
#pragma warning(disable : 4251)
#endif

namespace tconcurrent
{

class TCONCURRENT_EXPORT periodic_task
{
public:
  using duration_type = std::chrono::steady_clock::duration;

  enum StartOption
  {
    no_option,
    start_immediately,
  };

  ~periodic_task();

  void set_period(duration_type period)
  {
    scope_lock l(_mutex);
    _period = period;
  }

  template <typename C>
  auto set_callback(C&& cb) -> typename std::enable_if<
      std::is_same<decltype(cb()), future<void>>::value>::type
  {
    scope_lock l(_mutex);
    _callback = std::forward<C>(cb);
  }

  template <typename C>
  auto set_callback(C&& cb) -> typename std::enable_if<
      !std::is_same<decltype(cb()), future<void>>::value>::type
  {
    scope_lock l(_mutex);
    _callback = [cb = std::forward<C>(cb)] {
      cb();
      return make_ready_future();
    };
  }

  template <typename E>
  void set_executor(E&& executor)
  {
    _executor = std::forward<E>(executor);
  }

  void start(StartOption opt = no_option);
  future<void> stop();

  bool is_running() const
  {
    return _state == State::Running;
  }

private:
  using mutex = std::recursive_mutex;
  using scope_lock = std::lock_guard<mutex>;

  enum class State
  {
    Stopped,
    Running,
    Stopping,
  };

  mutex _mutex;

  State _state{State::Stopped};

  duration_type _period;
  std::function<future<void>()> _callback;

  future<void> _future;

  executor _executor{get_default_executor()};

  void reschedule();
  future<void> do_call(cancelation_token& token);
};
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
