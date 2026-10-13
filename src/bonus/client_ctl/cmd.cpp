#include "taskmaster.hpp"

std::string Taskmaster::handleCommand(const std::string &cmd)
{
	if (cmd == "status")
		return getStatusString();
	if (cmd.rfind("start ", 0) == 0)
	{
		startProgram(cmd.substr(6));
		return "OK\n";
	}
	if (cmd.rfind("stop ", 0) == 0)
	{
		stopProgram(cmd.substr(5));
		return "OK\n";
	}
	if (cmd.rfind("restart ", 0) == 0)
	{
		restartProgram(cmd.substr(8));
		return "OK\n";
	}
	if (cmd == "reload")
	{
		reloadConfigDiff();
		return "OK\n";
	}
	if (cmd.rfind("attach ", 0) == 0)
		return "ERROR: attach must be handled by the client handler\n";
	if (cmd == "quit")
		return "ERROR: quit is not supported through this handler\n";
	return "ERROR: unknown command\n";
}

void Taskmaster::handleClient(int client_fd)
{
	char buffer[1024];
	ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
	if (n <= 0)
	{
		close(client_fd);
		return;
	}
	buffer[n] = '\0';
	std::string cmd(buffer);
	while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r'))
	{
		cmd.pop_back();
	}
	if (cmd.rfind("attach ", 0) == 0)
	{
		std::string name = cmd.substr(7);
		while (!name.empty() && (name.back() == '\n' || name.back() == '\r'))
		{
			name.pop_back();
		}
		handleAttach(client_fd, name);
		return;
	}
	std::string reply = handleCommand(cmd);
	size_t total = 0;
	while (total < reply.size())
	{
		ssize_t written = write(client_fd, reply.c_str() + total, reply.size() - total);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0)
			break;
		total += static_cast<size_t>(written);
	}
	close(client_fd);
}

