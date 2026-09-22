#include "taskmaster.hpp"

std::string Taskmaster::timestamp()
{
	std::time_t t = std::time(nullptr);
	std::tm tm{};
	localtime_r(&t, &tm);

	char buf[64];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
	return std::string(buf);
}

std::string Taskmaster::levelToString(LogLevel lvl)
{
	switch (lvl)
	{
		case LogLevel::INFO:
			return "INFO";
		case LogLevel::WARN:
			return "WARN";
		case LogLevel::ERROR:
			return "ERROR";
		default:
			return "LOG";
	}
}


void Taskmaster::ensureLogArchiveDir()
{
	mkdir("./logs", 0755);
	mkdir("./logs/archive", 0755);
}

bool Taskmaster::isLogTooBig(const char *path)
{
	struct stat st{};
	if (stat(path, &st) == -1)
		return false;
	return st.st_size >= (1 * 1024 * 1024); // 1 MB
}

void Taskmaster::rotateLog()
{
	const char *log_path = "./logs/taskmaster.log";
	if (!isLogTooBig(log_path))
		return;

	ensureLogArchiveDir();

	std::time_t t = std::time(nullptr);
	std::tm tm{};
	localtime_r(&t, &tm);

	std::ostringstream oss;
	oss << "./logs/archive/taskmaster_" 
		<< std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") 
		<< ".log";

	rename(log_path, oss.str().c_str());
}

void Taskmaster::log(const std::string &msg, LogLevel lvl)
{
	ensureLogArchiveDir();
	const char *path = "./logs/taskmaster.log";
	rotateLog();
	std::ofstream ofs(path, std::ios::app);
	if (!ofs)
	{
		std::cerr << "[LOGGER ERROR] Cannot open log file: " << path << "\n";
		return;
	}
	ofs << timestamp() << " [" << levelToString(lvl) << "] " << msg << "\n";
}
