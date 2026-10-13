
#include "taskmaster.hpp"

std::string stateToString(ProcessState state)
{
	switch (state)
	{
		case ProcessState::STOPPED:
			return "STOPPED";
		case ProcessState::STARTING:
			return "STARTING";
		case ProcessState::RUNNING:
			return "RUNNING";
		case ProcessState::BACKOFF:
			return "BACKOFF";
		case ProcessState::EXITED:
			return "EXITED";
		case ProcessState::FATAL:
			return "FATAL";
		default:
			return "UNKNOWN";
	}
}

std::string Taskmaster::getStatusString()
{
	std::stringstream ss;
	std::lock_guard<std::mutex> lock(programs_mutex);
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


void Taskmaster::handleAttach(int client_fd, const std::string &name)
{
	std::shared_ptr<PtySession> session = getPtySession(name);
	if (!session)
	{
		const std::string reply = "ERROR: program not found or no active PTY session\n";
		write(client_fd, reply.c_str(), reply.size());
		close(client_fd);
		return;
	}
	{
		std::lock_guard<std::mutex> lock(session->mutex);
		if (session->attached_fd >= 0)
		{
			const std::string reply = "ERROR: a client is already attached\n";
			write(client_fd, reply.c_str(), reply.size());
			close(client_fd);
			return;
		}
		const std::string reply = "ATTACH_READY: " + name + "\n";
		size_t total = 0;
		while (total < reply.size())
		{
			ssize_t n = write(client_fd,reply.c_str() + total,reply.size() - total);
			if (n < 0 && errno == EINTR)
				continue;
			if (n <= 0)
			{
				close(client_fd);
				return;
			}
			total += static_cast<size_t>(n);
		}
		session->attached_fd = client_fd;
	}
	char buffer[4096];
	while (true)
	{
		ssize_t n = read(client_fd, buffer, sizeof(buffer));
		if (n > 0)
		{
			size_t total = 0;
			while (total < static_cast<size_t>(n))
			{
				ssize_t written = write( session->master_fd, buffer + total,static_cast<size_t>(n) - total);
				if (written < 0 && errno == EINTR)
					continue;
				if (written <= 0)
					break;
				total += static_cast<size_t>(written);
			}
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		break;
	}
	{
		std::lock_guard<std::mutex> lock(session->mutex);
		session->attached_fd = -1;
	}
	close(client_fd);
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
	std::strncpy(addr.sun_path, "/tmp/taskmaster.sock",sizeof(addr.sun_path) - 1);
	unlink("/tmp/taskmaster.sock");
	if (bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
	{
		log("ERROR: bind() failed");
		close(server_fd);
		return;
	}
	if (chmod("/tmp/taskmaster.sock", 0660) == -1)
	{
		log("ERROR: chmod socket failed: " + std::string(std::strerror(errno)));
		close(server_fd);
		unlink("/tmp/taskmaster.sock");
		return;
	}
	if (listen(server_fd, 10) < 0)
	{
		log("ERROR: listen() failed");
		close(server_fd);
		unlink("/tmp/taskmaster.sock");
		return;
	}
	log("Server listening on /tmp/taskmaster.sock");
	while (true)
	{
		int client_fd = accept(server_fd, nullptr, nullptr);
		if (client_fd < 0)
		{
			if (errno == EINTR)
				continue;
			log("ERROR: accept() failed");
			continue;
		}
		std::thread(&Taskmaster::handleClient,this,client_fd).detach();
	}
}
