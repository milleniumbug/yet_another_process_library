#include <iostream>
#include <thread>
#include <cassert>
#include <yet_another_process_library/process.hpp>

int main()
{
	namespace yapl = yet_another_process_library;
	yapl::process p("/nonexisting_executable", yapl::make_native_args({}));
	assert(p.get_exit_status() == boost::none);
	std::this_thread::sleep_for(std::chrono::milliseconds(15));
	std::cout << p.get_exit_status().value() << "\n";
}