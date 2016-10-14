// Valarray.h

#ifndef _Valarray_h
#define _Valarray_h
#include <chrono>
#include <complex>
#include <cstdint>
#include <future>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <limits>
#include <initializer_list>
#include <typeinfo>
#include <cstdint>
#include <complex>
#include <vector>
// zrdw::vector
#include "Vector.h"

namespace zrdw {
	//using std::vector; //during development and testing
	using zrdw::vector;

	template <typename T, typename Expr> //prototype declared here, defined later
	struct valarray;

	//namespace zrdw_hide(my uteid) hides all supporting templates, structs, etc from outside users
	namespace zrdw_hide {
		using namespace std::rel_ops;
		/*
		Scalar is used to expand scalar base type, either int, float, double, complex<T> into a valarray-like obj that
		can call operator[], and size().
		*/
		template <typename T>
		struct scalar {
			using value_type = T;
			const T k;
			scalar(const T& scalar) : k(scalar) {}
			T operator[](int64_t) const {
				return k;
			}
			size_t size() const {
				return static_cast<size_t>(std::numeric_limits<int64_t>::max());
			}
		};

		/*
		emptyOperand is used to support unary operations in Proxy, see Proxy for details.
		*/
		struct emptyOperand {
			using value_type = emptyOperand;
			size_t size() const {
				return static_cast<size_t>(std::numeric_limits<int64_t>::max());
			}  
		};

		/*
		To decided the return type of metafunctions, we need to know how to deduce it at compile time
		credit to Dr. Chase at UT in class 2015
		*/
		template <typename T> struct rank { static constexpr int value = -1; }; //all foo types ranked negative
		template <> struct rank<emptyOperand> { static constexpr int value = 0; };
		template <> struct rank<int> { static constexpr int value = 1; };
		template <> struct rank<float> { static constexpr int value = 2; };
		template <> struct rank<double> { static constexpr int value = 3; };
		template <typename T> struct rank<std::complex<T>> { static constexpr int value = rank<T>::value; };
		template <typename T, typename Expr> struct rank<valarray<T, Expr>> { static constexpr int value = rank<T>::value; };

		template <int R> struct stype { using type = void; }; //all foo types deduced to void
		template <> struct stype<1> { using type = int; static constexpr bool allowed = true; };
		template <> struct stype<2> { using type = float; static constexpr bool allowed = true; };
		template <> struct stype<3> { using type = double; static constexpr bool allowed = true; };

		template <typename T> struct is_complex : public std::false_type {};
		template <typename T> struct is_complex<std::complex<T>> : public std::true_type{};
		template <typename T, typename Expr> struct is_complex<valarray<std::complex<T>, Expr>> : public std::true_type{};

		template <bool p, typename T> struct ctype;
		template <typename T> struct ctype<true, T> { using type = std::complex<T>; };
		template <typename T> struct ctype<false, T> { using type = T; };

		template <typename T1, typename T2>
		struct choose_type {
			static constexpr int t1_rank = rank<T1>::value;
			static constexpr int t2_rank = rank<T2>::value;
			static constexpr int max_rank = (t1_rank < t2_rank) ? t2_rank : t1_rank;
			using my_stype = typename stype<max_rank>::type;
			static constexpr bool t1_comp = is_complex<T1>::value;
			static constexpr bool t2_comp = is_complex<T2>::value;
			static constexpr bool my_comp = t1_comp || t2_comp;
			using type = typename ctype<my_comp, my_stype>::type;
		};

		/*
		for Proxy's operands, if it's a vector valarray then just using referece, if it's a Proxy valarray, then copy it
		this is to avoid the compiler destroy Proxy operand before we use it, which is a potential defect when using lazy-evaluation.
		*/
		template <typename T>
		struct choose_operand_type { using type = const T; };
		template <typename T>
		struct choose_operand_type<valarray<T, vector<T>>> { using type = const vector<T>&; };
		template <typename T> //using copy, scalar is temp created by operator functions
		struct choose_operand_type<scalar<T>> { using type = const scalar<T>; };

		/*
		full-functionalities random-access iterator template for proxy, should be const_iterator
		*/
		template <typename T, typename P>
		class proxyIterator { //const_iterator
		public:
			using difference_type = int64_t;
			using value_type = T;
			using iterator_category = std::random_access_iterator_tag; //or sth else
			using pointer = T*;
			using reference = T&;
			using iterator = proxyIterator<T, P>;
			P* proxy;
			int64_t position;
			T structure_deref_temp;

