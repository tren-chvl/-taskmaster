#include "taskmaster.hpp"

Taskmaster *g_taskmaster = nullptr;


void signal_handler(int sig)
{
	if (!g_taskmaster)
		return;
	if (sig == SIGHUP)
	{
		g_taskmaster->log("Received SIGHUP: reloading configuration");
		g_taskmaster->reloadConfigDiff();
	}
	else if (sig == SIGCHLD){}
}


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config.json>\n";
		return 1;
	}
	std::string config_path = std::filesystem::absolute(argv[1]).string();
	Taskmaster task(config_path);
	g_taskmaster = &task;
	task.loadConfig();
	if (!dropPrivileges("taskmaster"))
	{
		std::cerr << "ERROR: Could not drop privileges\n";
		return 1;
	}
	task.log("Privileges dropped: running as taskmaster");
	std::signal(SIGHUP, signal_handler);
	std::signal(SIGCHLD, signal_handler);
	std::thread supervisor(&Taskmaster::supervision, &task);
	supervisor.detach();
	std::thread server(&Taskmaster::runServer, &task);
	server.detach();
	task.startAutostart();
	task.shell();
	return 0;
}