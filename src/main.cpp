#include "taskmaster.hpp"

taskmaster *g_taskmaster = nullptr;

void signal_handler(int sig)
{
	if (!g_taskmaster)
		return;
	if (sig == SIGHUP)
	{
		g_taskmaster->log("Received SIGHUP: reloading configuration");
		g_taskmaster->applyConfigChanges();
	}
	else if (sig == SIGCHLD)
	{
		g_taskmaster->handleSignals();
	}
}


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0]  << " <config.json\n>";
		return 1;
	}
	std::string config_path = argv[1];
	Taskmaster task(config_path);
	task.loadConfig();
	std::signal(SIGHUP, signal_handler);
	std::signal(SIGCHLD, signal_handler);
	task.startAutostart();
	task.shell();
	return 0;
}