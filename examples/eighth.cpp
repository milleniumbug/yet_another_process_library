#include <iostream>
#include <cassert>
#include <yet_another_process_library/process.hpp>

int main()
{
	namespace yapl = yet_another_process_library;
	// Making platform specific assumptions about charset of arguments:
	// With `make_ascii_args` you'll get an exception thrown in the face if you pass
	// a non-ASCII character instead of locale-specific Heisenbugs
	// (Hello, "local" codepage and xxxA WinAPI functions)
	try
	{
		yapl::process p("which", yapl::make_ascii_args({ "długa nazwa programu" }), [](boost::string_ref s)
		{
			std::cout << s << "\n";
		}, yapl::stderr_closed, yapl::stdin_closed | yapl::search_path_env);
		p.wait();
		assert(false);
	}
	catch(const yapl::invalid_encoding& ex)
	{
		std::cout << "Localization Heisenbug prevented!\n";
	}
}