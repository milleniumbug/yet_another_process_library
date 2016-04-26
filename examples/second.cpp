#include <iostream>
#include <thread>
#include <cassert>
#include <yet_another_process_library/process.hpp>

int main()
{
	namespace yapl = yet_another_process_library;
	yapl::process p(
		"sleep",
		yapl::make_ascii_args({ "1" }),
		nullptr,
		nullptr,
		yapl::process::stdin_closed | yapl::process::search_path_env);
	std::this_thread::sleep_for(std::chrono::milliseconds(15));
	assert(p.get_exit_status() == boost::none);
	std::this_thread::sleep_for(std::chrono::seconds(2));
	assert(p.get_exit_status() == 0);
}