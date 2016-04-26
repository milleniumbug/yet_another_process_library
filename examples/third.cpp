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
	}, yapl::stderr::closed, yapl::search_path_env);
	boost::string_ref data = "hello world!\n";
	p.write(data);
	p.close_stdin();
	p.wait();
	assert(p.get_exit_status() == 0);
	assert(ss.str() == data);
}