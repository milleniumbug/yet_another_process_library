#include <iostream>
#include <yet_another_process_library/process.hpp>

int main()
{
	namespace yapl = yet_another_process_library;
	yapl::process p("which", yapl::make_ascii_args({ "gdb" }), [](boost::string_ref s)
	{
		std::cout << s << "\n";
	}, nullptr, yapl::process::stdin_closed | yapl::process::search_path_env);
	p.wait();
}