			proxyIterator(P& p, int64_t k = 0) : proxy(&p), position(k) {}
			proxyIterator(const iterator& iter, int64_t k = 0) : proxy(iter.proxy), position(iter.position + k) {}
			//maybe need move ctor for faster copy of proxy, let me consider it later --- not really useful

			iterator& operator=(const iterator& iter) {
				this->proxy = iter.proxy;
				this->position = iter.position;
				return *this;
			}

			bool operator==(const iterator& iter) const {
				if (this->proxy != iter.proxy) throw std::runtime_error("Comparing two different type iterators");
				return this->position == iter.position;
			}

			bool operator<(const iterator& iter) const {
				if (this->proxy != iter.proxy) throw std::runtime_error("Comparing two different type iterators");
				return this->position < iter.position;
			}

			iterator operator+(int64_t k) const {
				return iterator(*this, k);
			}

			iterator operator-(int64_t k) const {
				return iterator(*this, -k);
			}

			int64_t operator-(const iterator& iter) const {
				return this->position - iter.position;
			}

			iterator& operator++(void) {
				++position;
				return *this;
			}

			iterator operator++(int) {
				iterator temp(*this);
				++(*this);
				return temp;
			}

			iterator& operator--(void) {
				--position;
				return *this;
			}

			iterator operator--(int) {
				iterator temp(*this);
				--(*this);
				return temp;
			}

			iterator& operator+=(int64_t k) {
				position += k;
				return *this;
			}

			iterator& operator-=(int64_t k) {
				position -= k;
				return *this;
			}

			//if T is complex, return value must be copied, not refered, since the return var is a temp
			const T operator*(void) const {
				return proxy->operator[](position);
			}
			const T* operator->(void) { // not const because of specialty of complex
				structure_deref_temp = (proxy->operator[](position)); //complex returns a temp obj, must copy to a member var
				return &structure_deref_temp;
			}
			const T operator[](int64_t k) const {
				return proxy->operator[](position + k);
			}

			friend iterator operator+(int64_t k, iterator& iter) {
				return iterator(iter, k);
			}

			friend void swap(iterator& iter1, iterator& iter2) {
				iterator temp(iter1);
				iter1 = iter2;
				iter2 = temp;
			}
		};

		/*
		Proxy is used to wrap the operands, support binary and unary(with right=emptyOperand) at the same time
		*/
		template <typename Operation, typename Left, typename Right = emptyOperand>
		struct Proxy {
			using T1 = typename Left::value_type;
			using T2 = typename Right::value_type;
			using value_type = typename choose_type<T1, T2>::type; // may differ from result_type
			using result_type = typename Operation::result_type; //result type after apply operation
			using L = typename choose_operand_type<Left>::type;
			using R = typename choose_operand_type<Right>::type;

			L l; //L is const
			R r; //R is const
			const Operation f;

			//parameter list L and R must be pass-by-ref ???
			Proxy(const Operation _f, const L& _l, const R& _r) : f(_f), l(_l), r(_r) {}

			template <typename SecondOperand> //evaluate binary operands operation
			result_type evaluate(int64_t k, const SecondOperand&) const {
				return static_cast<result_type>(f(this->l[k], this->r[k]));
			}

			result_type evaluate(int64_t k, const emptyOperand&) const { //evaluate unary operand operation
				return static_cast<result_type>(f(this->l[k]));
			}

			result_type operator[](int64_t k) const {
				return this->evaluate(k, r);
			}

			size_t size() const { //must use size_t, otherwise there will be int, unsigned int type ambiguity
				return static_cast<size_t>((l.size()<r.size()) ? l.size() : r.size());
			}

			//iterator
			using iterator = proxyIterator<result_type, Proxy<Operation, Left, Right>>;
			iterator begin() { return iterator(*this); }
			iterator end() { return iterator(*this, this->size()); }
		};

		//register supported valarray maths here
		enum OP { neg, add, sub, mul, div };
		template <int F, typename T> struct operation;
		template <typename T> struct operation <0, T> { using type = std::negate<T>; };
		template <typename T> struct operation <1, T> { using type = std::plus<T>; };
		template <typename T> struct operation <2, T> { using type = std::minus<T>; };
		template <typename T> struct operation <3, T> { using type = std::multiplies<T>; };
		template <typename T> struct operation <4, T> { using type = std::divides<T>; };

