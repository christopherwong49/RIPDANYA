#include <cstddef>

namespace rip {
	template <typename T> class vector {
	private:
		T arr[256];
		size_t len;

	public:
		vector() : len(0) {}

		void push_back(T x) {
			arr[len++] = x;
		}
		void pop_back() {
			len--;
		}
		void clear() {
			len = 0;
		}

		T &operator[](size_t i) {
			return arr[i];
		}
		const T &operator[](size_t i) const {
			return arr[i];
		}

		T &back() {
			return arr[len - 1];
		}

		size_t size() const {
			return len;
		}

		T *begin() {
			return &arr[0];
		}
		T *end() {
			return &arr[len];
		}

		bool empty() const {
			return len == 0;
		}
	};
};
