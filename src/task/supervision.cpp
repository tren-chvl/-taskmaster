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
                // -----------------------------
                // 1. Passage STARTING → RUNNING
                // -----------------------------
                if (proc.state == ProcessState::STARTING)
                {
                    time_t now = time(nullptr);
                    if (now - proc.start_timestamp >= prog.config.starttime)
                    {
                        proc.state = ProcessState::RUNNING;
                        log("Process " + prog.config.name + " is now RUNNING");
                    }
                }

                // -----------------------------
                // 2. Vérifier si le process est mort
                // -----------------------------
                if (proc.pid > 0)
                {
                    int status = 0;
                    pid_t result = waitpid(proc.pid, &status, WNOHANG);

                    if (result == proc.pid)
                    {
                        int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

                        proc.state = ProcessState::EXITED;
                        proc.exitcode = exitcode;
                        proc.pid = -1;

                        log("Process " + prog.config.name +
                            " exited with code " + std::to_string(exitcode));

                        // -----------------------------
                        // 3. EXIT NORMAL → PAS DE RESTART
                        // -----------------------------
                        bool expected = false;
                        for (int code : prog.config.exitcodes)
                        {
                            if (code == exitcode)
                            {
                                expected = true;
                                break;
                            }
                        }

                        if (expected)
                        {
                            log("Exit code is expected → no autorestart");
                            continue; // ne pas relancer
                        }

                        // -----------------------------
                        // 4. Crash → autorestart
                        // -----------------------------
                        handleAutorestart(prog, proc, exitcode);
                        continue;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
