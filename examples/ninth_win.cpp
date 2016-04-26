#ifdef _WIN32

#include <iostream>
#include <thread>
#include <cassert>
#include <sstream>
#include <yet_another_process_library/process.hpp>

boost::filesystem::path get_current_process_path();
boost::filesystem::path program_component();

int main()
{
	namespace yapl = yet_another_process_library;
	std::stringstream ss;
	auto path = get_current_process_path().parent_path()/program_component();
	std::cout << path << "\n";
	yapl::process p(path, yapl::make_ascii_args({ "first\"_arg", "second argument", "some ' special chars $@!~`\n", "last, for good measure", "and a quote \""}), [&](boost::string_ref s)
	{
		ss << s;
		ss.flush();
	}, yapl::stderr::closed, yapl::stdin_closed);
	p.wait();
	std::string expected =
R"*****(_arg_tester%ENDOFARGUMENT%
first"_arg%ENDOFARGUMENT%
second argument%ENDOFARGUMENT%
some ' special chars $@!~`
%ENDOFARGUMENT%
last, for good measure%ENDOFARGUMENT%
and a quote "%ENDOFARGUMENT%
)*****";
	std::string actual = ss.str();
	assert(actual == expected);
	std::cout << actual;
}

#include <windows.h>

boost::filesystem::path get_current_process_path()
{
	std::wstring w;
	w.resize(1024);
	auto size = GetModuleFileNameW(nullptr, &w[0], w.size());
	if(size == 0 || size == w.size())
	{
		// assuming the path is less than 1024 chars
		// and the function doesn't fail
		assert(false);
	}
	w.resize(size);
	return boost::filesystem::path(w);
}


boost::filesystem::path program_component()
{
	return L"_arg_tester.exe";
}

#else
int main()
{
	
}
#endif