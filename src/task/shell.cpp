#include "taskmaster.hpp"


void Taskmaster::shell()
{
	log("Starting  interactive Shell");
	std::string line;
	while(true)
	{
		std::cout << "taskmaster> ";
		if (!std::getline(std::cin, line))
			break;
		if (line == "quit" || line == "exit")
			break;
		else if (line == "status")
			status();
		else if (line.rfind("start ", 0) == 0)
			startProgram(line.substr(6));
		else if (line.rfind("stop ", 0) == 0)
			stopProgram(line.substr(5));
		else if (line.rfind("restart ", 0) == 0)
			restartProgram(line.substr(8));
		else if (line == "reload")
			reloadConfigDiff();
		else
			std::cout << "Unknow command\n";
	}
}


