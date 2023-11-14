#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <iomanip>
#include <utime.h>
#include <cstdlib>
#include <sched.h>
#include <sys/stat.h>
#include <cstring>
#include <memory>
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
using std::shared_ptr;
using namespace std;
#define DO_SYS( syscall, name ) do { \
    if( (syscall) == -1 ) { \
      perror("smash error: " #name " failed"); \
      return; \
    }         \
  } while(0)  \



std::vector<std::string> splitS(std::string cmd_line);

void stopLastJob();

void updateLastPid(int pid);
bool jobIsStopped(int pid);
int getLastPid();
bool checkIfContainAndStop(int pid);

class Command {
public:
    const char *cmd_line;

    Command(const char *cmd_line) : cmd_line(cmd_line) {}

    virtual ~Command() = default;

    virtual void execute() = 0;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

    virtual ~ExternalCommand() = default;

    void execute() override;
};

class PipeCommand : public Command {
    int index;
public:
    PipeCommand(const char *cmd_line, int index) : Command(cmd_line), index(index) {}

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    int index;
public:
    explicit RedirectionCommand(const char *cmd_line, int index) : Command(cmd_line), index(index) {}

    virtual ~RedirectionCommand() {}

    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
    char **plastPwd;
public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;


class QuitCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
    public:
        std::string cmd_line;
        time_t time0;
        int pid;
        int jobID;
        bool IS_STOPPED;
    public:
        JobEntry(const std::string cmd_line, time_t start_time, int pid, int jodid, bool stopped) : cmd_line(cmd_line),
                                                                                                    time0(start_time),
                                                                                                    pid(pid),
                                                                                                    jobID(jodid),
                                                                                                    IS_STOPPED(
                                                                                                            stopped) {}

    };

    std::vector<JobEntry> stopped_jobs;
    std::vector<JobEntry> jobs;
    std::vector<JobEntry> all_jobs;
    std::string last_dir;
    std::string current_dir;
public:
    JobsList() : stopped_jobs(), jobs(), all_jobs(), last_dir(""), current_dir("") {}

    ~JobsList() = default;

    void updateJobTime(int jobID);

    void addJob(std::string cmd_l, int pid, bool isStopped = false);

    void printJobsList();

    void printJobsListQuit();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    JobEntry *getJobByPid(int pid);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    void sendSignalToJobs(int SIG);

    void convertToUnstopped(int jobID);

    void convertToStopped(int jobID);

    void listUndo();
};

class JobsCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~ForegroundCommand() = default;

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~BackgroundCommand() = default;

    void execute() override;
};



class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line): BuiltInCommand(cmd_line){};;
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
 public:
  GetFileTypeCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
 public:
  SetcoreCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
  virtual ~SetcoreCommand() {}
  void execute() override;
};

////////////////////////////bonus part////////////////////////////
class TimeoutList {
public:
    class TimeoutEntry {
    public:
        int pid;
        time_t time_added;
        time_t duration;
        std::string cmd_line;
        TimeoutEntry(int pid, time_t time_added, time_t duration, std::string cmd_line) : pid(pid), time_added(time_added),
        duration(duration), cmd_line(cmd_line){}
        ~TimeoutEntry() = default;
    };
    vector<shared_ptr<TimeoutEntry>> commands;
    TimeoutList() = default;
    ~TimeoutList() = default;
    void addTimeoutCommand(int pid, time_t duration, string cmd_line);
};

////////////////////////////end of bonus part////////////////////////////





class SmallShell {
private:
    SmallShell() {
        jobs_list = new JobsList();
        timeouts = new TimeoutList();
    }

public:
    JobsList *jobs_list;
	int pid;
    TimeoutList* timeouts ;
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; 
    void operator=(SmallShell const &) = delete; 
    static SmallShell &getInstance() {
        static SmallShell instance; 
        return instance;
    }

   

     ~SmallShell(){
        delete  jobs_list;
        delete timeouts;
     }

    void executeCommand(const char *cmd_line);
};

#endif //SMASH_COMMAND_H_



