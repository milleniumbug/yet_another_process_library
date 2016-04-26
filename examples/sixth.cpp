#include <iostream>
#include <thread>
#include <cassert>
#include <yet_another_process_library/process.hpp>

int main()
{
	using namespace yet_another_process_library;
	process p("/nonexisting_executable", make_native_args({}));
	assert(p.get_exit_status() == boost::none);
	std::this_thread::sleep_for(std::chrono::milliseconds(15));
	std::cout << p.get_exit_status().value() << "\n";
}