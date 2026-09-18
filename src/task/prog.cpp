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
