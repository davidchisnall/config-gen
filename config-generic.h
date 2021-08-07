// Copyright David Chisnall
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <assert.h>
#include <chrono>
#include <initializer_list>
#include <string_view>
#include <ucl.h>
#include <unordered_map>
#include <utility>

#include <stdio.h>

#ifndef CONFIG_DETAIL_NAMESPACE
#	define CONFIG_DETAIL_NAMESPACE config::detail
#endif
#ifdef __clang__
#define CONFIG_LIFETIME_BOUND [[clang::lifetimebound]]
#else
#define CONFIG_LIFETIME_BOUND
#endif


namespace CONFIG_DETAIL_NAMESPACE
{
	/**
	 * Smart pointer to a UCL object, manages the lifetime of the object.
	 */
	class UCLPtr
	{
		/**
		 * The UCL object that this manages a reference to.
		 */
		const ucl_object_t *obj = nullptr;

		public:
		/**
		 * Default constructor, points to nothing.
		 */
		UCLPtr() {}

		/**
		 * Construct from a raw UCL object pointer.  This does not take
		 * ownership of the reference, the underlying object must have unref'd
		 * if this is called with an owning reference.
		 */
		UCLPtr(const ucl_object_t *o) : obj(ucl_object_ref(o)) {}

		/**
		 * Copy constructor, increments the reference count of the underlying
		 * object.
		 */
		UCLPtr(const UCLPtr &o) : obj(ucl_object_ref(o.obj)) {}

		/**
		 * Move constructor, takes ownership of the underlying reference.
		 */
		UCLPtr(UCLPtr &&o) : obj(o.obj)
		{
			o.obj = nullptr;
		}

		/**
		 * Destructor.  Drops the reference to the UCL object.
		 */
		~UCLPtr()
		{
			ucl_object_unref(const_cast<ucl_object_t *>(obj));
		}

		/**
		 * Assignment operator. Replaces currently managed object with a new
		 * one.
		 */
		UCLPtr &operator=(const ucl_object_t *o)
		{
			o = ucl_object_ref(o);
			ucl_object_unref(const_cast<ucl_object_t *>(obj));
			obj = o;
			return *this;
		}

		/**
		 * Copy assignment operator.
		 */
		UCLPtr &operator=(const UCLPtr o)
		{
			return (*this = o.obj);
		}

		/**
		 * Implicit conversion operator, returns the underlying object.
		 */
		operator const ucl_object_t *() const
		{
			return obj;
		}

		/**
		 * Comparison operator.  Returns true if `o` is a different UCL object
		 * to the one that this manages.
		 */
		bool operator!=(const ucl_object_t *o)
		{
			return obj != o;
		}

		/**
		 * Look up a property in this object by name and return a smart pointer
		 * to the new object.
		 */
		UCLPtr operator[](const char *key) const
		{
			return ucl_object_lookup(obj, key);
		}
	};

	/**
	 * Range.  Exposes a UCL collection as an iterable range of type `T`, with
	 * `Adaptor` used to convert from the underlying UCL object to `T`.  If
	 * `IterateProperties` is true then this iterates over the properties of an
	 * object, rather than just over UCL arrays.
	 */
	template<typename T, typename Adaptor = T, bool IterateProperties = false>
	class Range
	{
		/**
		 * The array that this will iterate over.
		 */
		UCLPtr array;

		/**
		 * The kind of iteration that this will perform.
		 */
		enum ucl_iterate_type iterate_type;

		/**
		 * Iterator type for this range.
		 */
		class Iter
		{
			/**
			 * The current iterator.
			 */
			ucl_object_iter_t iter{nullptr};

			/**
			 * The current object in the iteration.
			 */
			UCLPtr obj;

			/**
			 * The array that we're iterating over.
			 */
			UCLPtr array;

			/**
			 * The kind of iteration.
			 */
			enum ucl_iterate_type iterate_type;

			public:
			/**
			 * Default constructor.  Compares equal to the end iterator from any
			 * range.
			 */
			Iter() : iter(nullptr), obj(nullptr), array(nullptr) {}

