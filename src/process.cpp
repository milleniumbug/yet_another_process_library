#include <type_traits>
#include <thread>
#include <mutex>
#include <exception>
#include <iterator>
#include <numeric>
#include <boost/algorithm/string/replace.hpp>
#include <yet_another_process_library/process.hpp>

#define STRINGIFY1(x) #x
#define STRINGIFY2(x) STRINGIFY1(x)
#define LINESTR STRINGIFY2(__LINE__)

namespace yet_another_process_library
{
	template<typename T>
	bool is_set(T in, T flag)
	{
		return (in & flag) != 0;
	}
	
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
			if(!Policy::is_null(old_handle))
			{
				Policy::close(old_handle);
			}
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
	
	struct contains_a_non_ascii_letter
	{
		template<typename ForwardRange>
		bool operator()(ForwardRange x) const
		{
			using std::begin;
			using std::end;
			return std::any_of(begin(x), end(x), [](decltype(*begin(x)) c)
			{
				return c < 0 || c > 127;
			});
		}
	};
	
	native_args make_native_args(native_args::underlying_range r)
	{
		return native_args(std::move(r), native_args::avoid_copy_constructor_ambiguity());
	}
	
	native_args make_ascii_args(std::vector<std::string> args)
	{
		if(std::any_of(args.begin(), args.end(), contains_a_non_ascii_letter()))
			throw invalid_encoding("non-ASCII character passed");
		native_args::underlying_range underlying;
		underlying.reserve(args.size());
		std::transform(begin(args), end(args), std::back_inserter(underlying), [](const std::string& a)
		{
			return std::basic_string<native_args::native_char_type>(a.begin(), a.end());
		});
		return make_native_args(underlying);
	}
}

