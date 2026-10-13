#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>

static bool writeAll(int fd, const char *data, size_t size)
{
	size_t total = 0;

	while (total < size)
	{
		ssize_t n = write(fd, data + total, size - total);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return false;
		total += static_cast<size_t>(n);
	}
	return true;
}

static bool attachTerminal(int fd)
{
	std::string reply;
	char c;
	while (true)
	{
		ssize_t n = read(fd, &c, 1);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
		{
			std::cerr << "Connection closed by server\n";
			return false;
		}
		if (c == '\n')
			break;
		reply += c;
		if (reply.size() > 4096)
		{
			std::cerr << "Invalid server response\n";
			return false;
		}
	}
	if (reply.rfind("ATTACH_READY: ", 0) != 0)
	{
		std::cerr << reply << '\n';
		return false;
	}
	std::cout << "Attached. Press Ctrl+] to detach.\n" << std::flush;
	struct termios original;
	if (tcgetattr(STDIN_FILENO, &original) == -1)
	{
		perror("tcgetattr");
		return false;
	}
	struct termios raw = original;
	cfmakeraw(&raw);
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
	{
		perror("tcsetattr");
		return false;
	}
	bool running = true;
	while (running)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		FD_SET(fd, &readfds);
		int max_fd = (STDIN_FILENO > fd) ? STDIN_FILENO : fd;
		int ready = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
			break;
		}
		if (FD_ISSET(fd, &readfds))
		{
			char buffer[4096];
			ssize_t n = read(fd, buffer, sizeof(buffer));
			if (n <= 0)
				break;
			if (!writeAll(STDOUT_FILENO, buffer, static_cast<size_t>(n)))
				break;
		}
		if (FD_ISSET(STDIN_FILENO, &readfds))
		{
			char buffer[4096];
			ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
			if (n <= 0)
				break;
			for (ssize_t i = 0; i < n; ++i)
			{
				if (static_cast<unsigned char>(buffer[i]) == 0x1d)
				{
					running = false;
					break;
				}
				if (!writeAll(fd, &buffer[i], 1))
				{
					running = false;
					break;
				}
			}
		}
	}
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original) == -1)
		perror("tcsetattr restore");
	std::cout << "\nDetached.\n" << std::flush;
	return true;
}

int main()
{
	while (true)
	{
		std::cout << "taskmasterctl> " << std::flush;
		std::string cmd;
		if (!std::getline(std::cin, cmd))
			break;
		if (cmd.empty())
			continue;
		if (cmd == "quit" || cmd == "exit")
			break;
		int fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd < 0)
		{
			perror("socket");
			continue;
		}
		sockaddr_un addr{};
		addr.sun_family = AF_UNIX;
		std::strncpy(addr.sun_path, "/tmp/taskmaster.sock", sizeof(addr.sun_path) - 1);
		if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
		{
			perror("connect");
			close(fd);
			continue;
		}
		std::string request = cmd + "\n";
		if (!writeAll(fd, request.c_str(), request.size()))
		{
			perror("write");
			close(fd);
			continue;
		}
		if (cmd.rfind("attach ", 0) == 0)
		{
			attachTerminal(fd);
			close(fd);
			continue;
		}
		char buffer[4096];
		while (true)
		{
			ssize_t n = read(fd, buffer, sizeof(buffer));
			if (n < 0 && errno == EINTR)
				continue;
			if (n <= 0)
				break;
			std::cout.write(buffer, n);
		}
		close(fd);
	}
	return 0;
}
