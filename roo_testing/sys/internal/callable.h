#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace roo_testing {

namespace internal {

template <size_t... Indexes>
struct IndexTuple {
  typedef IndexTuple<Indexes..., sizeof...(Indexes)> next;
};

template <std::size_t N>
struct BuildIndexTuple {
  typedef typename BuildIndexTuple<N - 1>::type::next type;
};

template <>
struct BuildIndexTuple<0> {
  typedef IndexTuple<> type;
};

template <typename...>
struct And;

template <>
struct And<> : public std::true_type {};

template <typename Arg1>
struct And<Arg1> : public Arg1 {};

template <typename Arg1, typename Arg2>
struct And<Arg1, Arg2>
    : public std::conditional<Arg1::value, Arg2, Arg1>::type {};

template <typename Arg1, typename Arg2, typename Arg3, typename... ArgN>
struct And<Arg1, Arg2, Arg3, ArgN...>
    : public std::conditional<Arg1::value, And<Arg2, Arg3, ArgN...>,
                              Arg1>::type {};

template <typename Arg>
struct Not : public std::bool_constant<!bool(Arg::value)> {};

template <typename... Condition>
using Require = typename std::enable_if<And<Condition...>::value>::type;

template <typename A, typename B>
using RequireNotSame = Require<Not<std::is_same<
    typename std::remove_cv<typename std::remove_reference<A>::type>::type,
    typename std::remove_cv<typename std::remove_reference<B>::type>::type>>>;

template <typename Tuple>
struct Invoker {
  Tuple tuple;

  template <size_t Index>
  static std::tuple_element_t<Index, Tuple>&& DeclVal();

  template <size_t... Ind>
  auto invoke(internal::IndexTuple<Ind...>) noexcept(
      noexcept(std::invoke(DeclVal<Ind>()...)))
      -> decltype(std::invoke(DeclVal<Ind>()...)) {
    return std::invoke(std::get<Ind>(std::move(tuple))...);
  }

  using Indices =
      typename internal::BuildIndexTuple<std::tuple_size<Tuple>::value>::type;

  auto operator()() noexcept(
      noexcept(std::declval<Invoker&>().invoke(Indices())))
      -> decltype(std::declval<Invoker&>().invoke(Indices())) {
    return invoke(Indices());
  }
};

template <typename... Tp>
using DecayedTuple = std::tuple<typename std::decay<Tp>::type...>;

template <typename Callable, typename... Args>
inline Invoker<DecayedTuple<Callable, Args...>> MakeInvoker(Callable&& callable,
                                                            Args&&... args) {
  return {DecayedTuple<Callable, Args...>{std::forward<Callable>(callable),
                                          std::forward<Args>(args)...}};
}

class VirtualCallable {
 public:
  virtual ~VirtualCallable() = default;
  virtual void call() = 0;
};

template <typename Callable>
class DynamicCallable : public VirtualCallable {
 public:
  DynamicCallable(Callable&& callable)
      : callable_(std::forward<Callable>(callable)) {}

  void call() override { callable_(); }

 private:
  Callable callable_;
};

template <typename Callable>
static std::unique_ptr<internal::VirtualCallable> MakeDynamicCallable(
    Callable&& callable) {
  return std::unique_ptr<internal::VirtualCallable>{
      new DynamicCallable<Callable>{std::forward<Callable>(callable)}};
}

// Given the callable and the arguments that should be bound to it, returns
// an instance of the VirtualCallable that invokes that callable and calls the
// arguments, using perfect forwarding and pass-by-reference.
template <typename Callable, typename... Args>
static std::unique_ptr<internal::VirtualCallable> MakeDynamicCallableWithArgs(
    Callable&& callable, Args&&... args) {
  return MakeDynamicCallable(MakeInvoker(std::forward<Callable>(callable),
                                         std::forward<Args>(args)...));
}

}  // namespace internal
}  // namespace roo_testing