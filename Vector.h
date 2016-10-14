#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace zrdw {


	//rewritten std::rel_ops to accept different types comparison
	template<class _Ty, class _Tx> inline
		bool operator!=(const _Ty& _Left, const _Tx& _Right)
	{	// test for inequality, in terms of equality
		return (!(_Left == _Right));
	}

	template<class _Ty, class _Tx> inline
		bool operator>(const _Ty& _Left, const _Tx& _Right)
	{	// test if _Left > _Right, in terms of operator<
		return (_Right < _Left);
	}

	template<class _Ty, class _Tx> inline
		bool operator<=(const _Ty& _Left, const _Tx& _Right)
	{	// test if _Left <= _Right, in terms of operator<
		return (!(_Right < _Left));
	}

	template<class _Ty, class _Tx> inline
		bool operator>=(const _Ty& _Left, const _Tx& _Right)
	{	// test if _Left >= _Right, in terms of operator<
		return (!(_Left < _Right));
	}

	template<class _Ty> inline
		bool operator!=(const _Ty& _Left, const _Ty& _Right)
	{	// test for inequality, in terms of equality
		return (!(_Left == _Right));
	}

	template<class _Ty> inline
		bool operator>(const _Ty& _Left, const _Ty& _Right)
	{	// test if _Left > _Right, in terms of operator<
		return (_Right < _Left);
	}

	template<class _Ty> inline
		bool operator<=(const _Ty& _Left, const _Ty& _Right)
	{	// test if _Left <= _Right, in terms of operator<
		return (!(_Right < _Left));
	}

	template<class _Ty> inline
		bool operator>=(const _Ty& _Left, const _Ty& _Right)
	{	// test if _Left >= _Right, in terms of operator<
		return (!(_Left < _Right));
	}


	class invalid_iterator {
	public:
		enum SeverityLevel { SEVERE, MODERATE, MILD, WARNING };
		SeverityLevel level;

		invalid_iterator(SeverityLevel level = SEVERE) { this->level = level; }
		virtual const char* what() const {
			switch (level) {
			case WARNING:   return "Warning"; // not used
			case MILD:      return "Mild";
			case MODERATE:  return "Moderate";
			case SEVERE:    return "Severe";
			default:        return "ERROR"; // should not be used
			}
		}
	};

	template <typename T>
	class vector {
		const int64_t size_init = 8;
	private:
		T* head;
		T* front; //points to the first elem in the vector
		int64_t cap_front, cap_rear;
		int64_t len_Vector, len_elem;
		int64_t modify_version = 0;
		int64_t realloc_reassign_version = 0;

		void copy(const vector& v) {
			head = (T*) ::operator new(v.len_Vector*sizeof(T));

			this->cap_front = v.cap_front;
			this->cap_rear = v.cap_rear;
			this->len_Vector = v.len_Vector;
			this->len_elem = v.len_elem;

			front = head + this->cap_front;
			for (int64_t i = 0; i < len_elem; ++i) {
				new (front + i) T{ v.front[i] };
			}
		}

		void destroy(void) {
			for (int64_t i = 0; i < len_elem; i++) { // destruct each elem in vector
				front[i].~T();
			}
			::operator delete(head);
			head = front = nullptr; //?
			cap_front = cap_rear = len_Vector = len_elem = 0;

			// vector being invalidated, otherwise should be invalidated at each caller
			// ++modify_version;
			// ++realloc_reassign_version;
		}

	public:
		vector(void) {
			head = (T*) ::operator new(size_init*sizeof(T));
			front = head;
			cap_front = 0;
			cap_rear = size_init;
			len_Vector = size_init;
			len_elem = 0;

			modify_version = realloc_reassign_version = 0;
		}

		explicit vector(int64_t n) {
			if (n <= 0) throw std::out_of_range("In explicit constructor n<=0");
			else {
				head = (T*) ::operator new(n*sizeof(T));
				front = head;
				for (int64_t i = 0; i < n; i++) {
					new (front + i) T{};
				}
				cap_front = 0;
				cap_rear = 0;
				len_Vector = n;
				len_elem = n;

				modify_version = realloc_reassign_version = 0;
			}
		}

		// copy ctor
		vector(const vector& v) {
			copy(v);

			this->modify_version = this->realloc_reassign_version = 0;
		}

		// copy assign
		vector& operator=(const vector& v) {
			if (this != &v) {
				destroy();
				copy(v);

				++(this->modify_version);
				++(this->realloc_reassign_version);
			}
			return *this;
		}

		// move ctor
		vector(vector&& v) {
			this->head = v.head;
			this->front = v.front;
			this->cap_front = v.cap_front;
			this->cap_rear = v.cap_rear;
			this->len_elem = v.len_elem;
			this->len_Vector = v.len_Vector;

			this->modify_version = this->realloc_reassign_version = 0;

			v.head = v.front = nullptr;
			v.cap_front = v.cap_rear = v.len_elem = v.len_Vector = 0;

			// the moved-from vector being invalidated
			++v.modify_version;
			++v.realloc_reassign_version;
		}

		//move assign
		vector& operator=(vector&& v) {
			if (this != &v) {
				destroy();
				this->head = v.head;
				this->front = v.front;
				this->cap_front = v.cap_front;
				this->cap_rear = v.cap_rear;
				this->len_elem = v.len_elem;
				this->len_Vector = v.len_Vector;

				++(this->modify_version);
				++(this->realloc_reassign_version);

				v.head = v.front = nullptr;
				v.cap_front = v.cap_rear = v.len_elem = v.len_Vector = 0;

				// the moved-from vector being invalidated
				++v.modify_version;
				++v.realloc_reassign_version;
			}
			return *this;
		}

		~vector(void) {
			destroy();

			++modify_version;
			++realloc_reassign_version;
		}

		int64_t size(void) const {
			return len_elem;
		}

		T& operator[](int64_t k) {
			if (k >= len_elem || k<0) throw std::out_of_range("Index out of range in vector[]");
			return *(front + k);
			//return front[k];
		}

		const T& operator[](int64_t k) const {
			if (k >= len_elem || k<0) throw std::out_of_range("Index out of range in vector[]");
			return *(front + k);
			//return front[k];
		}

		void push_back(const T& e) {
			if (cap_rear < 0) throw std::out_of_range("cap_rear<0 in push_back");
			if (cap_rear == 0) { // realloc
				T* old_head = head;
				T* old_front = front;
				T* temp_head = (T*) ::operator new(2 * len_Vector*sizeof(T));
				T* temp_front = temp_head + cap_front;


				head = temp_head;
				front = temp_front;
				new (front + len_elem) T{ e };

				for (int64_t i = 0; i < len_elem; i++) {
					new (temp_front + i) T{ std::move(old_front[i]) }; //std::move, xvalue
				}

				cap_rear = len_Vector;
				len_Vector *= 2;

				for (int64_t i = 0; i < len_elem; i++) {
					old_front[i].~T();
				}
				::operator delete(old_head);
				old_head = old_front = nullptr;

				len_elem++;
				cap_rear--;

				++modify_version;
				++realloc_reassign_version;
			}
			else {
				new (front + len_elem) T{ e };
				len_elem++;
				cap_rear--;

				++modify_version;
			}
		}

		void push_back(T&& e) {
			if (cap_rear < 0) throw std::out_of_range("cap_rear<0 in push_back");
			if (cap_rear == 0) {
				T* old_head = head;
				T* old_front = front;
				T* temp_head = (T*) ::operator new(2 * len_Vector*sizeof(T));
				T* temp_front = temp_head + cap_front;


				head = temp_head;
				front = temp_front;
				new (front + len_elem) T{ std::move(e) };

				for (int64_t i = 0; i < len_elem; i++) {
					new (temp_front + i) T{ std::move(old_front[i]) }; //std::move, xvalue
				}

				cap_rear = len_Vector;
				len_Vector *= 2;

				for (int64_t i = 0; i < len_elem; i++) {
					old_front[i].~T();
				}
				::operator delete(old_head);
				old_head = old_front = nullptr;

				len_elem++;
				cap_rear--;

				++modify_version;
				++realloc_reassign_version;
			}
			else {
				new (front + len_elem) T{ std::move(e) };
				len_elem++;
				cap_rear--;

				++modify_version;
			}
		}

		void push_front(const T& e) {
			if (cap_front < 0) throw std::out_of_range("cap_front<0 in push_front");
			if (cap_front == 0) {
				//if (head != front) std::cout << "INTERESTING..." << std::endl;
				T* old_head = head;
				T* old_front = front;
				T* temp_head = (T*) ::operator new(2 * len_Vector*sizeof(T));
				T* temp_front = temp_head + len_Vector;

				head = temp_head;
				front = temp_front;
				front--;
				new (front) T{ e };

				for (int64_t i = 0; i < len_elem; i++) {
					new (temp_front + i) T{ std::move(old_front[i]) }; //std::move, xvalue
				}

				cap_front = len_Vector;
				len_Vector *= 2;

				for (int64_t i = 0; i < len_elem; i++) {
					old_front[i].~T();
				}
				::operator delete(old_head);
				old_head = old_front = nullptr;

				cap_front--;
				len_elem++;

				++modify_version;
				++realloc_reassign_version;
			}
			else {
				front--;
				new (front) T{ e };
				cap_front--;
				len_elem++;

				++modify_version;
			}
		}

		void push_front(T&& e) {
			if (cap_front < 0) throw std::out_of_range("cap_front<0 in push_front");
			if (cap_front == 0) {
				//if (head != front) std::cout << "INTERESTING..." << std::endl;
				T* old_head = head;
				T* old_front = front;
				T* temp_head = (T*) ::operator new(2 * len_Vector*sizeof(T));
				T* temp_front = temp_head + len_Vector;

				head = temp_head;
				front = temp_front;
				front--;
				new (front) T{ std::move(e) };

				for (int64_t i = 0; i < len_elem; i++) {
					new (temp_front + i) T{ std::move(old_front[i]) }; //std::move, xvalue
				}

				cap_front = len_Vector;
				len_Vector *= 2;

				for (int64_t i = 0; i < len_elem; i++) {
					old_front[i].~T();
				}
				::operator delete(old_head);
				old_head = old_front = nullptr;

				cap_front--;
				len_elem++;

				++modify_version;
				++realloc_reassign_version;
			}
			else {
				front--;
				new (front) T{ std::move(e) };
				cap_front--;
				len_elem++;

				++modify_version;
			}
		}

		void pop_back(void) {
			if (len_elem <= 0) throw std::out_of_range("Index out of range in pop_back");
			front[len_elem - 1].~T();
			len_elem--;
			cap_rear++;

			++modify_version;
		}

		void pop_front(void) {
			if (len_elem <= 0) throw std::out_of_range("Index out of range in pop_back");
			front[0].~T();
			front++;
			len_elem--;
			cap_front++;

			++modify_version;
		}

		// variadic class template function
		template <class... Args>
		void emplace_back(Args&&... args) {
			if (cap_rear < 0) throw std::out_of_range("cap_rear<0 in push_back");
			if (cap_rear == 0) {
				T* old_head = head;
				T* old_front = front;
				T* temp_head = (T*) ::operator new(2 * len_Vector*sizeof(T));
				T* temp_front = temp_head + cap_front;


				head = temp_head;
				front = temp_front;
				new (front + len_elem) T{ args... };

				for (int64_t i = 0; i < len_elem; i++) {
					new (temp_front + i) T{ std::move(old_front[i]) }; //std::move, xvalue
				}

				cap_rear = len_Vector;
				len_Vector *= 2;

				for (int64_t i = 0; i < len_elem; i++) {
					old_front[i].~T();
				}
				::operator delete(old_head);
				old_head = old_front = nullptr;

				len_elem++;
				cap_rear--;

				++modify_version;
				++realloc_reassign_version;
			}
			else {
				new (front + len_elem) T{ args... };
				len_elem++;
				cap_rear--;

				++modify_version;
			}
		}

		// member template ctor's member template function
		template <typename Iter>
		void vector_iter(Iter b, Iter e, std::random_access_iterator_tag x) {
			//std::cout << "random access tag" << std::endl;
			cap_front = cap_rear = 0;
			len_Vector = len_elem = e - b;

			head = (T*) ::operator new(len_Vector*sizeof(T));
			front = head;
			for (int64_t i = 0; b != e; ++b) {
				new (front + i) T{ *b };
				++i;
			}
		}

		template <typename Iter>
		void vector_iter(Iter b, Iter e, std::input_iterator_tag x) {
			//std::cout << "input tag" << std::endl;
			cap_front = 0;
			cap_rear = size_init;
			len_Vector = size_init;
			len_elem = 0;

			head = (T*) ::operator new(size_init*sizeof(T));
			front = head;
			for (; b != e; ++b) {
				push_back(*b);
			}
		}

		// member template ctor
		template <typename Iter>
		vector(Iter b, Iter e) {
			typename std::iterator_traits<Iter>::iterator_category x{};

			modify_version = realloc_reassign_version = 0;

			vector_iter(b, e, x);
		}

		//initializer_list constructor
		vector(std::initializer_list<T> lst) {
			//std::cout << "list init..." << std::endl;
			cap_front = cap_rear = 0;
			len_Vector = len_elem = lst.size();

			modify_version = realloc_reassign_version = 0;

			head = (T*) ::operator new(len_Vector*sizeof(T));
			front = head;
			int64_t i = 0;
			for (T elem : lst) {
				new (front + i) T{ elem };
				++i;
			}
		}

		//declare iterator, defined later
		class iterator;

		//const_iterator defined here
		class const_iterator {
			friend iterator;
		private:
			T* head; // iter.head = vec->head
			int64_t position; // 0-indexed
			const vector<T>* vec;
			int64_t vec_modify_version, vec_realloc_reassign_version;

		public:
			using difference_type = int64_t;
			using value_type = T;
			using iterator_category = std::random_access_iterator_tag;
			using pointer = T*;
			using reference = T&;

			const_iterator(void) {
				head = nullptr;
				position = 0;
				vec = nullptr;
				vec_modify_version = vec_realloc_reassign_version = 0;
			}

			// const vector<T>* v or vector<T>& v
			const_iterator(const vector<T>& v, int64_t pos) {
				head = v.head;
				position = pos;
				vec = &v;
				vec_modify_version = v.modify_version;
				vec_realloc_reassign_version = v.realloc_reassign_version;

				// need or not?
				//validate();
			}

			const_iterator(const const_iterator& iter) {
				iter.validate();
				this->head = iter.head;
				this->position = iter.position;
				this->vec = iter.vec;
				this->vec_modify_version = iter.vec_modify_version;
				this->vec_realloc_reassign_version = iter.vec_realloc_reassign_version;
			}

			const_iterator(const iterator& iter) {
				iter.validate();
				this->head = iter.head;
				this->position = iter.position;
				this->vec = iter.vec;
				this->vec_modify_version = iter.vec_modify_version;
				this->vec_realloc_reassign_version = iter.vec_realloc_reassign_version;
			}

			const_iterator& operator=(const const_iterator& iter) {
				iter.validate();
				this->head = iter.head;
				this->position = iter.position;
				this->vec = iter.vec;
				this->vec_modify_version = iter.vec_modify_version;
				this->vec_realloc_reassign_version = iter.vec_realloc_reassign_version;
				return *this;
			}

			const_iterator& operator=(const iterator& iter) {
				iter.validate();
				this->head = iter.head;
				this->position = iter.position;
				this->vec = iter.vec;
				this->vec_modify_version = iter.vec_modify_version;
				this->vec_realloc_reassign_version = iter.vec_realloc_reassign_version;
				return *this;
			}

			bool operator==(const const_iterator& iter) const {
				validate();
				iter.validate();
				if (this->vec != iter.vec) throw std::runtime_error("Comparing two different iters.");
				return this->position == iter.position;
			}

			bool operator==(const iterator& iter) const {
				validate();
				iter.validate();
				if (this->vec != iter.vec) throw std::runtime_error("Comparing two different iters.");
				return this->position == iter.position;
			}

			bool operator<(const const_iterator& iter) const {
				validate();
				iter.validate();
				if (this->vec != iter.vec) throw std::runtime_error("Comparing two different iters.");
				return this->position < iter.position;
			}

			bool operator<(const iterator& iter) const {
				validate();
				iter.validate();
				if (this->vec != iter.vec) throw std::runtime_error("Comparing two different iters.");
				return this->position < iter.position;
			}

			const_iterator operator+(int64_t k) const {
				validate();
				const_iterator temp(*this);
				temp.position += k;
				//temp.validate();
				return temp;
			}

			const_iterator operator-(int64_t k) const {
				validate();
				const_iterator temp(*this);
				temp.position -= k;
				//temp.validate();
				return temp;
			}

			ptrdiff_t operator-(const const_iterator& iter) const {
				if (this->vec != iter.vec) throw std::runtime_error("Two different iters.");
				validate();
				iter.validate();
				return this->position - iter.position;
			}

			ptrdiff_t operator-(const iterator& iter) const {
				if (this->vec != iter.vec) throw std::runtime_error("Two different iters.");
				validate();
				iter.validate();
				return this->position - iter.position;
			}

			const_iterator& operator++(void) { //prefix	
				validate();
				++position;
				//validate();
				return *this;
			}

			const_iterator operator++(int) { // postfix
				validate();
				const_iterator temp(*this);
				++(*this);
				// validate();
				return temp;
			}

			const_iterator& operator--(void) { //prefix
				validate();
				--position;
				//validate();
				return *this;
			}

			const_iterator operator--(int) { //postfix
				validate();
				const_iterator temp(*this);
				--(*this);
				// validate();
				return temp;
			}

			const_iterator& operator+=(int64_t k) {
				validate();
				position += k;
				//validate();
				return *this;
			}

			const_iterator& operator-=(int64_t k) {
				validate();
				position -= k;
				//validate();
				return *this;
			}

			const T& operator*(void) const {
				validate(true);
				return *(vec->front + position);
			}

			const T* operator->(void) const {
				validate(true);
				return vec->front + position;
			}

			const T& operator[](int64_t k) const {
				//I will allow iter[-k] to refer to previous elem
				((*this) + k).validate(true);
				return *(vec->front + position + k);
			}

			friend void swap(const_iterator& it1, const_iterator& it2) {
				const_iterator temp(it1);
				it1 = it2;
				it2 = temp;
			}

			// for k+iter
			friend const_iterator operator+(int64_t k, const_iterator iter) {
				iter.validate();
				const_iterator temp(iter);
				temp.position += k;
				//temp.validate();
				return temp;
			}

			bool outBounds(bool isderef) const {
				//check low bound
				if (position < 0) return true;

				//check high bound
				if (isderef) { if (position >(vec->len_elem - 1)) return true; }
				else { if (position > vec->len_elem) return true; }

				//check if isempty, when isempty, always out of bounds
				if (vec->len_elem == 0) return true;

				return false;
			}

			void validate(bool isderef = false) const {

				// vec has been destructed, or iter has not been binding to one
				if (vec == nullptr) {
					throw std::runtime_error("Iterator's binding to no vector, or vector has been destructed.");
					return;
				}

				// no modification has been made
				// though very unlikely, check if head has been changed while version are the same
				if (vec_modify_version == vec->modify_version && head == vec->head) {

					// dereference a out of range iter
					if (outBounds(isderef) && isderef) {
						throw std::out_of_range("Iterator is dereferencing a out-of-range vector position.");
						return;
					}

					return;
				}

				// invalidated by any modification

				// out-of-bounds
				if (outBounds(isderef)) {
					throw invalid_iterator{ invalid_iterator::SEVERE };
					return;
				}

				// in-bounds, but push-reallocated or re-assigned
				if (vec_realloc_reassign_version != vec->realloc_reassign_version) {
					throw invalid_iterator{ invalid_iterator::MODERATE };
					return;
				}

				// others
				throw invalid_iterator{ invalid_iterator::MILD };
				return;
			}

		};

		//iterator defined here
		class iterator {
			friend const_iterator;
		private:
			T* head; // iter.head = vec->head
			int64_t position; // 0-indexed
			vector<T>* vec;
			int64_t vec_modify_version, vec_realloc_reassign_version;

		public:
			using difference_type = int64_t;
			using value_type = T;
			using iterator_category = std::random_access_iterator_tag;
			using pointer = T*;
			using reference = T&;

			iterator(void) {
				head = nullptr;
				position = 0;
				vec = nullptr;
				vec_modify_version = vec_realloc_reassign_version = 0;
			}

			// vector<T>* v?  vector<T>&
			iterator(vector<T>& v, int64_t pos) {
				head = v.head;
				position = pos;
				vec = &v;
				vec_modify_version = v.modify_version;
				vec_realloc_reassign_version = v.realloc_reassign_version;

				// need or not?
				//validate();
			}

			iterator(const iterator& iter) {
				iter.validate();
				this->head = iter.head;
				this->position = iter.position;
				this->vec = iter.vec;
				this->vec_modify_version = iter.vec_modify_version;
				this->vec_realloc_reassign_version = iter.vec_realloc_reassign_version;
			}

			iterator& operator=(const iterator& iter) {
				iter.validate();
				this->head = iter.head;
				this->position = iter.position;
				this->vec = iter.vec;
				this->vec_modify_version = iter.vec_modify_version;
				this->vec_realloc_reassign_version = iter.vec_realloc_reassign_version;
				return *this;
			}

			bool operator==(const iterator& iter) const {
				validate();
				iter.validate();
				if (this->vec != iter.vec) throw std::runtime_error("Comparing two different iters.");
				return this->position == iter.position;
			}

			bool operator==(const const_iterator& iter) const {
				validate();
				iter.validate();
				if (this->vec != iter.vec) throw std::runtime_error("Comparing two different iters.");
				return this->position == iter.position;
			}

			bool operator<(const iterator& iter) const {
				validate();
				iter.validate();
				if (this->vec != iter.vec) throw std::runtime_error("Comparing two different iters.");
				return this->position < iter.position;
			}

			bool operator<(const const_iterator& iter) const {
				validate();
				iter.validate();
				if (this->vec != iter.vec) throw std::runtime_error("Comparing two different iters.");
				return this->position < iter.position;
			}

			iterator operator+(int64_t k) const {
				validate();
				iterator temp(*this);
				temp.position += k;
				//temp.validate();
				return temp;
			}

			iterator operator-(int64_t k) const {
				validate();
				iterator temp(*this);
				temp.position -= k;
				//temp.validate();
				return temp;
			}

			ptrdiff_t operator-(const iterator& iter) const {
				if (this->vec != iter.vec) throw std::runtime_error("Two different iters.");
				validate();
				iter.validate();
				return this->position - iter.position;
			}

			ptrdiff_t operator-(const const_iterator& iter) const {
				if (this->vec != iter.vec) throw std::runtime_error("Two different iters.");
				validate();
				iter.validate();
				return this->position - iter.position;
			}

			iterator& operator++(void) { //prefix	
				validate();
				++position;
				//validate();
				return *this;
			}

			iterator operator++(int) { // postfix
				validate();
				iterator temp(*this);
				++(*this);
				// validate();
				return temp;
			}

			iterator& operator--(void) { //prefix
				validate();
				--position;
				//validate();
				return *this;
			}

			iterator operator--(int) { //postfix
				validate();
				iterator temp(*this);
				--(*this);
				// validate();
				return temp;
			}

			iterator& operator+=(int64_t k) {
				validate();
				position += k;
				//validate();
				return *this;
			}

			iterator& operator-=(int64_t k) {
				validate();
				position -= k;
				//validate();
				return *this;
			}

			T& operator*(void) const {
				validate(true);
				return *(vec->front + position);
			}

			T* operator->(void) const {
				validate(true);
				return vec->front + position;
			}

			T& operator[](int64_t k) const {
				//I will allow user to do iter[-k] to refer to earlier elem
				((*this) + k).validate(true);
				return *(vec->front + position + k);
			}

			// for k+iter
			friend iterator operator+(int64_t k, iterator iter) {
				iter.validate();
				iterator temp(iter);
				temp.position += k;
				//temp.validate();
				return temp;
			}

			friend void swap(iterator& it1, iterator& it2) {
				iterator temp(it1);
				it1 = it2;
				it2 = temp;
			}

			bool outBounds(bool isderef) const {
				//check low bound
				if (position < 0) return true;

				//check high bound
				if (isderef) { if (position >(vec->len_elem - 1)) return true; }
				else { if (position > vec->len_elem) return true; }

				//check if isempty, when isempty, always out of bounds
				if (vec->len_elem == 0) return true;
				return false;
			}

			void validate(bool isderef = false) const {

				// vec has been destructed, or iter has not been binding to one
				if (vec == nullptr) {
					throw std::runtime_error("Iterator's binding to no vector, or vector has been destructed.");
					return;
				}

				// no modification has been made
				// though very unlikely, check if head has been changed while version are the same
				if (vec_modify_version == vec->modify_version && head == vec->head) {

					// dereference a out of range iter
					if (outBounds(isderef) && isderef) {
						throw std::out_of_range("Iterator is dereferencing a out-of-range vector position.");
						return;
					}

					return;
				}

				// invalidated by any modification

				// out-of-bounds
				if (outBounds(isderef)) {
					throw invalid_iterator{ invalid_iterator::SEVERE };
					return;
				}

				// in-bounds, but push-reallocated or re-assigned
				if (vec_realloc_reassign_version != vec->realloc_reassign_version) {
					throw invalid_iterator{ invalid_iterator::MODERATE };
					return;
				}

				// others
				throw invalid_iterator{ invalid_iterator::MILD };
				return;
			}

		};


		// const reference to vector must return const_iterator
		const_iterator begin(void) const {
			return const_iterator(*this, 0);
		}

		const_iterator end(void) const {
			return const_iterator(*this, len_elem);
		}

		iterator begin(void) {
			return iterator(*this, 0);
		}

		iterator end(void) {
			return iterator(*this, len_elem);
		}

	};

} //namespace zrdw

#endif
