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
		proc.retries = 0;
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
	{
		stopProcess(prog, proc);
		proc.state = ProcessState::STOPPED;
		proc.pid = -1;
		proc.exitcode = 0;
		proc.retries = 0;
	}
}

void Taskmaster::restartProgram(const std::string &name)
{
	stopProgram(name);
	startProgram(name);
}



bool Taskmaster::handleAutorestart(Program &prog, ProcessInfo &proc, int exitcode)
{
    // -----------------------------
    // 1. Déterminer si on doit restart
    // -----------------------------
    bool restart = false;

    if (prog.config.autorestart == ProgramRestart::ALWAYS)
    {
        restart = true;
    }
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

    // Si pas de restart → STOP
    if (!restart)
        return false;

    // -----------------------------
    // 2. Trop de retries → FATAL
    // -----------------------------
    proc.retries++;
    if (proc.retries > prog.config.startretries)
    {
        proc.state = ProcessState::FATAL;
        log("Process " + prog.config.name + " exceeded retries -> FATAL");
        return false;
    }

    // -----------------------------
    // 3. BACKOFF si mort trop vite
    // -----------------------------
    time_t now = time(nullptr);

    if (now - proc.start_timestamp < prog.config.starttime)
    {
        proc.state = ProcessState::BACKOFF;
        log("Process " + prog.config.name + " died too fast -> BACKOFF");

        // Attendre un peu avant restart (Supervisor fait pareil)
        std::this_thread::sleep_for(std::chrono::seconds(1));

        log("BACKOFF: retrying process " + prog.config.name);
        spawnProcess(prog, proc);
        proc.state = ProcessState::STARTING;
        proc.start_timestamp = time(nullptr);
        return true;
    }

    // -----------------------------
    // 4. Restart normal
    // -----------------------------
    log("Restarting process " + prog.config.name);
    spawnProcess(prog, proc);
    proc.state = ProcessState::STARTING;
    proc.start_timestamp = time(nullptr);
    return true;
}
