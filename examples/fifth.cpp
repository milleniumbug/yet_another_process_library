#include <iostream>
#include <thread>
#include <cassert>
#include <sstream>
#include <yet_another_process_library/process.hpp>

int main()
{
	namespace yapl = yet_another_process_library;
	std::stringstream ss;
	yapl::process p("bash", yapl::make_native_args({}), [&](boost::string_ref s)
	{
		ss << s;
		ss.flush();
	}, yapl::stderr_closed, yapl::search_path_env);
	p.write("echo Hello world\n");
	p.write("exit\n");
	p.wait();
	assert(p.get_exit_status() == 0);
	assert(ss.str() == "Hello world\n");
}