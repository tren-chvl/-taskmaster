#include "taskmaster.hpp"


void Taskmaster::supervision()
{
	while (true)
	{
		std::lock_guard<std::mutex> lock(programs_mutex);
		for (auto &pair : programs)
		{
			Program &prog = pair.second;
			for (auto &proc : prog.processes)
			{
				if (proc.state == ProcessState::STARTING)
				{
					time_t now = time(nullptr);
					if (now - proc.start_timestamp >= prog.config.starttime)
					{
						proc.state = ProcessState::RUNNING;
						log("Process " + prog.config.name + " is now RUNNING");
					}
				}
				if (proc.pid > 0)
				{
					int status = 0;
					pid_t result = waitpid(proc.pid, &status, WNOHANG);
					if (result == proc.pid)
					{
						int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
						proc.state = ProcessState::EXITED;
						proc.exitcode = exitcode;
						proc.pid = -1;
						log("Process " + prog.config.name + " exited with code " + std::to_string(exitcode));
						handleAutorestart(prog, proc, exitcode);
						continue;
					}
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}
