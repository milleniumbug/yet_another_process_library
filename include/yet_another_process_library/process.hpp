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
	struct waiting_on_sleeping_process_error : public std::runtime_error
	{
		waiting_on_sleeping_process_error(const char* what) : std::runtime_error(what) {}
		waiting_on_sleeping_process_error(const std::string& what) : std::runtime_error(what) {}
	};
	
	struct pipe_closed : public std::runtime_error
	{
		pipe_closed(const char* what) : std::runtime_error(what) {}
		pipe_closed(const std::string& what) : std::runtime_error(what) {}
	};
	
	class process
	{
	private:
		struct impl;
		std::unique_ptr<impl> i;
	public:
		typedef unsigned long long flags;
		static const flags suspended = (1ULL << 0);
		static const flags stdin_closed = (1ULL << 1);
		static const flags search_path_env = (1ULL << 2);
		
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
			std::function<void(boost::string_ref)> stdout_handler = nullptr,
			std::function<void(boost::string_ref)> stderr_handler = nullptr,
			flags fl = static_cast<flags>(0));
		
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
		
		void close_stdin();
		bool write(boost::string_ref input);
		void suspend();
		void resume();
		bool is_finished();
		boost::optional<int> get_exit_status();
		void kill(kill_brutality br);
		void kill();
		void wait();
		native_handle_type native_handle();
		
		~process();
		process(process&&);
		process& operator=(process&&);
		process(const process&) = delete;
		process& operator=(const process&) = delete;
	};
}

#endif