#include "taskmaster.hpp"

bool dropPrivileges(const char *username)
{
	if (geteuid() != 0)
	{
		std::cerr << "ERROR: Taskmaster must start as root " << "to drop privileges\n";
		return false;
	}
	struct passwd *pw = getpwnam(username);
	if (!pw)
	{
		std::cerr << "ERROR: User '" << username << "' does not exist\n";
		return false;
	}
	if (initgroups(pw->pw_name, pw->pw_gid) == -1)
	{
		std::cerr << "ERROR: initgroups failed: " << std::strerror(errno) << '\n';
		return false;
	}
	if (setgid(pw->pw_gid) == -1)
	{
		std::cerr << "ERROR: setgid failed: " << std::strerror(errno) << '\n';
		return false;
	}
	if (setuid(pw->pw_uid) == -1)
	{
		std::cerr << "ERROR: setuid failed: " << std::strerror(errno) << '\n';
		return false;
	}
	if (geteuid() == 0)
	{
		std::cerr << "ERROR: Failed to drop root privileges\n";
		return false;
	}
	return true;
}

