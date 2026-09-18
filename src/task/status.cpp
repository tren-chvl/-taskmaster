#include "taskmaster.hpp"



std::string Taskmaster::formatUptime(time_t start)
{
	if (start == 0)
		return "0s";
	time_t now = time(nullptr);
	time_t diff = now - start;
	int h = diff / 3600;
	int m = (diff % 3600) / 60;
	int s = diff % 60;
	char buf[64];
	snprintf(buf, sizeof(buf), "%02dh:%02dm:%02ds", h, m, s);
	return  (buf);
} 