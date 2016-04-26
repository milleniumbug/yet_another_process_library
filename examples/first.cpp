#include <iostream>
#include <yet_another_process_library/process.hpp>

int main()
{
	using namespace yet_another_process_library;
	process p("which", make_ascii_args({ "gdb" }), [](boost::string_ref s)
	{
		std::cout << s << "\n";
	}, nullptr, process::stdin_closed | process::search_path_env);
	p.wait();
}