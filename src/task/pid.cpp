#include "taskmaster.hpp"


std::string Taskmaster::signalName(int sig)
{
	switch (sig)
	{
		case SIGTERM: return "SIGTERM";
		case SIGKILL: return "SIGKILL";
		case SIGUSR1: return "SIGUSR1";
		case SIGUSR2: return "SIGUSR2";
		default: return "UNKNOWN_SIGNAL";
	}
}


void Taskmaster::stopProcess(Program &prog, ProcessInfo &proc)
{
	if (proc.pid <= 0)
		return;

	log("Stopping PID " + std::to_string(proc.pid) +
	" using " + signalName(prog.config.stopsignal));
	kill(proc.pid, prog.config.stopsignal);
	time_t start = time(nullptr);
	bool exited = false;
	while (true)
	{
		int status = 0;
		pid_t result = waitpid(proc.pid, &status, WNOHANG);
		if (result == proc.pid)
		{
			exited = true;
			break;
		}
		if (time(nullptr) - start >= prog.config.stoptime)
		{
			kill(proc.pid, SIGKILL);
			waitpid(proc.pid, NULL, 0);
			break;
		}
		usleep(100000);
	}
	proc.pid = -1;
	if (exited)
	{
		proc.state = ProcessState::EXITED;
		proc.exitcode = 0;
	}
	else
	{
		proc.state = ProcessState::STOPPED;
		proc.exitcode = 0;
	}
}



void Taskmaster::spawnProcess(Program &prog, ProcessInfo &proc)
{
	log("Spawning process for: " + prog.config.name);

	pid_t pid = fork();
	if (pid < 0)
	{
		log("ERROR: fork() failed");
		return;
	}

	// -------------------------
	// CHILD PROCESS
	// -------------------------
	if (pid == 0)
	{
		// 1. Working directory
		if (!prog.config.workingdir.empty())
		{
			if (chdir(prog.config.workingdir.c_str()) != 0)
				_exit(1);
		}

		// 2. Apply umask
		umask(prog.config.umask_value);

		// 3. Resolve stdout/stderr paths
		std::string stdout_path = resolvePath(prog.config.stdout_file, prog.config.workingdir);
		std::string stderr_path = resolvePath(prog.config.stderr_file, prog.config.workingdir);

		// Create logs directory if needed
		mkdir("logs", 0755);
		mkdir("logs/archive", 0755);

		// 4. Redirect STDOUT
		if (!prog.config.stdout_file.empty())
		{
			int fd_out = open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd_out < 0)
				_exit(1);
			dup2(fd_out, STDOUT_FILENO);
			close(fd_out);
		}

		// 5. Redirect STDERR
		if (!prog.config.stderr_file.empty())
		{
			int fd_err = open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd_err < 0)
				_exit(1);
			dup2(fd_err, STDERR_FILENO);
			close(fd_err);
		}

		// 6. Build argv[]
		std::vector<std::string> parts;
		{
			std::istringstream iss(prog.config.cmd);
			std::string tmp;
			while (iss >> tmp)
				parts.push_back(tmp);
		}

		char **argv = new char *[parts.size() + 1];
		for (size_t i = 0; i < parts.size(); ++i)
			argv[i] = strdup(parts[i].c_str());
		argv[parts.size()] = nullptr;

		// 7. Build envp[]
		std::vector<std::string> env_strings;
		std::vector<char*> envp;

		// Add custom env from JSON
		for (auto &kv : prog.config.env)
			env_strings.push_back(kv.first + "=" + kv.second);

		// Add inherited environment (PATH, HOME, USER, etc.)
		for (char **sys = environ; *sys != nullptr; sys++)
			envp.push_back(*sys);

		// Add our custom env
		for (auto &s : env_strings)
			envp.push_back(const_cast<char*>(s.c_str()));

		envp.push_back(nullptr);

		// 8. execve() with environment
		execve(argv[0], argv, envp.data());

		// If execve fails
		for (size_t i = 0; i < parts.size(); ++i)
			free(argv[i]);
		delete [] argv;

		_exit(1);
	}

	// -------------------------
	// PARENT PROCESS
	// -------------------------
	proc.pid = pid;
	proc.state = ProcessState::STARTING;
	proc.start_timestamp = time(nullptr);
	log("Spawned PID " + std::to_string(pid));
}