		//check if is_valarray_math, two aspects: is valarray or not, is valid valarray maths operand or not
		template <typename T> struct is_valarray : public std::false_type { static constexpr bool do_maths = false; };
		template <> struct is_valarray<emptyOperand> : public std::false_type{ static constexpr bool do_maths = true; };
		template <> struct is_valarray<int> : public std::false_type{ static constexpr bool do_maths = true; };
		template <> struct is_valarray<float> : public std::false_type{ static constexpr bool do_maths = true; };
		template <> struct is_valarray<double> : public std::false_type{ static constexpr bool do_maths = true; };
		template <typename T>
		struct is_valarray<std::complex<T>> : public std::false_type{ static constexpr bool do_maths = true; };
		template <typename T, typename Expr>
		struct is_valarray<valarray<T, Expr>> : public std::true_type{ static constexpr bool do_maths = true; };

		//check if is valid valarray maths
		template <typename T1, typename T2>
		struct is_val_maths {
			static constexpr bool t1_is_valarray = is_valarray<T1>::value;
			static constexpr bool t2_is_valarray = is_valarray<T2>::value;
			static constexpr bool t1_can_do_maths = is_valarray<T1>::do_maths;
			static constexpr bool t2_can_do_maths = is_valarray<T2>::do_maths;
			static constexpr bool value = (t1_is_valarray || t2_is_valarray) && (t1_can_do_maths && t2_can_do_maths);
		};

		//make scalar compatible to do maths with valarry
		template <bool p, typename T> struct valarray_lize;
		template <typename T> struct valarray_lize<true, T> { using type = T; };
		template <> struct valarray_lize<false, emptyOperand> { using type = emptyOperand; };
		template <typename T> struct valarray_lize<false, T> { using type = scalar<T>; };

		//resolve valarray math return type and as an maths adaptor to do lazy maths  
		template <int f, typename T1, typename T2>
		struct maths_retType {
			using type = typename choose_type<T1, T2>::type;
			using F_type = typename operation<f, type>::type;
			using T1_valarray = typename valarray_lize<is_valarray<T1>::value, T1>::type;
			using T2_valarray = typename valarray_lize<is_valarray<T2>::value, T2>::type;
			using retType = valarray<type, Proxy<F_type, T1_valarray, T2_valarray>>;
			using T1_Type = typename choose_operand_type<T1_valarray>::type;
			using T2_Type = typename choose_operand_type<T2_valarray>::type;
			T1_Type l;
			T2_Type r;
			maths_retType(const T1& _l, const T2& _r) : l(T1_Type(_l)), r(T2_Type(_r)) {}
			retType operator()() { return retType(F_type(), l, r); } //do lazy maths
		};

		//enable_if
		template <bool, typename T> struct enable_if;
		template <typename T> struct enable_if<true, T> { using type = T; };
		template <typename T> struct enable_if<false, T> {};
		//short form of enable_if
		template <int f, typename T1, typename T2>
		using Enable_if = typename enable_if<is_val_maths<T1, T2>::value, typename maths_retType<f, T1, T2>::retType>::type;
	}

	using namespace zw3425;

	//supported maths defined here: neg, add, sub, mul, div
	template <typename T1>
	Enable_if<OP::neg, T1, emptyOperand> operator-(const T1& l) {
		return maths_retType<OP::neg, T1, emptyOperand>(l, emptyOperand())();
	}

	template <typename T1, typename T2>
	Enable_if<OP::add, T1, T2> operator+(const T1& l, const T2& r) {
		return maths_retType<OP::add, T1, T2>(l, r)();
	}

	template <typename T1, typename T2>
	Enable_if<OP::sub, T1, T2> operator-(const T1& l, const T2& r) {
		return maths_retType<OP::sub, T1, T2>(l, r)();
	}

	template <typename T1, typename T2>
	Enable_if<OP::mul, T1, T2> operator*(const T1& l, const T2& r) {
		return maths_retType<OP::mul, T1, T2>(l, r)();
	}

	template <typename T1, typename T2>
	Enable_if<OP::div, T1, T2> operator/(const T1& l, const T2& r) {
		return maths_retType<OP::div, T1, T2>(l, r)();
	}

	//ostream for valarray
	template <typename T, typename Expr>
	std::ostream& operator<<(std::ostream& os, const valarray<T, Expr>& v) {
		int64_t size = v.size();
		os << '{';
		for (int64_t i = 0; i < size; ++i) {
			switch (i) {
			case 0: os << v[i]; break;
			default: os << ',' << v[i]; break;
			}
		}
		os << '}';
		return os;
	}

	//Sqrt wraps std::sqrt, makes it a functor
	template <typename T>
	struct Sqrt {
		static constexpr bool comp = is_complex<T>::value;
		using result_type = typename ctype<comp, double>::type;
		using argument_type = T;
		result_type operator() (const T& x) const {
			return std::sqrt(x);
		}
	};

