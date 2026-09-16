#ifndef TASKMASTER_HPP
#define TASKMASTER_HPP

#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctime>
#include <iostream>



enum class ProgramRestart
{
	ALWAYS,
	NEVER,
	UNEXPECTED
};

enum class ProcessState
{
	STOPPED,
	STARTING,
	RUNNING,
	BACKOFF,
	FATAL,
	EXITED
};

struct ProgramConfig
{
	std::string name;
	std::string cmd;
	std::string stdout_file;
	std::string stderr_file;
	int numprocs = 1;
	int umask_value = 022;
	int startretries = 3;
	int starttime = 5;
	int stopsignal = SIGTERM;
	int stoptime = 10;
	bool autostart = false;
	ProgramRestart autorestart = ProgramRestart::UNEXPECTED;
	std::vector<int> exitcodes;
	std::map<std::string, std::string> env;
	std::string workingdir;
}

struct ProcessInfo
{
	pid_t pid = -1;
	ProcessState state = ProcessState::STOPPED;
	int retries = 0;
	time_t start_timestamp = 0;
};

struct Program
{
	ProgramConfig config;
	std::vector<ProcessInfo> processes;
};


class Taskmaster 
{
	public:
		Taskmaster(const std::string &config_path);

		void loadConfig();
		void applyConfigChanges();
		void startAutostart();

		void startProgram(const std::string &name);
		void stopProgram(const std::string &name);
		void restartProgram(const std::string &name);

		void superviseLoop();
		void handleSignals();

		void shell();

		void log(const std::string &msg);

	private:
		std::string config_path;
		std::map<std::string, Program> programs;

		void spawnProcess(Program &prog, ProcessInfo &proc);
		void stopProcess(Program &prog, ProcessInfo &proc);
		void checkProcessStatus(Program &prog, ProcessInfo &proc);
};
#endif  

