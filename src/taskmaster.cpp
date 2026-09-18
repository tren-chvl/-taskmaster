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
		log("ERROR: Cannot open config file");
		return;
	}
	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;
	if (!Json::parseFromStream(builder, file, &root, &errs))
	{
		log("ERROR: Invalid JSON: " + errs);
		return;
	}
	if (!root.isMember("programs") || !root["programs"].isObject())
	{
		log("ERROR: Config missing 'programs' object");
		return;
	}
	programs.clear();
	for (const std::string &name : root["programs"].getMemberNames())
	{
		Json::Value p = root["programs"][name];
		if (!p.isObject())
		{
			log("ERROR: Program '" + name + "' is not an object");
			continue;
		}
		ProgramConfig cfg;
		cfg.name = name;
		if (!p.isMember("cmd"))
		{
			log("ERROR: Program '" + name + "' missing 'cmd'");
			continue;
		}
		cfg.cmd = p["cmd"].asString();
		cfg.numprocs = p.get("numprocs", 1).asInt();
		cfg.autostart = p.get("autostart", false).asBool();
		std::string ar = p.get("autorestart", "unexpected").asString();
		if (ar == "always")
			cfg.autorestart = ProgramRestart::ALWAYS;
		else if (ar == "never")
			cfg.autorestart = ProgramRestart::NEVER;
		else
			cfg.autorestart = ProgramRestart::UNEXPECTED;
		for (auto &c : p["exitcodes"])
			cfg.exitcodes.push_back(c.asInt());
		cfg.startretries = p.get("startretries", 3).asInt();
		cfg.starttime = p.get("starttime", 1).asInt();
		cfg.stopsignal = SIGTERM;
		cfg.stoptime = p.get("stoptime", 5).asInt();
		cfg.stdout_file = p.get("stdout", "").asString();
		cfg.stderr_file = p.get("stderr", "").asString();
		cfg.workingdir = p.get("workingdir", "").asString();
		cfg.umask_value = p.get("umask", 022).asInt();
		if (p.isMember("env"))
		{
			for (auto &key : p["env"].getMemberNames())
				cfg.env[key] = p["env"][key].asString();
		}
		if (!validateProgramConfig(cfg))
		{
			log("ERROR: Invalid config for program: " + name);
			continue;
		}
		Program prog;
		prog.config = cfg;
		prog.processes.resize(cfg.numprocs);
		programs[name] = prog;
	}
	log("Configuration loaded successfully");
}


void Taskmaster::log(const std::string &msg)
{
	LogLevel lvl = LogLevel::INFO;

	if (msg.find("error") != std::string::npos || msg.find("ERROR") != std::string::npos ||
		msg.find("fatal") != std::string::npos || msg.find("FATAL") != std::string::npos)
		lvl = LogLevel::ERROR;
	else if (msg.find("warn") != std::string::npos || msg.find("WARN") != std::string::npos || msg.find("changed") != std::string::npos)
		lvl = LogLevel::WARN;
	log(msg, lvl);
}


