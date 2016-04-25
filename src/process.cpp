#include <type_traits>
#include <thread>
#include <mutex>
#include <iterator>
#include <yet_another_process_library/process.hpp>

namespace yet_another_process_library
{
	template<typename T>
	bool is_set(T in, T flag)
	{
		return (in | flag) != 0;
	};
	
	template<typename Policy>
	class unique_handle
	{
		typename Policy::handle_type h;
		
	public:
		unique_handle(const unique_handle&) = delete;
		
		typename Policy::handle_type get() const
		{
			return h;
		}
		
		typename Policy::handle_type release()
		{
			typename Policy::handle_type temp = h;
			h = Policy::get_null();
			return temp;
		}
		
		void reset(typename Policy::handle_type new_handle)
		{
			typename Policy::handle_type old_handle = h;
			h = new_handle;
			if(Policy::is_null(old_handle))
				Policy::close(old_handle);
		}
		
		void swap(unique_handle& other)
		{
			std::swap(this->h, other.h);
		}
		
		void reset()
		{
			reset(Policy::get_null());
		}
		
		~unique_handle()
		{
			reset();
		}
		
		unique_handle& operator=(unique_handle other) noexcept
		{
			this->swap(other);
			return *this;
		}
		
		unique_handle(unique_handle&& other) noexcept
		{
			this->h = other.h;
			other.h = Policy::get_null();
		}
		
		unique_handle()
		{
			h = Policy::get_null();
		}
		
		unique_handle(typename Policy::handle_type handle)
		{
			h = handle;
		}
	};
	
	void process::kill()
	{
		kill(process::ask);
	}
}

#if defined(_WIN32)
#include <windows.h>
static_assert(std::is_same<yet_another_process_library::process::native_handle_type, HANDLE>::value, "invalid assumption about native type");

namespace yet_another_process_library
{
	struct windows_process_handle_policy
	{
		typedef process::native_handle_type handle_type;
		static void close(handle_type handle)
		{
			::CloseHandle(handle);
		}
		
		static handle_type get_null()
		{
			return INVALID_HANDLE_VALUE;
		}
		
		static bool is_null(handle_type handle)
		{
			return handle == INVALID_HANDLE_VALUE;
		}
	};
	
	struct process::impl
	{
		unique_handle<windows_process_handle_policy> process_handle;
	};
	
	process::process(
		boost::filesystem::path executable_file,
		std::vector<std::wstring> arguments,
		std::function<void(boost::string_ref)> stdout_handler,
		std::function<void(boost::string_ref)> stderr_handler,
		const flags fl)
	{
		
	}
	
	process::process(
		boost::filesystem::path executable_file,
		std::vector<std::string> arguments,
		std::function<void(boost::string_ref)> stdout_handler,
		std::function<void(boost::string_ref)> stderr_handler,
		const flags fl)
	{
		
	}
	
	void process::write(boost::string_ref input)
	{
		
	}
	
	bool process::is_finished()
	{
		
	}
	
	boost::optional<int> process::get_exit_status()
	{
		
	}
	
	void process::kill(kill_brutality br)
	{
		
	}
	
	void process::wait()
	{
		
	}
	
	process::native_handle_type process::native_handle()
	{
		
	}
	
}
#else
#include <sys/types.h>
#include <sys/wait.h>

namespace yet_another_process_library
{
	struct unix_fd_handle_policy
	{
		typedef process::native_handle_type handle_type;
		static void close(handle_type handle)
		{
			::close(handle);
		}
		
		static handle_type get_null()
		{
			return -1;
		}
		
		static bool is_null(handle_type handle)
		{
			return handle == -1;
		}
	};
	
	struct pipe
	{
		unique_handle<unix_fd_handle_policy> consumer;
		unique_handle<unix_fd_handle_policy> producer;
		
		void reset()
		{
			consumer.reset();
			producer.reset();
		}
	};
	
	pipe create_pipe()
	{
		int rawp[2];
		if(::pipe(rawp) == -1)
			throw "FUCK";
		
		pipe p;
		p.consumer = unique_handle<unix_fd_handle_policy>(rawp[0]);
		p.producer = unique_handle<unix_fd_handle_policy>(rawp[1]);
		return p;
	}
	
