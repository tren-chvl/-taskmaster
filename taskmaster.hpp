#ifndef TASKMASTER_HPP
#define TASKMASTER_HPP

#include <csignal>
#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <mutex>
#include <sys/stat.h>
#include <jsoncpp/json/json.h>
#include <thread>
#include <chrono>



#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"

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

enum class LogLevel
{
	INFO,
	WARN,
	ERROR
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
};

struct ProcessInfo
{
	pid_t pid = -1;
	ProcessState state = ProcessState::STOPPED;
	int retries = 0;
	time_t start_timestamp = 0;
	int exitcode = -1;
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
		std::string signalName(int sig);
		bool validateProgramConfig(const ProgramConfig &cfg);
		bool compareProgramConfig(const ProgramConfig &old_prog, const ProgramConfig &new_prog);
		void reloadConfigDiff();
		void startProgram(Program &prog);
		void startProgram(const std::string &name);
		void stopProgram(Program &prog);
		void stopProgram(const std::string &name);
		void restartProgram(const std::string &name);
		void startAutostart();
		void superviseLoop();
		void runServer();
		std::string getStatusString();
		std::string handleCommand(const std::string &cmd);
		std::string resolvePath(const std::string &path, const std::string &workindir);
		void checkProcessStatus(Program &prog, ProcessInfo &proc);
		void spawnProcess(Program &prog, ProcessInfo &proc);
		void stopProcess(Program &prog, ProcessInfo &proc);
		void shell();
		void status();
		void log(const std::string &msg);
		void log(const std::string &msg, LogLevel lvl);
		void supervision();
		bool handleAutorestart(Program &prog, ProcessInfo &proc, int exitcode);
		std::string colorState(ProcessState st);
		std::string formatUptime(time_t start);

	private:	
		std::string config_path;
		std::map<std::string, Program> programs;
		std::mutex programs_mutex;
		void ensureLogArchiveDir();
		bool isLogTooBig(const char *path);
		void rotateLog();
		std::string timestamp();
		std::string levelToString(LogLevel lvl);
};


#endif
