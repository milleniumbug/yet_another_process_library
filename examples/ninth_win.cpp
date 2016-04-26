#ifdef _WIN32

#include <iostream>
#include <thread>
#include <cassert>
#include <sstream>
#include <yet_another_process_library/process.hpp>
#include <windows.h>

int main()
{
	namespace yapl = yet_another_process_library;
	
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
	
	std::stringstream ss;
	auto path = boost::filesystem::path(w).parent_path()/L"third";
	yapl::process p(path, yapl::make_native_args({}), [&](boost::string_ref s)
	{
		ss << s;
		ss.flush();
	}, yapl::stderr::closed, yapl::stdin_closed);
	boost::string_ref data = "hello world!\n";
	p.wait();
	assert(p.get_exit_status() == 0);
	assert(ss.str() == data);
}

#else
int main()
{
	
}
#endif