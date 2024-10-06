int _test_codec(int, int, int, int);

int test_codec(int, int, int, int);
int test_stream();
int test_solver();
int test_seed(int, int, int, int);
int run_demo();

int main()
{
	int result = 0;
	//result += _test_codec(100, 10000, 40, 0);
	//result += test_codec(100, 10000, 40, 0);
	result += test_seed(10, 20, 50, 1);
	
	//result += test_solver();
	//result += run_demo();
	return 0;
}
