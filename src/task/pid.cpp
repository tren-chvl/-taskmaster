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
        // CHILD
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        std::cerr << "CHILD initial cwd = " << cwd << "\n";

        // 1) Changer de répertoire AVANT tout
        if (!prog.config.workingdir.empty())
        {
            if (chdir(prog.config.workingdir.c_str()) != 0)
            {
                perror("chdir");
                _exit(1);
            }
        }

        // Vérifier où on est
        getcwd(cwd, sizeof(cwd));
        std::cerr << "CHILD after chdir cwd = " << cwd << "\n";

        // 2) Résoudre les chemins
        std::string stdout_path = resolvePath(prog.config.stdout_file, prog.config.workingdir);
        std::string stderr_path = resolvePath(prog.config.stderr_file, prog.config.workingdir);

        // 3) Créer les dossiers au bon endroit
        mkdir("logs", 0755);
        mkdir("logs/archive", 0755);

        // 4) Redirections
        if (!prog.config.stdout_file.empty())
        {
            int fd = open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
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
            int fd = open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0)
            {
                perror("open stderr");
                _exit(1);
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        // 5) Préparer argv
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

        // 6) Lancer le programme
        execvp(argv[0], argv);

        perror("execvp");
        for (size_t i = 0; i < parts.size(); ++i)
            free(argv[i]);
        delete [] argv;
        _exit(1);
    }

    // PARENT
    proc.pid = pid;
    proc.state = ProcessState::STARTING;
    proc.start_timestamp = time(nullptr);
    proc.retries = 0;

    log("Spawned PID " + std::to_string(pid));
}
