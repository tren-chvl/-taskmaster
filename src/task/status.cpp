#include "taskmaster.hpp"



std::string Taskmaster::formatUptime(time_t start)
{
	if (start == 0)
		return "0s";
	time_t now = time(nullptr);
	time_t diff = now - start;
	int h = diff / 3600;
	int m = (diff % 3600) / 60;
	int s = diff % 60;
	char buf[64];
	snprintf(buf, sizeof(buf), "%02dh:%02dm:%02ds", h, m, s);
	return  (buf);
}

void Taskmaster::status()
{
	for (auto &pair : programs)
	{
		const std::string &name = pair.first;
		Program &prog = pair.second;
		std::cout << "Program: " << name << "\n";
		std::cout << "  numprocs: " << prog.config.numprocs << "\n";
		for (auto &proc : prog.processes)
		{
			std::cout << "    PID: " << proc.pid << " | State: " << colorState(proc.state)
					  << " | Exit: " << proc.exitcode << " | Uptime: " << formatUptime(proc.start_timestamp)
					  << " | Retries: " << proc.retries << "\n";
		}
		std::cout << "\n";
	}
}

std::string Taskmaster::resolvePath(const std::string &path, const std::string &workindir)
{
	if (path.empty())
		return path;
	if (path[0] == '/')
		return path;
	return workindir +"/" + path;
}