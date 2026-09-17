#include "taskmaster.hpp"



void Taskmaster::applyConfigChanges()
{
	log("reloading configuration...");
	loadConfig();
}

void Taskmaster::startAutostart()
{
	for (auto &pair : progams)
	{
		Program &prog = pair.second;
		if (prog.config.autostart)
			startProgram(prog.config.name);
	}
}


void Taskmaster::startProgram(const std::string &name)
{
	if (!programs.count(name))
	{
		log("ERROR: program not found: " + name);
		return;
	}
	Program &prog = programs[name];
	log("Starting program: " + name);
	for (auto &proc : prog.processes)
		spawnProcess(prog, proc);
}


void Taskmaster::stopProgram(const std::string &name)
{
	if (!programs.count (name))
	{
		log("ERROR: Program not found: " + name);
		return;
	}
	Program &prog = programs[name];
	log("Stoppping program: " + name);
	for (auto &proc : prog.processes)
		stopProcess(prog, proc);
}

void Taskmaster::restartProgram(const std::string &name)
{
	stopProgram(name);
	startProgram(name);
}