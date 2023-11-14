#include "signals.h"
using namespace std;

void ctrlZHandler(int sig_num) {
    int last_pid = getLastPid();
//    cout<<endl<<endl<<last_pid<<endl<<endl;
    cout << "smash: got ctrl-Z" << endl;
    if (checkIfContainAndStop(last_pid)) {
        return;
    }
    if (last_pid != -1) {
        if (waitpid(last_pid, nullptr, WNOHANG) == 0) {
            stopLastJob();
            updateLastPid(-1);

            if (kill(last_pid, SIGTSTP) == -1) {
                perror("smash error: kill failed");
                return;
            }
            cout << "smash: process " << last_pid << " was stopped" << endl;
        }
    }

}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    int last_pid = getLastPid();
    if (last_pid != -1) {
        if (jobIsStopped(last_pid))
            return;
        if (waitpid(last_pid, nullptr, WNOHANG) == 0) {
            updateLastPid(-1);
            if (kill(last_pid, SIGKILL) == -1) {
                perror("smash error: kill failed");
                return;
            }
            cout << "smash: process " << last_pid << " was killed" << endl;
        }
    }
}

void alarmHandler(int sig_num) {
    bool flag = true;
    SmallShell& smash = SmallShell::getInstance();
    while(flag)
    {
        cout << "smash: got an alarm" << endl;
        smash.jobs_list->removeFinishedJobs();
        int seconds_to_alarm = 0;
        TimeoutList::TimeoutEntry* next_alarm = nullptr;
        // search for alarmed command
        for(int i = 0 ; i < (int)(smash.timeouts->commands.size()) ; ++i)
        {
            if (!next_alarm)
            {
                next_alarm = smash.timeouts->commands[i].get();
                seconds_to_alarm = next_alarm->duration + next_alarm->time_added;
                continue;
            }
            if(smash.timeouts->commands[i]->duration + smash.timeouts->commands[i]->time_added < seconds_to_alarm)
            {
                next_alarm = smash.timeouts->commands[i].get();
                seconds_to_alarm = next_alarm->duration + next_alarm->time_added;
            }
        }
        int pid = next_alarm->pid; //Segmentation
        int sys_result = waitpid(pid , NULL , WNOHANG);
        if(sys_result == 0)
        {
            DO_SYS(kill(pid , SIGKILL) , kill);
            cout << "smash: " << next_alarm->cmd_line << " timed out!" << endl;
        }
        // delete alarmed command
        for (int i = 0; i < (int)(smash.timeouts->commands.size()); ++i)
        {
            if (smash.timeouts->commands[i]->pid == next_alarm->pid) {
                smash.timeouts->commands.erase(smash.timeouts->commands.begin() + i);
            }
        }

        if (!smash.timeouts->commands.empty()) {
            next_alarm = nullptr;
            seconds_to_alarm = 0;
            // search for alarmed command
            for(int i = 0 ; i < (int)(smash.timeouts->commands.size()) ; ++i)
            {
                if (!next_alarm)
                {
                    next_alarm = smash.timeouts->commands[i].get();
                    seconds_to_alarm = next_alarm->duration + next_alarm->time_added;
                    continue;
                }

                if(smash.timeouts->commands[i]->duration + smash.timeouts->commands[i]->time_added < seconds_to_alarm)
                {
                    next_alarm = smash.timeouts->commands[i].get();
                    seconds_to_alarm = next_alarm->duration + next_alarm->time_added;
                }
            }
            time_t now;
            if (time(&now) == ((time_t) -1))
            {
                perror("smash error: time failed");
                return;
            }
            seconds_to_alarm -= now;
        }
        alarm(seconds_to_alarm);
        flag = seconds_to_alarm == 0;
    }
}