	struct process::impl
	{
		unique_handle<unix_fd_handle_policy> process_handle;
		unique_handle<unix_fd_handle_policy> stdin_fd;
		unique_handle<unix_fd_handle_policy> stdout_fd;
		unique_handle<unix_fd_handle_policy> stderr_fd;
		std::mutex stdin_mutex;
		int exit_status;
	};
	
	process::process(
		boost::filesystem::path executable_file,
		std::vector<std::wstring> arguments,
		std::function<void(boost::string_ref)> stdout_handler,
		std::function<void(boost::string_ref)> stderr_handler,
		const flags fl)
	{
		
	}
	
	process::process(
		boost::filesystem::path executable_file,
		std::vector<std::string> arguments,
		std::function<void(boost::string_ref)> stdout_handler,
		std::function<void(boost::string_ref)> stderr_handler,
		const flags fl)
	{
		pipe stdin_pipe;
		pipe stdout_pipe;
		pipe stderr_pipe;
		
		const bool open_stdin = !is_set(fl, stdin_closed);
		
		if(open_stdin)
			stdin_pipe = create_pipe();
		if(stdout_handler)
			stdout_pipe = create_pipe();
		if(stderr_handler)
			stderr_pipe = create_pipe();
		
		native_handle_type pid = fork();
		if(pid < 0)
			throw "FUCK";
		
		if(pid == 0)
		{
			if(open_stdin) dup2(stdin_pipe.consumer.get(), 0);
			if(stdout_handler) dup2(stdout_pipe.producer.get(), 1);
			if(stderr_handler) dup2(stdout_pipe.producer.get(), 2);
			
			auto close_parent_fds_above = [](int fd)
			{
				const int fd_max=sysconf(_SC_OPEN_MAX);
				for(int i = fd+1; i < fd_max; i++)
					::close(i);
			};
			close_parent_fds_above(2);
			
			::setpgid(0, 0);
			
			auto before_exec = [&]()
			{
				const bool should_suspend = is_set(fl, suspended);
				
				if(should_suspend)
				{
					::kill(0, SIGSTOP);
				}
			};
			before_exec();
			
			std::string arg0 = "";
			std::vector<char*> raw_args;
			raw_args.reserve(arguments.size());
			raw_args.push_back(&arg0[0]);
			std::transform(arguments.begin(), arguments.end(), std::back_inserter(raw_args), [](std::string& s)
			{
				return &s[0];
			});
			raw_args.push_back(nullptr);
			::execv(executable_file.string().c_str(), raw_args.data());
		}
		
		stdin_pipe.consumer.reset();
		stdout_pipe.producer.reset();
		stderr_pipe.producer.reset();
		
		i.reset(new impl());
		i->stdin_fd = std::move(stdin_pipe.producer);
		i->stdout_fd = std::move(stdout_pipe.consumer);
		i->stderr_fd = std::move(stderr_pipe.consumer);
		i->process_handle.reset(pid);
	}
	
	void process::write(boost::string_ref input)
	{
		if(!i)
			return;
		
		std::lock_guard<std::mutex> lock(i->stdin_mutex);
		
	}
	
	void process::suspend()
	{
		::kill(native_handle(), SIGSTOP);
	}
	
	void process::resume()
	{
		::kill(native_handle(), SIGCONT);
	}
	
	bool process::is_finished()
	{
		if(::kill(native_handle(), 0) == -1)
		{
			const int err = errno;
			assert(err == ESRCH);
			return true;
		}
		return false;
	}
	
	boost::optional<int> process::get_exit_status()
	{
		if(is_finished())
		{
			wait();
			return i->exit_status;
		}
		return boost::none;
	}
	
	void process::kill(kill_brutality br)
	{
		if(br == force)
			::kill(native_handle(), SIGTERM);
		if(br == ask)
			::kill(native_handle(), SIGKILL);
	}
	
	void process::wait()
	{
		waitpid(native_handle(), &i->exit_status, 0);
	}
	
	process::native_handle_type process::native_handle()
	{
		return i->process_handle.get();
	}
	
}

#endif