#ifndef PROCESS_HPP_E9B66D13DBDB48DAB5081F499D4BFA39
#define PROCESS_HPP_E9B66D13DBDB48DAB5081F499D4BFA39

#include <functional>
#include <boost/utility/string_ref.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <vector>
#include <stdexcept>

namespace yet_another_process_library
{
	class process
	{
	private:
		struct impl;
		std::unique_ptr<impl> i;
	public:
		enum flags : unsigned long long
		{
			suspended = (1 << 0),
			stdin_closed = (1 << 1)
		};
		
		enum kill_brutality : unsigned int
		{
			force,
			ask
		};
#if defined(_WIN32)
		typedef void* native_handle_type;
#else
		typedef int native_handle_type;
#endif
		
		process(
			boost::filesystem::path executable_file,
			std::vector<std::string> arguments,
			std::function<void(boost::string_ref)> stdout_handler = nullptr,
			std::function<void(boost::string_ref)> stderr_handler = nullptr,
			flags fl = static_cast<flags>(0));
		
		process(
			boost::filesystem::path executable_file,
			std::vector<std::wstring> arguments,
			std::function<void(boost::string_ref)> stdout_handler = nullptr,
			std::function<void(boost::string_ref)> stderr_handler = nullptr,
			flags fl = static_cast<flags>(0));
		
		void write(boost::string_ref input);
		void suspend();
		void resume();
		bool is_finished();
		boost::optional<int> get_exit_status();
		void kill(kill_brutality br);
		void kill();
		void wait();
		native_handle_type native_handle();
	};
}

#endif