			/**
			 * These iterators cannot be copy constructed.
			 */
			Iter(const Iter &) = delete;

			/**
			 * These iterators cannot be move constructed.
			 */
			Iter(Iter &&) = delete;

			/**
			 * Constructor, passed an object to iterate over and the kind of
			 * iteration to perform.
			 */
			Iter(const ucl_object_t *arr, const ucl_iterate_type type)
			  : array(arr), iterate_type(type)
			{
				// If this is not an array and we are not iterating over
				// properties then treat this as a collection of one object.
				if (!IterateProperties && (ucl_object_type(array) != UCL_ARRAY))
				{
					array = nullptr;
					obj   = arr;
					return;
				}
				iter = ucl_object_iterate_new(array);
				++(*this);
			}

			/**
			 * Arrow operator, uses `Adaptor` to expose the underlying object
			 * as if it were of type `T`.
			 */
			T operator->()
			{
				return Adaptor(obj);
			}

			/**
			 * Dereference operator, uses `Adaptor` to expose the underlying
			 * object as if it were of type `T`.
			 */
			T operator*()
			{
				return Adaptor(obj);
			}

			/**
			 * Non-equality comparison, used to terminate range-based for
			 * loops.  Compares the current iterated object.  This is typically
			 * a comparison against `nullptr` from the iterator at the end of a
			 * range.
			 */
			bool operator!=(const Iter &other)
			{
				return obj != other.obj;
			}

			/**
			 * Pre-increment operator, advances the iteration point.  If we
			 * have reached the end then the object will be `nullptr`.
			 */
			Iter &operator++()
			{
				if (iter == nullptr)
				{
					obj = nullptr;
				}
				else
				{
					obj = ucl_object_iterate_safe(iter, iterate_type);
				}
				return *this;
			}

			/**
			 * Destructor, frees any iteration state.
			 */
			~Iter()
			{
				if (iter != nullptr)
				{
					ucl_object_iterate_free(iter);
				}
			}
		};

		public:
		/**
		 * Constructor.  Constructs a range from an object.  The second
		 * argument specifies whether this should iterate over implicit arrays,
		 * explicit arrays, or both.
		 */
		Range(const ucl_object_t *   arr,
		      const ucl_iterate_type type = UCL_ITERATE_BOTH)
		  : array(arr), iterate_type(type)
		{
		}

		/**
		 * Returns an iterator to the start of the range.
		 */
		Iter begin()
		{
			return {array, iterate_type};
		}

		/**
		 * Returns an iterator to the end of the range.
		 */
		Iter end()
		{
			return {};
		}

		/**
		 * Returns true if this is an empty range.
		 */
		bool empty()
		{
			return (array == nullptr) || (ucl_object_type(array) == UCL_NULL);
		}
	};

	/**
	 * String view adaptor exposes a UCL object as a string view.
	 *
	 * Adaptors are intended to be short-lived, created only as temporaries,
	 * and must not outlive the object that they are adapting.
	 */
	class StringViewAdaptor
	{
		/**
		 * Non-owning pointer to the UCL object that this adaptor is wrapping.
		 */
		const ucl_object_t *obj;

		public:
		/**
		 * Constructor, captures a non-owning reference to a UCL object.
		 */
		StringViewAdaptor(const ucl_object_t *o) : obj(o) {}

		/**
		 * Implicit cast operator.  Returns a string view representing the
		 * object.
		 */
		operator std::string_view()
		{
			const char *cstr = ucl_object_tostring(obj);
			if (cstr != nullptr)
			{
				return cstr;
			}
			return std::string_view("", 0);
		}
	};

	/**
	 * Duration adaptor, exposes a UCL object as a duration.
	 *
	 * Adaptors are intended to be short-lived, created only as temporaries,
	 * and must not outlive the object that they are adapting.
	 */
	class DurationAdaptor
	{
		/**
		 * Non-owning pointer to the UCL object that this adaptor is wrapping.
		 */
		const ucl_object_t *obj;