#if defined(_WIN32)
#include <windows.h>
#include <TlHelp32.h>
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
			return nullptr;
		}
		
		static bool is_null(handle_type handle)
		{
			return handle || handle == INVALID_HANDLE_VALUE;
		}
	};
	
	struct pipe
	{
		unique_handle<windows_process_handle_policy> producer;
		unique_handle<windows_process_handle_policy> consumer;
		
		void reset()
		{
			producer.reset();
			consumer.reset();
		}
	};
	
	pipe create_pipe()
	{
		SECURITY_ATTRIBUTES security_attributes;
		security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
		security_attributes.bInheritHandle = TRUE;
		security_attributes.lpSecurityDescriptor = NULL;
		
		HANDLE rawp[2];
		if(!CreatePipe(&rawp[0], &rawp[1], &security_attributes, 0))
			throw "FUCK";
		
		pipe p;
		p.producer = unique_handle<windows_process_handle_policy>(rawp[0]);
		p.consumer = unique_handle<windows_process_handle_policy>(rawp[1]);
		return p;
	}
	
	void mark_inherited(unique_handle<windows_process_handle_policy>& handle)
	{
		if(!SetHandleInformation(handle.get(), HANDLE_FLAG_INHERIT, 0))
			throw "FUCK";
	}
	
	struct process::impl
	{
		std::mutex close_mutex;
		std::mutex stdin_mutex;
		std::thread stdout_reader;
		std::thread stderr_reader;
		DWORD process_id;
		unique_handle<windows_process_handle_policy> process_handle;
		unique_handle<windows_process_handle_policy> stdin_fd;
		unique_handle<windows_process_handle_policy> stdout_fd;
		unique_handle<windows_process_handle_policy> stderr_fd;
		int exit_status = -1;
	};
	
	// adapted from https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
	std::wstring arg_quote(const std::wstring& Argument, bool Force)
	{
		std::wstring CommandLine;
		// Unless we're told otherwise, don't quote unless we actually
		// need to do so --- hopefully avoid problems if programs won't
		// parse quotes properly
		if(
			!Force &&
			!Argument.empty() &&
			Argument.find_first_of(L" \t\n\v\"") == Argument.npos)
		{
			CommandLine.append(Argument);
		}
		else
		{
			CommandLine.push_back(L'"');
			for(auto It = Argument.begin(); true; ++It) {
				unsigned NumberBackslashes = 0;
				while(It != Argument.end() && *It == L'\\')
				{
					++It;
					++NumberBackslashes;
				}
				if(It == Argument.end())
				{
					// Escape all backslashes, but let the terminating
					// double quotation mark we add below be interpreted
					// as a metacharacter.
					CommandLine.append(NumberBackslashes * 2, L'\\');
					break;
				}
				else if (*It == L'"')
				{
					// Escape all backslashes and the following
					// double quotation mark.
					CommandLine.append(NumberBackslashes * 2 + 1, L'\\');
					CommandLine.push_back(*It);
				}
				else
				{
					// Backslashes aren't special here.
					CommandLine.append(NumberBackslashes, L'\\');
					CommandLine.push_back(*It);
				}
			}
			CommandLine.push_back(L'"');
		}
		return CommandLine;
	}
	
	std::wstring arg_quote(const std::wstring& Argument)
	{
		return arg_quote(Argument, false);
	}
	
	process::process(
		boost::filesystem::path executable_file,
		native_args wrapped_arguments,
		boost::variant<stdout, stream_consumer> stdout_handler,
		boost::variant<stderr, stream_consumer> stderr_handler,
		const flags fl)
	{
		pipe stdin_pipe;
		pipe stdout_pipe;
		pipe stderr_pipe;
		
		const bool open_stdin = !is_set(fl, stdin_closed);
		const bool open_stdout = stdout_handler.which() != 0;
		const bool open_stderr = stderr_handler.which() != 0;
		
		if(open_stdin)
		{
			stdin_pipe = create_pipe();
			mark_inherited(stdin_pipe.consumer);
		}
		if(open_stdout)
		{
			stdout_pipe = create_pipe();
			mark_inherited(stdout_pipe.producer);
		}
		if(open_stderr)
		{
			stderr_pipe = create_pipe();
			mark_inherited(stderr_pipe.producer);
		}
		
		PROCESS_INFORMATION process_info;
		STARTUPINFOW startup_info;
		memset(&process_info, 0, sizeof process_info);
		memset(&startup_info, 0, sizeof startup_info);
		startup_info.cb = sizeof startup_info;
		startup_info.hStdInput = stdin_pipe.producer.get();
		startup_info.hStdOutput = stdout_pipe.consumer.get();
		startup_info.hStdError = stderr_pipe.consumer.get();
		if(open_stdin || open_stdout || open_stderr)
			startup_info.dwFlags |= STARTF_USESTDHANDLES;
		
		std::wstring application_name = executable_file.native();
		auto&& arguments = wrapped_arguments.args;
		// TODO: a proper joining function
		std::wstring first_arg = arg_quote(executable_file.filename().native());
		std::transform(arguments.begin(), arguments.end(), arguments.begin(), [](const std::wstring& w){ return arg_quote(w); });
		std::wstring command_line = std::accumulate(arguments.begin(), arguments.end(), first_arg, [](std::wstring lhs, std::wstring rhs)
		{
			return lhs + L" " + rhs;
		});
		DWORD creation_flags = 0;
		creation_flags |= is_set(fl, suspended) ? DEBUG_PROCESS : 0;
		//CreateProcess(NULL, process_command.empty()?NULL:&process_command[0], NULL, NULL, TRUE, 0,
                                //NULL, path.empty()?NULL:path.c_str(), &startup_info, &process_info);
		BOOL success = CreateProcessW(
			&application_name[0],
			&command_line[0],
			nullptr,
			nullptr,
			TRUE,
			creation_flags,
			nullptr,
			nullptr,
			&startup_info,
			&process_info);
		
		i.reset(new impl());
		i->stdin_fd = std::move(stdin_pipe.consumer);
		i->stdout_fd = std::move(stdout_pipe.producer);
		i->stderr_fd = std::move(stderr_pipe.producer);
		i->process_handle.reset(process_info.hProcess);
		i->process_id = process_info.dwProcessId;
		unique_handle<windows_process_handle_policy> thread_handle(process_info.hThread);
		
		auto reader = [](std::function<void(boost::string_ref)> reader, HANDLE fd)
		{
			std::vector<char> buffer(4096);
			DWORD n;
			while(true)
			{
				BOOL success = ReadFile(fd, buffer.data(), buffer.size(), &n, nullptr);
				if(!success || n == 0)
					break;
				reader(boost::string_ref(buffer.data(), static_cast<size_t>(n)));
			}
		};
		if(stdout_handler.which() == 1)
			i->stdout_reader = std::thread(reader, boost::strict_get<stream_consumer>(stdout_handler), i->stdout_fd.get());
		if(stderr_handler.which() == 1)
			i->stderr_reader = std::thread(reader, boost::strict_get<stream_consumer>(stderr_handler), i->stderr_fd.get());
	}
	
	void process::close_stdin()
	{
		std::lock_guard<std::mutex> lock(i->stdin_mutex);
		i->stdin_fd.reset();
	}
	
	bool process::write(boost::string_ref input)
	{
		std::lock_guard<std::mutex> lock(i->stdin_mutex);
		DWORD written;
		BOOL success = WriteFile(i->stdin_fd.get(), input.data(), input.size(), &written, nullptr);
		// TODO: check correctness
		if(!success || written == 0)
		{
			const DWORD err = GetLastError();
			if(err == EBADF)
				throw pipe_closed("stdin is closed");
			return false;
		}
		return true;
	}
	
	bool process::is_finished()
	{
		DWORD exit_status;
		WaitForSingleObject(i->process_handle.get(), 0);
		if(!GetExitCodeProcess(i->process_handle.get(), &exit_status))
			return false;
		i->exit_status = static_cast<int>(exit_status);
		return true;
	}
	
	void process::suspend()
	{
		DebugActiveProcess(i->process_id);
	}
	
	void process::resume()
	{
		DebugActiveProcessStop(i->process_id);
	}
	
	boost::optional<int> process::get_exit_status()
	{
		if(is_finished())
			return i->exit_status;
		return boost::none;
	}
	
	void process::kill(kill_brutality br)
	{
		// Based on http://stackoverflow.com/a/1173396
		std::lock_guard<std::mutex> lock(i->close_mutex);
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if(snapshot)
		{
			PROCESSENTRY32 process;
			memset(&process, 0, sizeof process);
			process.dwSize = sizeof process;
			if(Process32First(snapshot, &process))
			{
				do
				{
					if(process.th32ParentProcessID == i->process_id)
					{
						HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process.th32ProcessID);
						if(process_handle)
							TerminateProcess(process_handle, 2);
					}
				} while(Process32Next(snapshot, &process));
			}
		}
		TerminateProcess(i->process_handle.get(), 2);
	}
	
	void process::wait()
	{
		WaitForSingleObject(i->process_handle.get(), INFINITE);
	}
	
	process::native_handle_type process::native_handle()
	{
		return i->process_handle.get();
	}
	
	process::~process()
	{
		if(!is_finished())
			std::terminate();
		if(i->stdout_reader.joinable())
			i->stdout_reader.join();
		if(i->stderr_reader.joinable())
			i->stderr_reader.join();
	}
	
	process::process(process&&) = default;
	process& process::operator=(process&&) = default;
	
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
		unique_handle<unix_fd_handle_policy> producer;
		unique_handle<unix_fd_handle_policy> consumer;
		
		void reset()
		{
			producer.reset();
			consumer.reset();
		}
	};
	
	pipe create_pipe()
	{
		int rawp[2];
		if(::pipe(rawp) == -1)
			throw "FUCK";
		
		pipe p;
		p.producer = unique_handle<unix_fd_handle_policy>(rawp[0]);
		p.consumer = unique_handle<unix_fd_handle_policy>(rawp[1]);
		return p;
	}
	
	struct process::impl
	{
		std::mutex stdin_mutex;
		std::thread stdout_reader;
		std::thread stderr_reader;
		unique_handle<unix_fd_handle_policy> process_handle;
		unique_handle<unix_fd_handle_policy> stdin_fd;
		unique_handle<unix_fd_handle_policy> stdout_fd;
		unique_handle<unix_fd_handle_policy> stderr_fd;
		int exit_status = -1;
	};
	
	process::process(
		boost::filesystem::path executable_file,
		native_args wrapped_arguments,
		boost::variant<stdout, stream_consumer> stdout_handler,
		boost::variant<stderr, stream_consumer> stderr_handler,
		const flags fl)
	{
		pipe stdin_pipe;
		pipe stdout_pipe;
		pipe stderr_pipe;
		
		const bool open_stdin = !is_set(fl, stdin_closed);
		const bool open_stdout = stdout_handler.which() != 0;
		const bool open_stderr = stderr_handler.which() != 0;
		
		if(open_stdin)
			stdin_pipe = create_pipe();
		if(open_stdout)
			stdout_pipe = create_pipe();
		if(open_stderr)
			stderr_pipe = create_pipe();
		
		native_handle_type pid = fork();
		if(pid < 0)
			throw "FUCK";
		
		if(pid == 0)
		{
			if(open_stdin)
			{
				dup2(stdin_pipe.producer.get(), 0);
				stdin_pipe.consumer.reset();
			}
			if(open_stdout)
			{
				dup2(stdout_pipe.consumer.get(), 1);
				stdout_pipe.producer.reset();
			}
			if(open_stderr)
			{
				dup2(stderr_pipe.consumer.get(), 2);
				stderr_pipe.producer.reset();
			}
			
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
			
			std::string arg0 = executable_file.stem().native();
			std::vector<char*> raw_args;
			auto&& arguments = wrapped_arguments.args;
			raw_args.reserve(arguments.size()+2);
			raw_args.push_back(&arg0[0]);
			std::transform(arguments.begin(), arguments.end(), std::back_inserter(raw_args), [](std::string& s)
			{
				return &s[0];
			});
			raw_args.push_back(nullptr);
			if(is_set(fl, search_path_env))
			{
				::execvp(executable_file.native().c_str(), raw_args.data());
			}
			else
			{
				::execv(executable_file.native().c_str(), raw_args.data());
			}
			_exit(EXIT_FAILURE);
		}
		
		stdin_pipe.producer.reset();
		stdout_pipe.consumer.reset();
		stderr_pipe.consumer.reset();
		
		i.reset(new impl());
		i->stdin_fd = std::move(stdin_pipe.consumer);
		i->stdout_fd = std::move(stdout_pipe.producer);
		i->stderr_fd = std::move(stderr_pipe.producer);
		i->process_handle.reset(pid);
		
		auto reader = [](std::function<void(boost::string_ref)> reader, int fd)
		{
			std::vector<char> buffer(4096);
			ssize_t n;
			while(true)
			{
				n = read(fd, buffer.data(), buffer.size());
				if(n <= 0)
					break;
				reader(boost::string_ref(buffer.data(), static_cast<size_t>(n)));
			}
		};
		if(stdout_handler.which() == 1)
			i->stdout_reader = std::thread(reader, boost::strict_get<stream_consumer>(stdout_handler), i->stdout_fd.get());
		if(stderr_handler.which() == 1)
			i->stderr_reader = std::thread(reader, boost::strict_get<stream_consumer>(stderr_handler), i->stderr_fd.get());
	}
	
	void process::close_stdin()
	{
		std::lock_guard<std::mutex> lock(i->stdin_mutex);
		i->stdin_fd.reset();
	}
	
	bool process::write(boost::string_ref input)
	{
		std::lock_guard<std::mutex> lock(i->stdin_mutex);
		const int res = ::write(i->stdin_fd.get(), input.data(), input.size());
		if(res < 0)
		{
			perror(LINESTR);
			const int err = errno;
			if(err == EBADF)
				throw pipe_closed("stdin is closed");
			return false;
		}
		else if(res == 0)
			return false;
		else
			return true;
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
		int exitcode = -1;
		const int res = ::waitpid(native_handle(), &exitcode, WNOHANG);
		if(res > 0)
		{
			i->exit_status = exitcode;
			return true;
		}
		else if(res == 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	
	boost::optional<int> process::get_exit_status()
	{
		if(is_finished())
		{
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
		i->stdin_fd.reset();
		i->stdout_fd.reset();
		i->stderr_fd.reset();
	}
	
	process::native_handle_type process::native_handle()
	{
		return i->process_handle.get();
	}
	
	process::~process()
	{
		if(!is_finished())
			std::terminate();
		if(i->stdout_reader.joinable())
			i->stdout_reader.join();
		if(i->stderr_reader.joinable())
			i->stderr_reader.join();
	}
	
	process::process(process&&) = default;
	process& process::operator=(process&&) = default;
}

#endif