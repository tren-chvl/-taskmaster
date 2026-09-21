#include "taskmaster.hpp"

std::string trim(const std::string &s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

void Taskmaster::shell()
{
	log("Starting interactive Shell");

	std::string line;
	while (true)
	{
		std::cout << "taskmaster> ";
		if (!std::getline(std::cin, line))
			break;
		line = trim(line);
		if (line.empty())
			continue;
		if (line == "quit" || line == "exit")
			break;
		if (line == "status")
		{
			status();
			continue;
		}
		if (line == "reload")
		{
			reloadConfigDiff();
			continue;
		}
		if (line.rfind("start ", 0) == 0)
		{
			startProgram(trim(line.substr(6)));
			continue;
		}
		if (line.rfind("stop ", 0) == 0)
		{
			stopProgram(trim(line.substr(5)));
			continue;
		}
		if (line.rfind("restart ", 0) == 0)
		{
			restartProgram(trim(line.substr(8)));
			continue;
		}
		std::cout << "Unknown command\n";
	}
}
