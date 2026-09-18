#include "taskmaster.hpp"


void Taskmaster::supervision()
{
	while (true)
	{
		std::lock_guard<std::mutex> lock(programs_mutex);
		for (auto &tmp : programs)
		{
			Program &prog = tmp.second;
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
				if (proc.state == ProcessState::BACKOFF)
				{
					std::this_thread::sleep_for(std::chrono::seconds(1));
					continue;
				}
				if (proc.pid <= 0)
					continue;
				int status = 0;
				pid_t result = waitpid(proc.pid, &status, WNOHANG);
				if (result == 0)
					continue;
				else if (result == -1)
				{
					log("waitpid error for PID " + std::to_string(proc.pid));
					continue;
				}
				else
				{
					int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
					log("Process " + std::to_string(proc.pid) + " exited with code " + std::to_string(exitcode));
					proc.state = ProcessState::EXITED;
					proc.exitcode = exitcode;
					proc.pid = -1;
					handleAutorestart(prog, proc, exitcode);
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}


bool Taskmaster::handleAutorestart(Program &prog, ProcessInfo &proc, int exitcode)
{
	bool restart = false;
	if (prog.config.autorestart == ProgramRestart::ALWAYS)
		restart = true;
	else if (prog.config.autorestart == ProgramRestart::UNEXPECTED)
	{
		bool expected = false;
		for (auto code : prog.config.exitcodes)
		{
			if (code == exitcode)
			{
				expected = true;
				break;
			}
		}
		if (!expected)
			restart = true;
	}
	else if (prog.config.autorestart == ProgramRestart::NEVER)
		restart = false;
	if (!restart)
		return false;
	proc.retries++;
	if (proc.retries > prog.config.startretries)
	{
		log("Process " + prog.config.name + " exceeded startretries -> FATAL");
		proc.state = ProcessState::FATAL;
		return false;
	}
	time_t now = time(nullptr);
	if (now - proc.start_timestamp < prog.config.starttime)
	{
		log("Process " + prog.config.name + " died too fast -> BACKOFF");
		proc.state = ProcessState::BACKOFF;
		return false;
	}
	log("Restarting process " + prog.config.name);
	proc.state = ProcessState::STARTING;
	spawnProcess(prog, proc);
	proc.start_timestamp = time(nullptr);
	return true;
}

