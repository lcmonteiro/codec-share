#include <iostream>
#include <fstream>
#include <vector>
int test_codec(int, int, int, int);
int test_codec_(int, int, int, int);
int test_stream();

template <typename T, typename F>
void f(
	const std::vector<T> &v, F comp = [](int) {})
{
	comp();
}

template <class AA, class BB, class CC>
class Test
{
  public:
	template <class A, class B, class C = int>
	Test(A &&a, B &&b, C c = 1)
		: a_{std::forward<A>(a)},
		  b_{std::forward<B>(b)},
		  c_{std::forward<C>(c)} {}

  private:
	AA a_;
	BB b_;
	CC c_;
};
template <class A, class B, class C>
Test(A &&a, B &&b, C &&c)->Test<A, B, C>;
template <class A, class B>
Test(A &&a, B &&b)->Test<A, B, int>;

using True = std::true_type;
const auto TRUE = std::true_type{};
std::true_type a = TRUE;
int main()
{
	test_codec(100, 10000, 40, 0);
	test_codec_(100, 10000, 40, 0);
	//test_codec(10000, 100);
	//test_codec(10000, 1000);
}
