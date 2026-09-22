#include "taskmaster.hpp"

void Taskmaster::startAutostart()
{
	for (auto &pair : programs)
	{
		Program &prog = pair.second;
		if (prog.config.autostart)
			startProgram(prog.config.name);
	}
}

void Taskmaster::startProgram(const std::string &name)
{
	std::lock_guard<std::mutex> lock(programs_mutex);
	if (!programs.count(name))
	{
		log("ERROR: program not found: " + name);
		return;
	}
	Program &prog = programs[name];
	log("Starting program: " + name);
	startProgram(prog);
}

void Taskmaster::startProgram(Program &prog)
{
	log("Starting program: " + prog.config.name);
	for (auto &proc : prog.processes)
	{
		proc.state = ProcessState::STARTING;
		spawnProcess(prog, proc);
	}
}

void Taskmaster::stopProgram(const std::string &name)
{
	std::lock_guard<std::mutex> lock(programs_mutex);
	if (!programs.count(name))
	{
		log("ERROR: Program not found: " + name);
		return;
	}
	Program &prog = programs[name];
	log("Stopping program: " + name);
	stopProgram(prog);
}

void Taskmaster::stopProgram(Program &prog)
{
	log("Stopping program: " + prog.config.name);
	for (auto &proc : prog.processes)
		stopProcess(prog, proc);
}

void Taskmaster::restartProgram(const std::string &name)
{
	stopProgram(name);
	startProgram(name);
}


bool Taskmaster::handleAutorestart(Program &prog, ProcessInfo &proc, int exitcode)
{
    bool restart = false;

    if (prog.config.autorestart == ProgramRestart::ALWAYS)
        restart = true;

    else if (prog.config.autorestart == ProgramRestart::UNEXPECTED)
    {
        bool expected = false;

        for (int c : prog.config.exitcodes)
        {
            if (c == exitcode)
            {
                expected = true;
                break;
            }
        }

        if (!expected)
            restart = true;
    }

    if (!restart)
        return false;

    proc.retries++;

    if (proc.retries > prog.config.startretries)
    {
        proc.state = ProcessState::FATAL;
        log("Process " + prog.config.name + " exceeded retries -> FATAL");
        return false;
    }

    log("Restarting process " + prog.config.name);

    spawnProcess(prog, proc);
    proc.start_timestamp = time(nullptr);
    proc.state = ProcessState::STARTING;

    return true;
}