	//valarray
	// if Expr the valarray wraps is at the its first level,
	// i.e. if Expr is vector<T>, then Expr does not need to be explicitly designated, else, Expr is explicitly designated as a kind of Proxy
	template <typename T, typename Expr = vector<T>>
	struct valarray : public Expr {
	public:
		using value_type = T;
		const bool is_allowed = stype<rank<T>::value>::allowed; //to forbid valarray<foo>

		valarray() : Expr() {}
		explicit valarray(int64_t n) : Expr(n) {}
		valarray(std::initializer_list<T> lst) : Expr(lst) { /*cout << "list-init" << endl;*/ }
		valarray(const valarray& val) : Expr(val) {}

		//ctor for vector from convertible valarray of vector or proxy
		template <typename T1, typename Expr1>
		valarray(const valarray<T1, Expr1>& val) { //cout << "ctor from vector" << endl;
			int64_t size = val.size();
			for (int64_t i = 0; i<size; ++i) {
				this->push_back(static_cast<T>(val[i]));
			}
		}

		//ctor for cases derived from Proxy
		template <typename Operation, typename Left, typename Right>
		valarray(const Operation _f, const Left& _l, const Right& _r) : Expr(_f, _l, _r) { /*cout << "proxy init" << endl;*/ }

		template <typename T1, typename Expr1>
		valarray& assignment(const valarray<T1, Expr1>& v) {
			uint64_t size = (this->size()<v.size()) ? this->size() : v.size();
			for (uint64_t i = this->size(); i > size; --i) {
				this->pop_back();
			}
			for (uint64_t i = 0; i<size; ++i) {
				(*this)[i] = static_cast<T>(v[i]);
			}
			return *this;
		}

		valarray& operator=(const valarray& v) { //always take the smaller size
			return assignment(v);
		}

		//copy assignment from a convertible valarray of vector or proxy, expression template evaluated
		template <typename T1, typename Expr1>
		valarray& operator=(const valarray<T1, Expr1>& v) { //always take the smaller size
			return assignment(v);
		}

		//change/init the value of each elem
		valarray& operator=(const T t) {
			int64_t size = this->size();
			for (int64_t i = 0; i < size; ++i) {
				(*this)[i] = static_cast<T>(t);
			}
			return *this;
		}

		//accumulate using given function object
		template <typename F, typename Type = typename F::result_type>
		Type accumulate(F f) {
			if (this->size() == 0) return T(); //no elem, return default zero-init value as return value
			Type sum = static_cast<Type>((*this)[0]); //init to the first elem, for both + and * ...
			int64_t size = this->size();
			for (int64_t i = 1; i<size; ++i) {
				sum = f(sum, (*this)[i]);
			}
			return sum;
		}

		//sum up elements
		T sum() {
			return accumulate(std::plus<T>());
		}

		//apply a unary function to valarray elements
		template <typename Func, typename Type = typename Func::result_type>
		valarray<Type, Proxy<Func, valarray<T, Expr>>> apply(Func f) {
			return valarray<Type, Proxy<Func, valarray<T, Expr>>>(f, *this, emptyOperand());
		}

		//calc square root of each element
		template <typename Type = typename Sqrt<T>::result_type>
		valarray<Type, Proxy<Sqrt<T>, valarray<T, Expr>>> sqrt() {
			return apply(Sqrt<T>());
		}
	};
};
#endif /* _Valarray_h */



/*
the following code are previous operator overloading functions, which have now been replaced by meta functions
*/
////plus
//template <typename T1, typename Expr1, typename T2, typename Expr2, typename Type = typename choose_type<T1, T2>::type>
//valarray<Type, Proxy<std::plus<Type>, valarray<T1, Expr1>, valarray<T2, Expr2>>> operator+(const valarray<T1, Expr1>& l, const valarray<T2, Expr2>& r) {
//	return valarray<Type, Proxy<std::plus<Type>, valarray<T1, Expr1>, valarray<T2, Expr2>>>(std::plus<Type>(), l, r);
//}
//template <typename T1, typename Expr1, typename K2, typename Type = typename choose_type<T1, K2>::type>
//valarray<Type, Proxy<std::plus<Type>, valarray<T1, Expr1>, scalar<K2>>> operator+(const valarray<T1, Expr1>& l, const K2 r) {
//	return valarray<Type, Proxy<std::plus<Type>, valarray<T1, Expr1>, scalar<K2>>>(std::plus<Type>(), l, scalar<K2>(r));
//}
//template <typename T1, typename Expr1, typename K2, typename Type = typename choose_type<T1, K2>::type>
//valarray<Type, Proxy<std::plus<Type>, scalar<K2>, valarray<T1, Expr1>>> operator+(const K2 r, const valarray<T1, Expr1>& l) {
//	return valarray<Type, Proxy<std::plus<Type>, scalar<K2>, valarray<T1, Expr1>>>(std::plus<Type>(), scalar<K2>(r), l);
//}

