#ifndef PROCESS_HPP_E9B66D13DBDB48DAB5081F499D4BFA39
#define PROCESS_HPP_E9B66D13DBDB48DAB5081F499D4BFA39

#include <functional>
#include <boost/utility/string_ref.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <iterator>
#include <vector>
#include <string>
#include <type_traits>
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
	
	struct invalid_encoding : public std::invalid_argument
	{
		invalid_encoding(const char* what) : std::invalid_argument(what) {}
		invalid_encoding(const std::string& what) : std::invalid_argument(what) {}
	};
	
	class native_args
	{
	public:
#if defined(_WIN32)
		typedef wchar_t native_char_type;
#else
		typedef char native_char_type;
#endif
		typedef std::vector<std::basic_string<native_char_type>> underlying_range;
	private:
		struct avoid_copy_constructor_ambiguity {};
		underlying_range args;
		
		template<typename T>
		native_args(T&& a, avoid_copy_constructor_ambiguity) :
			args(std::forward<T>(a))
		{
			
		}
		friend class process;
		friend native_args make_native_args(underlying_range r);
	};
	
	native_args make_native_args(native_args::underlying_range r);
	native_args make_ascii_args(std::vector<std::string> args);
	
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
			native_args arguments,
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