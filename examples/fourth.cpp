#include <iostream>
#include <thread>
#include <cassert>
#include <sstream>
#include <yet_another_process_library/process.hpp>

int main()
{
	namespace yapl = yet_another_process_library;
	std::stringstream ss;
	yapl::process p("cat", yapl::make_native_args({}), [&](boost::string_ref s)
	{
		ss << s;
		ss.flush();
	}, yapl::stderr_closed, yapl::search_path_env | yapl::suspended);
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