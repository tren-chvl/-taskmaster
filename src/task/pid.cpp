#include "taskmaster.hpp"


std::string Taskmaster::signalName(int sig)
{
	switch (sig)
	{
		case SIGTERM:
			return "SIGTERM";
		case SIGKILL:
			return "SIGKILL";
		case SIGUSR1:
			return "SIGUSR1";
		case SIGUSR2:
			return "SIGUSR2";
		default:
			return "UNKNOWN_SIGNAL";
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
	std::string stdoutPath;
	std::string stderrPath;
	if (!prog.config.stdout_file.empty())
		stdoutPath = resolvePath(prog.config.stdout_file, prog.config.workingdir);
	if (!prog.config.stderr_file.empty())
		stderrPath = resolvePath(prog.config.stderr_file, prog.config.workingdir);
	auto ensureLogDirectory = [](const std::string &path) -> bool
	{
		if (path.empty())
			return true;
		std::filesystem::path p(path);
		std::error_code ec;
		if (!p.is_absolute())
			p = std::filesystem::absolute(p, ec);
		if (ec)
			return false;
		std::filesystem::create_directories(p.parent_path(), ec);
		if (ec)
		{
			std::cerr << "[LOGGER ERROR] Cannot create log directory: " << ec.message() << '\n';
			return false;
		}
		return true;
	};
	if (!ensureLogDirectory(stdoutPath) || !ensureLogDirectory(stderrPath))
	{
		log("ERROR: Cannot create log directories");
		return;
	}
	int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
	if (master_fd < 0)
	{
		log("ERROR: posix_openpt() failed: " + std::string(std::strerror(errno)));
		return;
	}
	if (grantpt(master_fd) < 0 || unlockpt(master_fd) < 0)
	{
		log("ERROR: PTY initialization failed");
		close(master_fd);
		return;
	}
	char *slave_name = ptsname(master_fd);
	if (!slave_name)
	{
		log("ERROR: ptsname() failed");
		close(master_fd);
		return;
	}
	std::string slave_path(slave_name);
	pid_t pid = fork();
	if (pid < 0)
	{
		log("ERROR: fork() failed");
		close(master_fd);
		return;
	}
	if (pid == 0)
	{
		close(master_fd);
		if (setsid() < 0)
			_exit(1);
		int slave_fd = open(slave_path.c_str(), O_RDWR);
		if (slave_fd < 0)
			_exit(1);
		if (ioctl(slave_fd, TIOCSCTTY, 0) < 0)
			_exit(1);
		if (dup2(slave_fd, STDIN_FILENO) < 0 || dup2(slave_fd, STDOUT_FILENO) < 0
			|| dup2(slave_fd, STDERR_FILENO) < 0)
			_exit(1);
		if (slave_fd > STDERR_FILENO)
			close(slave_fd);
		if (!prog.config.workingdir.empty()
			&& chdir(prog.config.workingdir.c_str()) != 0)
			_exit(1);
		umask(prog.config.umask_value);
		std::vector<std::string> parts;
		{
			std::istringstream iss(prog.config.cmd);
			std::string tmp;
			while (iss >> tmp)
				parts.push_back(tmp);
		}
		if (parts.empty())
			_exit(1);
		std::vector<char *> argv;
		for (auto &part : parts)
			argv.push_back(const_cast<char *>(part.c_str()));
		argv.push_back(nullptr);
		std::vector<std::string> env_strings;
		std::vector<char *> envp;
		for (char **sys = environ; *sys != nullptr; ++sys)
			envp.push_back(*sys);
		for (auto &kv : prog.config.env)
			env_strings.push_back(kv.first + "=" + kv.second);
		for (auto &entry : env_strings)
			envp.push_back(const_cast<char *>(entry.c_str()));
		envp.push_back(nullptr);
		execve(argv[0], argv.data(), envp.data());
		_exit(127);
	}
	auto session = std::make_shared<PtySession>();
	session->master_fd = master_fd;
	if (!stdoutPath.empty())
	{
		session->stdout_log_fd = open(stdoutPath.c_str(),O_WRONLY | O_CREAT | O_APPEND,0644);
		if (session->stdout_log_fd < 0)
		{
			log("ERROR: Cannot open stdout log: " + std::string(std::strerror(errno)));
			kill(pid, SIGKILL);
			waitpid(pid, nullptr, 0);
			close(master_fd);
			return;
		}
	}
	proc.pid = pid;
	proc.state = ProcessState::STARTING;
	proc.start_timestamp = time(nullptr);
	proc.pty = session;
	std::thread(&Taskmaster::pumpPty, this, session).detach();
	log("Spawned PID " + std::to_string(pid));
}