		public:
		/**
		 * Constructor, captures a non-owning reference to the underling UCL
		 * object.
		 */
		DurationAdaptor(const ucl_object_t *o) : obj(o) {}

		/**
		 * Implicit cast operator, returns the underlying object as a duration
		 * in seconds.
		 */
		operator std::chrono::seconds()
		{
			assert(ucl_object_type(obj) == UCL_TIME);
			return std::chrono::seconds(
			  static_cast<long long>(ucl_object_todouble(obj)));
		}
	};

	/**
	 * Generic adaptor for number types.  Templated with the type of the number
	 * and the UCL function required to convert to a type that can represent
	 * the value.
	 *
	 * Adaptors are intended to be short-lived, created only as temporaries,
	 * and must not outlive the object that they are adapting.
	 */
	template<typename NumberType,
	         auto (*Conversion)(const ucl_object_t *) = ucl_object_toint>
	class NumberAdaptor
	{
		/**
		 * Non-owning pointer to the UCL object that this adaptor is wrapping.
		 */
		const ucl_object_t *obj;

		public:
		/**
		 * Constructor, captures a non-owning reference to the underling UCL
		 * object.
		 */
		NumberAdaptor(const ucl_object_t *o) : obj(o) {}

		/**
		 * Implicit conversion operator, returns the object represented as a
		 * `NumberType`.
		 */
		operator NumberType()
		{
			return static_cast<NumberType>(Conversion(obj));
		}
	};

	using UInt64Adaptor = NumberAdaptor<uint64_t>;
	using UInt32Adaptor = NumberAdaptor<uint32_t>;
	using UInt16Adaptor = NumberAdaptor<uint16_t>;
	using UInt8Adaptor  = NumberAdaptor<uint8_t>;
	using Int64Adaptor  = NumberAdaptor<int64_t>;
	using Int32Adaptor  = NumberAdaptor<int32_t>;
	using Int16Adaptor  = NumberAdaptor<int16_t>;
	using Int8Adaptor   = NumberAdaptor<int8_t>;
	using BoolAdaptor   = NumberAdaptor<bool, ucl_object_toboolean>;
	using FloatAdaptor  = NumberAdaptor<float, ucl_object_todouble>;
	using DoubleAdaptor = NumberAdaptor<double, ucl_object_todouble>;

	/**
	 * String for use in C++20 templates as a literal value.
	 */
	template<size_t N>
	struct StringLiteral
	{
		/**
		 * Constructor, takes a string literal and provides an object that can
		 * be used as a template parameter.
		 */
		constexpr StringLiteral(const char (&str)[N])
		{
			std::copy_n(str, N, value);
		}

		/**
		 * Implicit conversion, exposes the object as a string view.
		 */
		operator std::string_view() const
		{
			// Strip out the null terminator
			return {value, N - 1};
		}

		/**
		 * Implicit conversion, exposes the object as a C string.
		 */
		operator const char *() const
		{
			return value;
		}

		/**
		 * Storage for the string.  Template arguments are compared by
		 * structural equality and so two instances of this class will compare
		 * equal for the purpose of template instantiation of this array
		 * contains the same set of characters.
		 */
		char value[N];
	};

	/**
	 * A key-value pair of a string and an `enum` value defined in a way that
	 * allows them to be used as template argument literals.  The template
	 * arguments should never be specified directly, they should be inferred by
	 * calling the constructor.
	 */
	template<size_t Size, typename EnumType>
	struct Enum
	{
		/**
		 * Constructor, takes a string literal defining a name and the `enum`
		 * value that this maps to as arguments.
		 */
		constexpr Enum(const char (&str)[Size], EnumType e) : val(e)
		{
			std::copy_n(str, Size - 1, key_buffer);
		}

		/**
		 * Internal storage for the key.  Object literals in template arguments
		 * are compared by structural equality so two instances of this refer
		 * to the same type if they have the same character sequence here, the
		 * same `enum` type and the same value in `val`.
		 */
		char key_buffer[Size - 1] = {0};

