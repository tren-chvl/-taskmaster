#include "taskmaster.hpp"


void Taskmaster::stopProcess(Program &prog, ProcessInfo &proc)
{
	if (proc.id <= 0)
		return;
	log("Stopping PID " + std::to_string(proc.pid));
	kill(proc.pid, prog.config.stopsignal);
	sleep(proc.config.stoptime);
	kill(proc.pid, SIGKILL);
}

void Taskmaster::checkProcessStatus(Program &prog, ProcessInfo &proc)
{
	int status = 0;
	pid_t result = waitpid(proc.pid, &status, WNOHANG);
	if (result == 0)
		return;
	if (result == -1)
		return;
	proc.state = ProcessState::EXITED;
	int exitcode = WEXITSTATUS(status);
	bool expected = false;
	for (int c : prog.config.exitcode)
		if (c == exitcode)
			expected = true;
	if (!expected && prog.config.autorestart == ProcessRestart::UNEXPECTED)
		spawnProcess(prog, proc);
}

void handleSignals()
{
	for (auto &pair : Programs)
	{
		Program &prog = pair.second;
		for(auto &proc : prog.processes)
			checkProcessStatus(prog, proc);
	}
}

void Taskmaster::spawnProcess(Program &prog, ProcessInfo &proc)
{
	log("Spawning process for: " + prog.config.name);
	pid_t pid = fork();
	if (pid < 0)
	{
		log("ERRORL: fork() failed\n");
		return;
	}
	if (pid == 0)
	{
		if (!prog.config.workingdir.empty())
		{
			if (chdir(prog.config.workingdir.c_str()) != 0)
			{
				perror("chdir");
				exit(1);
			}
		}
		umask(prog.config.umask_value);
		for(auto &tmp : prog.config.env)
			setenv(tmp.first.c_str(), tmp.second.c_str(), 1);
		if (!prog.config.stdout_file.empty())
		{
			int fd =  open(prog.config.stdout_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644)
			if (fd < 0)
			{
				perror("open stdout");
				close(fd);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		if (!prog.config.stderr_file.empty())
		{
			int fd = open(prog.config.stderr_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644)
			if (fd < 0)
			{
				perror("open stderr")
				exit(1);
			}
			dup2(fd, STDERR_FILENO);
			close(fd);
		}
		std::vector<std::string> parts;
		std::string tmp;
		std::istringstream iss(prog.config.cmd);
		while(iss >> tmp)
			parts.push_back(tmp);
		char **argv = new char *[parts.size() + 1];
		for(size_t i = 0; i < parts.size(); i++)
			argv[i] = stdup(parts[i].c_str());
		argv[parts.size()] = nullptr;
		execvp(argv[0], argv);
		perror("execvp");
		exit(1);
	}
	proc.pid = pid;
	proc.state = ProcessState::STARTING;
	proc.start_timestamp = time(nullptr);
	log("Spawned PID " + std::to_string(pid));
}
