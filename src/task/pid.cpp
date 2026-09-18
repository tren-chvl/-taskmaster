#include "taskmaster.hpp"

void Taskmaster::stopProcess(Program &prog, ProcessInfo &proc)
{
	if (proc.pid <= 0)
		return;

	log("Stopping PID " + std::to_string(proc.pid));

	kill(proc.pid, prog.config.stopsignal);
	sleep(prog.config.stoptime);
	kill(proc.pid, SIGKILL);
}

void Taskmaster::checkProcessStatus(Program &prog, ProcessInfo &proc)
{
	int status = 0;
	pid_t result = waitpid(proc.pid, &status, WNOHANG);

	if (result == 0 || result == -1)
		return;
	proc.state = ProcessState::EXITED;
	proc.exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	bool expected = false;
	for (int c : prog.config.exitcodes)
		if (c == proc.exitcode)
			expected = true;
	if (!expected && prog.config.autorestart == ProgramRestart::UNEXPECTED)
		spawnProcess(prog, proc);
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
			{
				perror("chdir");
				_exit(1);
			}
		}
		umask(prog.config.umask_value);
		for (auto &tmp : prog.config.env)
			setenv(tmp.first.c_str(), tmp.second.c_str(), 1);
		if (!prog.config.stdout_file.empty())
		{
			int fd = open(prog.config.stdout_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd < 0)
			{
				perror("open stdout");
				_exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		if (!prog.config.stderr_file.empty())
		{
			int fd = open(prog.config.stderr_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd < 0)
			{
				perror("open stderr");
				_exit(1);
			}
			dup2(fd, STDERR_FILENO);
			close(fd);
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
		perror("execvp");
		for (size_t i = 0; i < parts.size(); ++i)
			free(argv[i]);
		delete [] argv;
		_exit(1);
	}

	proc.pid = pid;
	proc.state = ProcessState::STARTING;
	proc.start_timestamp = time(nullptr);

	log("Spawned PID " + std::to_string(pid));
}
