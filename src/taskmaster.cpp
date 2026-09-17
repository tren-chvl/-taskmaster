#include "taskmaster.hpp"

Taskmaster::Taskmaster(const std::string &path) : config_path(path)
{
	log("Taskmaster initalized");
}


void Taskmaster::loadConfig()
{
	log("Loading configuration: " + config_path);
	std::ifstream file(config_path);
	if (!file.is_open())
	{
		log("ERROR: Cannot open config file")
		return;
	}
	Json::value root;
	file >> root;
	programs.clear();
	for (auto &name : root["programs"].getMemberNames())
	{
		Json::Value p = root["progams"][name];
		ProgramConfig cfg;
		cfg.name = name;
		cfg.cmd = p["cmd"].asString();
		cfg.numprocs = p["numprocs"].asInt();
		cfg.autostart = p["autostart"].asBool();
		std::string ar = p["autorestart"].asString();
		if (ar == "always")
			cfg.autorestart =  ProgramRestart::ALWAYS;
		else if (ar == "never")
			cfg.autorestart = ProgramRestart::NEVER;
		else
			cfg.autorestart = ProcessRestart::UNEXPECTED;
		for (auto &c : p["exitcodes"])
			cfg.exitcodes.push_back(c.asInt());
		cfg.startretries = p["startretries"].asInt();
		cfg.starttime = p["starttime"].asInt();
		cfg.stopsignal = SIGTERM;
		cfg.stoptime = p["stoptime"].asInt();
		cfg.stdout_file = p["stdout"].asString();
		cfg.stderr_file = p["stderr"].asString();
		cfg.workingdir = p["workingdir"].asString();
		cfg.umask_value = p["umask"].asInt();

		for (auto &key : p["env"]. getMemberNames())
			cfg.env[key] = p["env"][key].asString();
		Program prog;
		prog.config = cfg;
		prog.processes.resize(cfg.numprocs);
		programs[name] = prog;
	}
	log("Configuration loaded successfully");
}


void Taskmaster::log(const std::string &msg)
{
	std::cout << "[LOG] " << msg << std::endl;
}


