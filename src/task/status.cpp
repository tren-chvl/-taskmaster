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
			std::string state = colorState(proc.state);
			std::string uptime = formatUptime(proc.start_timestamp);
			int exitcode = proc.exitcode;
			if (proc.state == ProcessState::STOPPED || proc.state == ProcessState::EXITED || proc.state == ProcessState::FATAL)
				uptime = "0s";
			std::cout << "    PID: " << proc.pid
					  << " | State: " << state
					  << " | Exit: " << exitcode
					  << " | Uptime: " << uptime
					  << " | Retries: " << proc.retries
					  << "\n";
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
