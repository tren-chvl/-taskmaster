#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <string>

int main()
{
	while (true)
	{
		std::cout << "taskmasterctl> ";
		std::string cmd;
		std::getline(std::cin, cmd);
		if (cmd.empty())
			continue;
		int fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd < 0)
		{
			perror("socket");
			continue;
		}
		sockaddr_un addr{};
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, "/tmp/taskmaster.sock");
		if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
		{
			perror("connect");
			close(fd);
			continue;
		}
		write(fd, cmd.c_str(), cmd.size());
		char buffer[4096];
		int n = read(fd, buffer, sizeof(buffer)-1);
		if (n > 0)
		{
			buffer[n] = '\0';
			std::cout << buffer << "\n";
		}
		else
		{
			std::cout << "No response from daemon\n";
		}
		close(fd);
		if (cmd == "quit" || cmd == "exit")
			break;
	}
	return 0;
}
