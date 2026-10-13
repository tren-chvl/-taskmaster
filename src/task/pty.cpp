#include "taskmaster.hpp"


void Taskmaster::pumpPty(std::shared_ptr<PtySession> session)
{
    char buffer[4096];

    while (true)
    {
        ssize_t n = read(session->master_fd, buffer, sizeof(buffer));

        if (n > 0)
        {
            std::lock_guard<std::mutex> lock(session->mutex);

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
}