		/**
		 * The `enum` value that this key-value pair maps to.
		 */
		EnumType val;

		/**
		 * Expose the key as a string view.
		 */
		constexpr std::string_view key() const
		{
			// Strip out the null terminator
			return {key_buffer, Size - 1};
		}
	};

	/**
	 * Compile-time map from strings to `enum` values.  Created by providing a
	 * list of `Enum` object literals as template parameters.
	 */
	template<Enum... kvp>
	class EnumValueMap
	{
		/**
		 * The type of a tuple of all of the key-value pairs in this map.
		 */
		using KVPs = std::tuple<decltype(kvp)...>;

		/**
		 * The key-value pairs.
		 */
		static constexpr KVPs kvps{kvp...};

		/**
		 * The type of a value.  All of the key-value pairs must refer to the
		 * same `enum` type.
		 */
		using Value = std::remove_reference_t<decltype(get<0>(kvps).val)>;

		/**
		 * Look up a key.  This is `constexpr` and can be compile-time
		 * evaluated.  The template parameter is the index into the key-value
		 * pair list to look.  If the lookup fails, then this will recursively
		 * invoke the same method on the next key-value pair.
		 *
		 * This is currently a linear scan, so if this is
		 * evaluated at run time then it will be O(n).  In the intended use,
		 * `n` is small and the values of the keys are sufficiently small that
		 * the comparison can be 1-2 instructions.  Compilers seem to generate
		 * very good code for this after inlining.
		 */
		template<size_t Element>
		static constexpr Value lookup(std::string_view key) noexcept
		{
			static_assert(
			  std::is_same_v<
			    Value,
			    std::remove_reference_t<decltype(get<Element>(kvps).val)>>,
			  "All entries must use the same enum value");
			// Re
			if (key == get<Element>(kvps).key())
			{
				return get<Element>(kvps).val;
			}
			if constexpr (Element + 1 < std::tuple_size_v<KVPs>)
			{
				return lookup<Element + 1>(key);
			}
			else
			{
				return static_cast<Value>(-1);
			}
		}

		public:
		/**
		 * Looks up an `enum` value by name.  This can be called at compile
		 * time.
		 */
		static constexpr Value get(std::string_view key) noexcept
		{
			return lookup<0>(key);
		}
	};

	/**
	 * Adaptor for `enum`s.  This is instantiated with the type of the `enum`
	 * and a `EnumValueMap` that maps from strings to `enum` values.
	 */
	template<typename EnumType, typename Map>
	class EnumAdaptor
	{
		/**
		 * Non-owning pointer to the UCL object that this adaptor is wrapping.
		 */
		const ucl_object_t *obj;

		public:
		/**
		 * Constructor, captures a non-owning pointer to the underlying object.
		 */
		EnumAdaptor(const ucl_object_t *o) : obj(o) {}

		/**
		 * Implicit conversion, extracts the string value and converts it to an
		 * `enum` using the compile-time map provided as the second template
		 * parameter.
		 */
		operator EnumType()
		{
			return Map::get(ucl_object_tostring(obj));
		}
	};

	/**
	 * Class representing a compile-time mapping from a string to a type.  This
	 * is used to generate compile-time maps from keys to types.
	 */
	template<StringLiteral Key, typename Adaptor>
	struct NamedType
	{
		/**
		 * Given a UCL object, construct an `Adaptor` from that object.
		 */
		static Adaptor construct(const ucl_object_t *obj)
		{
			return Adaptor{obj};
		}

		/**
		 * Return the key as a string view.
		 */
		static constexpr std::string_view key()
		{
			return Key;
		}
	};

	/**
	 * Named type adaptor.  This is used to represent UCL objects that have a
	 * field whose name describes their type.  The template parameters are the
	 * name of the key that defines the type and a list of `NamedType`s that
	 * represent the set of valid values and the types that they represent.
	 *
	 * This exposes a visitor interface, where callbacks for each of the
	 * possible types can be provided, giving a match-like API.
	 */
	template<StringLiteral KeyName, typename... Types>
	class NamedTypeAdaptor
	{
		/**
		 * The underlying object that this represents.
		 */
		UCLPtr obj;

