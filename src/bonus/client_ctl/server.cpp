#include "taskmaster.hpp"


std::string Taskmaster::getStatusString()
{
	std::stringstream ss;
	for (auto &pair : programs)
	{
		ss << "Program: " << pair.first << "\n";
		for (auto &proc : pair.second.processes)
		{
			ss << "  PID: " << proc.pid
			   << " | State: " << stateToString(proc.state)
			   << " | Exit: " << proc.exitcode
			   << " | Retries: " << proc.retries << "\n";
		}
	}
	return ss.str();
}


std::string Taskmaster::handleCommand(const std::string &cmd)
{
	if (cmd == "status")
		return getStatusString();
	if (cmd.rfind("start ", 0) == 0)
	{
		std::string name = cmd.substr(6);
		startProgram(name);
		return "OK\n";
	}
	if (cmd.rfind("stop ", 0) == 0)
	{
		std::string name = cmd.substr(5);
		stopProgram(name);
		return "OK\n";
	}
	if (cmd.rfind("restart ", 0) == 0)
	{
		std::string name = cmd.substr(8);
		restartProgram(name);
		return "OK\n";
	}
	if (cmd == "reload")
	{
		reloadConfigDiff();
		return "OK\n";
	}
	if (cmd == "shutdown")
		exit(0);
	return "ERROR: unknown command\n";
}


void Taskmaster::runServer()
{
	int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		log("ERROR: socket() failed");
		return;
	}
	sockaddr_un addr{};
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, "/tmp/taskmaster.sock");
	unlink("/tmp/taskmaster.sock");
	if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
	{
		log("ERROR: bind() failed");
		close(server_fd);
		return;
	}
	if (listen(server_fd, 10) < 0)
	{
		log("ERROR: listen() failed");
		close(server_fd);
		return;
	}
	log("Server listening on /tmp/taskmaster.sock");
	while (true)
	{
		int client_fd = accept(server_fd, nullptr, nullptr);
		if (client_fd < 0)
			continue;
		char buffer[1024];
		int n = read(client_fd, buffer, sizeof(buffer)-1);
		if (n <= 0)
		{
			close(client_fd);
			continue;
		}

		buffer[n] = '\0';
		std::string cmd(buffer);
		std::string reply = handleCommand(cmd);
		write(client_fd, reply.c_str(), reply.size());
		close(client_fd);
	}
}