////minus
//template <typename T1, typename Expr1, typename T2, typename Expr2, typename Type = typename choose_type<T1, T2>::type>
//valarray<Type, Proxy<std::minus<Type>, valarray<T1, Expr1>, valarray<T2, Expr2>>> operator-(const valarray<T1, Expr1>& l, const valarray<T2, Expr2>& r) {
//	return valarray<Type, Proxy<std::minus<Type>, valarray<T1, Expr1>, valarray<T2, Expr2>>>(std::minus<Type>(), l, r);
//}
//template <typename T1, typename Expr1, typename K2, typename Type = typename choose_type<T1, K2>::type>
//valarray<Type, Proxy<std::minus<Type>, valarray<T1, Expr1>, scalar<K2>>> operator-(const valarray<T1, Expr1>& l, const K2 r) {
//	return valarray<Type, Proxy<std::minus<Type>, valarray<T1, Expr1>, scalar<K2>>>(std::minus<Type>(), l, scalar<K2>(r));
//}
//template <typename T1, typename Expr1, typename K2, typename Type = typename choose_type<T1, K2>::type>
//valarray<Type, Proxy<std::minus<Type>, scalar<K2>, valarray<T1, Expr1>>> operator-(const K2 r, const valarray<T1, Expr1>& l) {
//	return valarray<Type, Proxy<std::minus<Type>, scalar<K2>, valarray<T1, Expr1>>>(std::minus<Type>(), scalar<K2>(r), l);
//}

////multiply
//template <typename T1, typename Expr1, typename T2, typename Expr2, typename Type = typename choose_type<T1, T2>::type>
//valarray<Type, Proxy<std::multiplies<Type>, valarray<T1, Expr1>, valarray<T2, Expr2>>> operator*(const valarray<T1, Expr1>& l, const valarray<T2, Expr2>& r) {
//	return valarray<Type, Proxy<std::multiplies<Type>, valarray<T1, Expr1>, valarray<T2, Expr2>>>(std::multiplies<Type>(), l, r);
//}
//template <typename T1, typename Expr1, typename K2, typename Type = typename choose_type<T1, K2>::type>
//valarray<Type, Proxy<std::multiplies<Type>, valarray<T1, Expr1>, scalar<K2>>> operator*(const valarray<T1, Expr1>& l, const K2 r) {
//	return valarray<Type, Proxy<std::multiplies<Type>, valarray<T1, Expr1>, scalar<K2>>>(std::multiplies<Type>(), l, scalar<K2>(r));
//}
//template <typename T1, typename Expr1, typename K2, typename Type = typename choose_type<T1, K2>::type>
//valarray<Type, Proxy<std::multiplies<Type>, scalar<K2>, valarray<T1, Expr1>>> operator*(const K2 r, const valarray<T1, Expr1>& l) {
//	return valarray<Type, Proxy<std::multiplies<Type>, scalar<K2>, valarray<T1, Expr1>>>(std::multiplies<Type>(), scalar<K2>(r), l);
//}

////divide
//template <typename T1, typename Expr1, typename T2, typename Expr2, typename Type = typename choose_type<T1, T2>::type>
//valarray<Type, Proxy<std::divides<Type>, valarray<T1, Expr1>, valarray<T2, Expr2>>> operator/(const valarray<T1, Expr1>& l, const valarray<T2, Expr2>& r) {
//	return valarray<Type, Proxy<std::divides<Type>, valarray<T1, Expr1>, valarray<T2, Expr2>>>(std::divides<Type>(), l, r);
//}
//template <typename T1, typename Expr1, typename K2, typename Type = typename choose_type<T1, K2>::type>
//valarray<Type, Proxy<std::divides<Type>, valarray<T1, Expr1>, scalar<K2>>> operator/(const valarray<T1, Expr1>& l, const K2 r) {
//	return valarray<Type, Proxy<std::divides<Type>, valarray<T1, Expr1>, scalar<K2>>>(std::divides<Type>(), l, scalar<K2>(r));
//}
//template <typename T1, typename Expr1, typename K2, typename Type = typename choose_type<T1, K2>::type>
//valarray<Type, Proxy<std::divides<Type>, scalar<K2>, valarray<T1, Expr1>>> operator/(const K2 r, const valarray<T1, Expr1>& l) {
//	return valarray<Type, Proxy<std::divides<Type>, scalar<K2>, valarray<T1, Expr1>>>(std::divides<Type>(), scalar<K2>(r), l);
//}