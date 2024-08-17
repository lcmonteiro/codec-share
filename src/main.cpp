int _test_codec(int, int, int, int);

int test_codec(int, int, int, int);
int test_stream();
int test_solver();

int main()
{
	_test_codec(100, 10000, 40, 0);
	
	test_codec(100, 10000, 40, 0);
	//test_solver();
	return 0;
}
