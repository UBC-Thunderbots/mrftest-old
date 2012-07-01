#ifndef UTIL_CACHEABLE_H
#define UTIL_CACHEABLE_H

#include "util/noncopyable.h"
#include <memory>
#include <tuple>
#include <unordered_map>

/**
 * The base class of all cacheable object types.
 * This should not be subclassed directly; instead, subclass Cacheable.
 * This class exists only to expose the flush_all() function.
 */
class CacheableBase : public NonCopyable {
	public:
		CacheableBase();
		~CacheableBase();
		static void flush_all();

	protected:
		virtual void flush() = 0;
};

template<typename ... T> struct CacheableKeyArgs {
};

template<typename ... T> class CacheableNonKeyArgs {
};

template<std::size_t DROP_FIRST, typename ... Args> class CacheableTupleBuilder;

template<> class CacheableTupleBuilder<0> {
	public:
		typedef std::tuple<> Tuple;

		static Tuple build_tuple() {
			return Tuple();
		}
};

template<typename Head, typename ... Tail> class CacheableTupleBuilder<0, Head, Tail ...> {
	public:
		typedef std::tuple<Head, Tail ...> Tuple;

		static Tuple build_tuple(const Head &head, const Tail & ... tail) {
			return Tuple(head, tail ...);
		}
};

template<std::size_t DROP_FIRST, typename Head, typename ... Tail> class CacheableTupleBuilder<DROP_FIRST, Head, Tail ...> {
	public:
		typedef typename CacheableTupleBuilder<DROP_FIRST - 1, Tail ...>::Tuple Tuple;

		static Tuple build_tuple(const Head &, const Tail & ... tail) {
			return CacheableTupleBuilder<DROP_FIRST - 1, Tail ...>::build_tuple(tail ...);
		}
};

template<std::size_t OFFSET, typename K, typename ... Args> struct CacheableTupleHasherImpl;

template<std::size_t OFFSET, typename ... Args> struct CacheableTupleHasherImpl<OFFSET, CacheableKeyArgs<Args ...>> {
	static std::size_t hash(const std::tuple<Args ...> &) {
		return 0;
	}
};

template<std::size_t OFFSET, typename ... Args, typename First, typename ... Rest> struct CacheableTupleHasherImpl<OFFSET, CacheableKeyArgs<Args ...>, First, Rest ...> {
	static std::size_t hash(const std::tuple<Args ...> &t) {
		return std::hash<First>()(std::get<OFFSET>(t)) * 17 + CacheableTupleHasherImpl<OFFSET + 1, CacheableKeyArgs<Args ...>, Rest ...>::hash(t);
	}
};

template<typename ... Args> struct CacheableTupleHasher {
	std::size_t operator()(const std::tuple<Args ...> &t) const {
		return CacheableTupleHasherImpl<0, CacheableKeyArgs<Args ...>, Args ...>::hash(t);
	}
};

template<typename R, typename NK, typename K, typename ... Args> class CacheableImpl;

template<typename R, typename ... NK, typename ... Args> class CacheableImpl<R, CacheableNonKeyArgs<NK ...>, CacheableKeyArgs<>, Args ...> {
	public:
		const R &operator()(const Args & ... args) {
			if (!cache) {
				cache.reset(new R(compute(args ...)));
			}
			return *cache.get();
		}

		void flush() {
			cache.reset();
		}

	protected:
		virtual R compute(Args ... args) = 0;

	private:
		std::unique_ptr<R> cache;
};

template<typename R, typename ... NK, typename ... K, typename ... Args> class CacheableImpl<R, CacheableNonKeyArgs<NK ...>, CacheableKeyArgs<K ...>, Args ...> {
	public:
		const R &operator()(const Args & ... args) {
			const Tuple &cache_key = CacheableTupleBuilder<sizeof ... (NK), Args ...>::build_tuple(args ...);
			typename Map::iterator iter = cache.find(cache_key);
			if (iter == cache.end()) {
				iter = cache.insert(cache.begin(), typename Map::value_type(cache_key, compute(args ...)));
			}
			return iter->second;
		}

		void flush() {
			cache.clear();
		}

	protected:
		virtual R compute(Args ... args) = 0;

	private:
		typedef std::tuple<K ...> Tuple;
		typedef std::unordered_map<Tuple, R, CacheableTupleHasher<K ...>> Map;
		Map cache;
};

template<typename R, typename NK, typename K> class Cacheable;

/**
 * A "function" whose value can be cached and not recomputed during a tick.
 *
 * As an example of how to implement such a computation, the following would be placed in a header file:
 *
 * \code
 * // Third parameter type (part of cache key) ------------------------------------------------------------------\\\\\\
 * //                                                                                                            ||||||
 * // Second parameter type (part of cache key) ---------------------------------------------------------\\\\\\  ||||||
 * //                                                                                                    ||||||  ||||||
 * // First parameter type (not saved or compared in cache) -------\\\\\\\\\\\\\\\\\\                    ||||||  ||||||
 * //                                                              ||||||||||||||||||                    ||||||  ||||||
 * //          Function return type ---\\\\\\                      ||||||||||||||||||                    ||||||  ||||||
 * class MyFunction : public Cacheable<double, CacheableNonKeyArgs<AI::HL::W::World &>, CacheableKeyArgs<double, double>> {
 *     protected:
 *         double compute(AI::HL::W::World &w, double x, double y);
 * };
 *
 * extern MyFunction my_function;
 * \endcode
 *
 * The following would be placed in the corresponding source file:
 *
 * \code
 * double MyFunction::compute(AI::HL::W::World &w, double x, double y) const {
 *     return x * y + w.field().length(); // Or whatever calculation you want to do.
 * }
 *
 * MyFunction my_function;
 * \endcode
 *
 * Finally, to call the function, just do this:
 *
 * \code
 * double product = my_function(my_world, 27, 42);
 * \endcode
 *
 * In this particular example, the world is not part of the cache key
 * (because we assume there is only one, and because it is too heavy-weight to copy around into the cache).
 * Therefore, if you were to call \c my_function with a different world, this would not be detected.
 * The parameters \p x and \p y, on the other hand, are part of the cache key.
 * Therefore, a cached value will be returned precisely when \p x and \p y match a prior invocation.
 *
 * \tparam R the type of value returned by the function.
 *
 * \tparam NK the non-key parameters to the function.
 *
 * \tparam K the key parameters to the function.
 */
template<typename R, typename ... NK, typename ... K> class Cacheable<R, CacheableNonKeyArgs<NK ...>, CacheableKeyArgs<K ...>> : public CacheableImpl<R, CacheableNonKeyArgs<NK ...>, CacheableKeyArgs<K ...>, NK ..., K ...>, public CacheableBase {
	public:
		void flush() {
			CacheableImpl<R, CacheableNonKeyArgs<NK ...>, CacheableKeyArgs<K ...>, NK ..., K ...>::flush();
		}
};

#endif

