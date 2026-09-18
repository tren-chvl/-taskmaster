#include "taskmaster.hpp"


void Taskmaster::reloadConfigDiff()
{
	std::lock_guard<std::mutex> lock(programs_mutex);
	log("Reloadding conifguration ");
	auto old_prog = programs;
	loadConfig();
	auto &new_prog = programs;
	for (auto &old : old_prog)
	{
		const std::string &name = old.first;
		if (new_prog.find(name) == new_prog.end())
		{
			log("Program removed: " + name);
			stopProgram(old.second);
			programs.erase(name);
		}
	}
	for (auto &tmp : new_prog)
	{
		const std::string &name = tmp.first;
		if (old_prog.find(name) == old_prog.end())
		{
			log("New program added: " + name);
			startProgram(tmp.second);
		}
	}
	for(auto &tmp : new_prog)
	{
		const std::string &name = tmp.first;
		if (old_prog.find(name) != old_prog.end())
		{
			Program &oldprog = old_prog[name];
			Program &newprog = tmp.second;
			if (!compareProgramConfig(oldprog.config, newprog.config))
			{
				log("Program changed: " + name);
				stopProgram(oldprog);
				startProgram(newprog);
			}
		}
	}
	log("Reload diff complete.");
}





bool Taskmaster::compareProgramConfig(const ProgramConfig &old_prog, const ProgramConfig &new_prog)
{
	bool same = true;

	if (old_prog.cmd != new_prog.cmd)
		same = false;
	if (old_prog.numprocs != new_prog.numprocs)
		same = false;
	if (old_prog.autorestart != new_prog.autorestart)
		same = false;
	if (old_prog.starttime != new_prog.starttime)
		same = false;
	if (old_prog.startretries != new_prog.startretries)
		same = false;
	if (old_prog.stoptime != new_prog.stoptime)
		same = false;
	if (old_prog.exitcodes != new_prog.exitcodes)
		same = false;
	if (old_prog.workingdir != new_prog.workingdir)
		same = false;
	if (old_prog.umask_value != new_prog.umask_value)
		same = false;	
	if (old_prog.stdout_file != new_prog.stdout_file)
		same = false;
	if (old_prog.stderr_file != new_prog.stderr_file)
		same = false;
	if (old_prog.env != new_prog.env)
		same = false;
	return same;
}


bool Taskmaster::validateProgramConfig(const ProgramConfig &cfg)
{
	if (cfg.name.empty())
		return false;
	if (cfg.cmd.empty())
		return false;
	if (cfg.numprocs <= 0)
		return false;
	if (cfg.startretries < 0)
		return false;
	if (cfg.stoptime < 0)
		return false;
	if (cfg.umask_value < 0 || cfg.umask_value > 0777)
		return false;
	return true;
}