		/**
		 * A type describing all of the string-to-type maps as a tuple.
		 */
		using TypesAsTuple = std::tuple<Types...>;

		/**
		 * Implementation of the visitor.  Recursive template that inspects
		 * each of the possible values in the names map and invokes the visitor
		 * with the matching type.
		 */
		template<typename T, size_t I = 0>
		constexpr void visit_impl(std::string_view key, T &&v)
		{
			if (key == std::tuple_element_t<I, TypesAsTuple>::key())
			{
				return v(std::tuple_element_t<I, TypesAsTuple>::construct(obj));
			}
			if constexpr (I + 1 < std::tuple_size_v<TypesAsTuple>)
			{
				return visit_impl<T, I + 1>(key, std::forward<T &&>(v));
			}
		}

		/**
		 * Dispatch helper.  This is instantiated with an array of lambdas, one
		 * for each matched type.  It will expose all of their call operators
		 * as different overloads.
		 *
		 * Taken from an example on cppreference.com.
		 */
		template<class... Fs>
		struct Dispatcher : Fs...
		{
			using Fs::operator()...;
		};

		/**
		 * Deduction guide for Dispatcher.
		 */
		template<class... Ts>
		Dispatcher(Ts &&...) -> Dispatcher<std::remove_reference_t<Ts>...>;

		public:
		/**
		 * Constructor, captures a non-owning reference to a UCL object.
		 */
		NamedTypeAdaptor(const ucl_object_t *o) : obj(o) {}

		/**
		 * Visit.  Can be called in two configurations, either with a single
		 * visitor class, or with an array of lambdas each taking one of the
		 * possible types as its sole argument.
		 *
		 * Either the explicit visitor or the lambda must cover all cases.
		 * This can be done with an `auto` parameter overload as the fallback.
		 *
		 * WARNING: This cannot be called with a visitor class *and* lambdas,
		 * because it will attempt to copy the visitor and so any state will
		 * not be updated.
		 */
		template<typename... Visitors>
		auto visit(Visitors &&...visitors)
		{
			/**
			 * If this is called with a single visitor, use it directly.
			 */
			if constexpr (sizeof...(Visitors) == 1)
			{
				return visit_impl(StringViewAdaptor(obj[KeyName]), visitors...);
			}
			else
			{
				return visit_impl(StringViewAdaptor(obj[KeyName]),
				                  Dispatcher{visitors...});
			}
		}

		/**
		 * Visit.  This should be called with a list of lambdas as the
		 * argument.  Each lambda should take one of the types.  Unlike the
		 * `visit` method, this does not need to cover all cases.  Any cases
		 * that are not handled will be silently ignored.
		 */
		template<typename... Visitors>
		auto visit_some(Visitors &&...visitors)
		{
			return visit_impl(StringViewAdaptor(obj[KeyName]),
			                  Dispatcher{visitors..., [](auto &&) {}});
		}
	};

	/**
	 * Property adaptor.  Wraps some other adaptor for the value and also
	 * provides an accessor for the key.
	 */
	template<typename Adaptor>
	class PropertyAdaptor : public Adaptor
	{
		public:
		/**
		 * Constructor, captures a non-owning reference to a UCL object.
		 */
		PropertyAdaptor(const ucl_object_t *o) : Adaptor(o) {}

		/**
		 * Provide the key as a string view.
		 */
		std::string_view key()
		{
			return ucl_object_key(Adaptor::obj);
		}
	};

	/**
	 * Helper to construct a value with an adaptor if it exists.  If `o` is not
	 * null, uses `Adaptor` to construct an instance of `T`.  Returns an
	 * `optional<T>`, where the value is present if `o` is not null.
	 */
	template<typename Adaptor, typename T = Adaptor>
	std::optional<T> make_optional(const ucl_object_t *o)
	{
		if (o == nullptr)
		{
			return {};
		}
		return Adaptor(o);
	}

} // namespace CONFIG_DETAIL_NAMESPACE
