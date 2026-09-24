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



void Taskmaster::handleSignals()
{
	for (auto &pair : programs)
	{
		Program &prog = pair.second;
		for (auto &proc : prog.processes)
			checkProcessStatus(prog, proc);
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
	if (pid == 0)
	{
		if (!prog.config.workingdir.empty())
		{
			if (chdir(prog.config.workingdir.c_str()) != 0)
				_exit(1);
		}
		
		std::string stdout_path = resolvePath(prog.config.stdout_file, prog.config.workingdir);
		std::string stderr_path = resolvePath(prog.config.stderr_file, prog.config.workingdir);
		mkdir("logs", 0755);
		mkdir("logs/archive", 0755);
		if (!prog.config.stdout_file.empty())
		{
			int fd_out = open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd_out < 0)
				_exit(1);
			dup2(fd_out, STDOUT_FILENO);
			close(fd_out);
		}
		if (!prog.config.stderr_file.empty())
		{
			int fd_err = open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd_err < 0)
				_exit(1);
			dup2(fd_err, STDERR_FILENO);
			close(fd_err);
		}
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
		execvp(argv[0], argv);
		for (size_t i = 0; i < parts.size(); ++i)
			free(argv[i]);
		delete [] argv;
		_exit(1);
	}
	proc.pid = pid;
	proc.state = ProcessState::STARTING;
	proc.start_timestamp = time(nullptr);
	proc.retries = 0;
	log("Spawned PID " + std::to_string(pid));
}
