#include "taskmaster.hpp"

std::shared_ptr<PtySession> Taskmaster::getPtySession(const std::string &name)
{
	std::lock_guard<std::mutex> lock(programs_mutex);
	auto it = programs.find(name);
	if (it == programs.end())
		return nullptr;
	for (auto &proc : it->second.processes)
	{
		if (proc.pid > 0 && proc.pty)
			return proc.pty;
	}
	return nullptr;
}


void Taskmaster::pumpPty(std::shared_ptr<PtySession> session)
{
    char buffer[4096];

    while (true)
    {
        ssize_t n = read(session->master_fd, buffer, sizeof(buffer));

        if (n > 0)
        {
            std::lock_guard<std::mutex> lock(session->mutex);

            // Write PTY output to stdout log.
            if (session->stdout_log_fd >= 0)
            {
                size_t total = 0;

                while (total < static_cast<size_t>(n))
                {
                    ssize_t written = write(
                        session->stdout_log_fd,
                        buffer + total,
                        static_cast<size_t>(n) - total);

                    if (written < 0 && errno == EINTR)
                        continue;

                    if (written <= 0)
                        break;

                    total += static_cast<size_t>(written);
                }
            }

            // Forward output to attached client.
            if (session->attached_fd >= 0)
            {
                size_t total = 0;

                while (total < static_cast<size_t>(n))
                {
                    ssize_t written = write(
                        session->attached_fd,
                        buffer + total,
                        static_cast<size_t>(n) - total);

                    if (written < 0 && errno == EINTR)
                        continue;

                    if (written <= 0)
                        break;

                    total += static_cast<size_t>(written);
                }
            }

            continue;
        }

        if (n < 0 && errno == EINTR)
            continue;

        break;
    }

    std::lock_guard<std::mutex> lock(session->mutex);

    if (session->stdout_log_fd >= 0)
    {
        close(session->stdout_log_fd);
        session->stdout_log_fd = -1;
    }

    if (session->master_fd >= 0)
    {
        close(session->master_fd);
        session->master_fd = -1;
    }
}
