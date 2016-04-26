#include <iostream>
#include <thread>
#include <cassert>
#include <sstream>
#include <yet_another_process_library/process.hpp>

int main()
{
	using namespace yet_another_process_library;
	std::stringstream ss;
	process p("cat", make_native_args({}), [&](boost::string_ref s)
	{
		ss << s;
		ss.flush();
	}, nullptr, process::search_path_env | process::suspended);
	boost::string_ref data = "hello world!\n";
	p.write(data);
	p.close_stdin();
	assert(p.get_exit_status() == boost::none);
	std::this_thread::sleep_for(std::chrono::seconds(2));
	p.resume();
	p.wait();
	assert(p.get_exit_status() == 0);
	assert(ss.str() == data);
}