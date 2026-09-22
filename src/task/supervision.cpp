#include "taskmaster.hpp"



void Taskmaster::supervision()
{
    while (true)
    {
        std::lock_guard<std::mutex> lock(programs_mutex);

        for (auto &pair : programs)
        {
            Program &prog = pair.second;

            for (auto &proc : prog.processes)
            {
                // STARTING → RUNNING
                if (proc.state == ProcessState::STARTING)
                {
                    time_t now = time(nullptr);

                    if (now - proc.start_timestamp >= prog.config.starttime)
                    {
                        proc.state = ProcessState::RUNNING;
                        log("Process " + prog.config.name + " is now RUNNING");
                    }
                }

                // Process non lancé
                if (proc.pid <= 0)
                    continue;

                // Check exit
                int status = 0;
                pid_t result = waitpid(proc.pid, &status, WNOHANG);

                if (result == 0)
                    continue;

                if (result == -1)
                    continue;

                // Process exited
                int exitcode = -1;
                if (WIFEXITED(status))
                    exitcode = WEXITSTATUS(status);

                proc.state = ProcessState::EXITED;
                proc.exitcode = exitcode;
                proc.pid = -1;

                handleAutorestart(prog, proc, exitcode);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
