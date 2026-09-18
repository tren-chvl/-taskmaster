#include "taskmaster.hpp"



void Taskmaster::ensureLogArchiveDir()
{
	mkdir("/var/log/taskmaster/archive", 0755);
}



bool Taskmaster::isLogTooBig(const char *path)
{
	struct stat st{};
	if (stat(path, &st) == -1)
		return false;
	const off_t max_size = 1 * 1024 * 1024;
	return st.st_size >= max_size;
}


void Taskmaster::rotateLog()
{
	const char *log_path = "/var/log/taskmaster.log";
	if (!isLogTooBig(log_path))
		return;
	ensureLogArchiveDir();
	std::time_t t = std::time(nullptr);
	std::tm tm{};
	localtime_r(&t, &tm);

	std::ostringstream oss;
	oss << "/var/log/taskmaster/archive/taskmaster_" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".log";
	std::string archive_path = oss.str();
	rename(log_path, archive_path.c_str());
}


void Taskmaster::log(const std::string &msg, LogLevel lvl)
{
	const char *path = "/var/log/taskmaster.log";
	rotateLog();
	std::ofstream ofs(path, std::ios::app);
	if (!ofs)
	{
		std::cerr << "[LOGGER ERROR] Cannot open log file: " << path << "\n";
		return;
	}
	ofs << timestamp() << " [" << levelToString(lvl) << "] "<< msg << "\n";
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


std::string Taskmaster::timestamp()
{
	std::time_t t = std::time(nullptr);
	std::tm tm{};
	localtime_r(&t, &tm);

	char buf[64];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
	return buf;
}
