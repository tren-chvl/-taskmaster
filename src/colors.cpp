#include "taskmaster.hpp"



std::string Taskmaster::colorState(ProcessState st)
{
	switch (st)
	{
		case ProcessState::RUNNING:
			return ANSI_GREEN + std::string("RUNNING") + ANSI_RESET;

		case ProcessState::STARTING:
			return ANSI_BLUE + std::string("STARTING") + ANSI_RESET;

		case ProcessState::BACKOFF:
			return ANSI_YELLOW + std::string("BACKOFF") + ANSI_RESET;

		case ProcessState::FATAL:
			return ANSI_RED + std::string("FATAL") + ANSI_RESET;

		case ProcessState::EXITED:
			return ANSI_RED + std::string("EXITED") + ANSI_RESET;

		case ProcessState::STOPPED:
			return ANSI_YELLOW + std::string("STOPPED") + ANSI_RESET;

		default:
			return ANSI_YELLOW + std::string("UNKNOWN") + ANSI_RESET;
	}
}
