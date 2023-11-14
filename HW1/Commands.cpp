#include "Commands.h" 

using namespace std;
int max_jobID = 0;
int g_last_pid = -1;
string last_command("");
SmallShell &smash = SmallShell::getInstance();
std::string WHITESPACE(" \t\f\v\n\r");

#if 0
#define FUNC_ENTRY()
cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif


/////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfContainAndStop(int pid) { 
    smash.jobs_list->removeFinishedJobs();
    JobsList::JobEntry *job = smash.jobs_list->getJobByPid(pid);

        if (!job){
            return false;
        }
        if (job->IS_STOPPED) {
            delete job;
            return true;
        }

    smash.jobs_list->convertToStopped(pid);

    delete job;
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////general helper/validation functions//////////////////////////////////////////////////////////////
int bgGetMaxJobID(vector<string> vec) {
    int max = 0;
        for (unsigned int i = 0; i < smash.jobs_list->all_jobs.size(); i++) {
            if ((smash.jobs_list->all_jobs[i].jobID > max) && (smash.jobs_list->all_jobs[i].IS_STOPPED))
                max = smash.jobs_list->all_jobs[i].jobID;
        }
    return max;
}

int fgGetMaxJobID(vector<string> vec) {
    int max = 0;
        for (unsigned int i = 0; i < smash.jobs_list->all_jobs.size(); i++) {
            if (smash.jobs_list->all_jobs[i].jobID > max)
                max = smash.jobs_list->all_jobs[i].jobID;
        }
    return max;
}


void stopLastJob() {smash.jobs_list->addJob(last_command, g_last_pid, true);}

int isPipe(vector<string> vec) {
    for (int i = 1; i < 19; ++i) {
        if ((vec[i] == "|") || (vec[i] == "|&")) {
            return i;
        }
    }
    return -1;
}

int isRedir(vector<string> vec) {
    
    for (int i = 0; i < 19; ++i) {
        if ((vec[i] == ">") || (vec[i] == ">>")) {
            return i;
        }
    }
    return -1;
}

int getToAddJobID() {
    int max = 1;
    int size = smash.jobs_list->all_jobs.size();
    JobsList *lst = smash.jobs_list;
    for (int i = 0; i < size; ++i) {
        if (lst->all_jobs[i].jobID >= max) {
            max = lst->all_jobs[i].jobID + 1;
        }
    }
    return max;
}

bool isSimple (vector<string>& vec) {
    for (string s : vec) {
        if (s.find('*') != string::npos || s.find('?') != string::npos)
            return false;
    }
    return true;
}

bool isExecutable (vector<string>& vec) {
    if ((vec[0][0] == '.' && vec[0][1] == '/') || !access(vec[0].c_str(), X_OK))
        return true;
    return false;
}

void simpleCommandRemoveBG (vector<string>& vec) {
    for (string& s : vec) {
        for (int i = 0; i < (int)s.size(); i++) {
            if (s[i] == '&') {
                s.erase(i,1);
            }
        }
    }
}

int getArgsNum (vector<string>& vec) {
    int args_num = 0;
    for (int i = 0; i < (int)vec.size(); i++) {
        if (vec[i] == "") return args_num;
        args_num++;
    }
    return args_num;
}

bool bdl_used_id(int job_id, vector<JobsList::JobEntry> vec) {
    for (unsigned int i = 0; i < vec.size(); i++) {
        if (vec[i].jobID == job_id){
            return true;
        }
    }
    return false;
}

////////commands for reading the entered line///////////////////////////////////////////////////////////////////////
string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
        for (std::string s; iss >> s;) {
            args[i] = (char *) malloc(s.length() + 1);
            memset(args[i], 0, s.length() + 1);
            strcpy(args[i], s.c_str());
            args[++i] = NULL;
        }
    return i;
    FUNC_EXIT()
}

vector<string> splitS(string s1) {
    vector<string> vec(20, "");
    char *args[20];
    int size = _parseCommandLine(s1.c_str(), args);
        for (int i = 0; i < size; ++i) {
            vec[i] = args[i];
        }
    return vec;
}

string firstWordRemoveUmprsnt(string s) {
    int len = s.length();
        if (len == 0){
            return "";
        }
        if (s[len - 1] == '&'){
            return s.substr(0, len - 1);
        }
    return s;
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    if (idx == string::npos) {
        return;
    }
    if (cmd_line[idx] != '&') {
        return;
    }
    cmd_line[idx] = ' ';
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool areSameCommands(const char *cmd1, const char *cmd2) {
    char x[strlen(cmd1) + 1];
    char y[strlen(cmd2) + 1];
    strcpy(x, cmd1);
    strcpy(y, cmd2);;
    _removeBackgroundSign(x);
    _removeBackgroundSign(y);
    vector<string> vec1 = splitS(x);
    vector<string> vec2 = splitS(y);
    for (int i = 0; i < 20; ++i) {
        if (vec1[i] != vec2[i])
            return false;
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////function of validation of input////////////////////////////////////////////////////////////////////////////
bool isValidArgs(vector<string> vec)
{
    if(vec.size()<3){
        cerr << "smash error: timeout: invalid arguments" << endl;
        return "";
    }
   
    try{
        int duration = stoi(vec[1]);
        if(duration < 0){
            cerr << "smash error: timeout: invalid arguments" << endl;
            return false;
        }
    }catch (...){
        cerr << "smash error: timeout: invalid arguments" << endl;
        return false;
    }
    return true;
}
bool isNumber(string str) {
  
  if ((str[0] == '-') && (str.length() > 1)){
         str = str.substr(1);
  }
        for (char const &c: str) {
            if (std::isdigit(c) == 0) return false;
        }

    return true;
}

bool isValidSigNumber(string str) {
    
    if (!isNumber(str)){
        return false;
    }

    if ((str[0] == '-') && (str.length() > 1)){
        str = str.substr(1);
    }

    int num = stoi(str);
        if ((num < 0) || (num > 64)){
            return false;
        }
    return true;
}

bool KillIsValid(vector<string> args){

      if ((args[1] == "") || (args[2] == "") || (args[3] != "")) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return false;
        }

    string first_arg = args[1];

        if ((first_arg[0] != '-') || (first_arg.length() == 1)) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return false;
        }

    first_arg = first_arg.substr(1);

        if ((!isNumber(first_arg)) || (!isNumber(args[2])) || (!isValidSigNumber(first_arg))) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return false;
        }
    
    return true;
}






bool cdIsValid(vector<string> vec) {
    if ((vec[2] != "")) {
            cerr << "smash error: cd: too many arguments" << endl;
            return false;
    }

        if (vec[1] == "-") {
            if (smash.jobs_list->last_dir == "") {
                cerr << "smash error: cd: OLDPWD not set" << endl;
                return false;
            }
        }
    return true;
}






///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////helper function of the class job////////////////////////////////////////////////////////////////////////////

void JobsList::updateJobTime(int jobID) {
  int size = all_jobs.size();

    for (int i = 0; i < size; ++i) {
            if ((all_jobs[i]).jobID == jobID) {
                (all_jobs[i]).time0 = time(nullptr);
                return;
            }
    }
}

void JobsList::addJob(string cmd_l, int pid, bool isStopped) {
    time_t t = time(nullptr);
    max_jobID = getToAddJobID();
    removeFinishedJobs();
///should the above lines be reversed????????????????????????????????????????
    JobEntry to_add(cmd_l, t, pid, max_jobID, isStopped);
    all_jobs.push_back(to_add);

        if (isStopped) {
            this->stopped_jobs.push_back(to_add);
        } else {
            this->jobs.push_back(to_add);
        }

}

void JobsList::printJobsList() {

    int size = all_jobs.size();
        for (int i = 0; i < size; ++i) {
            JobEntry j = all_jobs[i];
            cout << "[" << j.jobID << "] " << j.cmd_line << " : " << j.pid << " " << difftime(time(nullptr), j.time0)
                << " secs";
                if (j.IS_STOPPED) {
                    cout << " (stopped)";
                }
            cout << endl;
        }

}

void JobsList::printJobsListQuit() {

    int size = all_jobs.size();

        for (int i = 0; i < size; ++i) {
              JobEntry j = all_jobs[i];
              cout << j.pid << ": " << j.cmd_line;
              cout << endl;
        }

}

void JobsList::removeFinishedJobs() { 
    int all_size = all_jobs.size();
    int s_size = stopped_jobs.size();
    int u_size = jobs.size();
    vector<JobEntry> helper;

        for (int i = 0; i < all_size; ++i) {
            if (!(waitpid(all_jobs[i].pid, nullptr, WNOHANG))) {
                helper.push_back(all_jobs[i]);
            }
        }

    all_jobs = helper;
    vector<JobEntry> helper2;

        for (int i = 0; i < s_size; ++i) {
            if (!(waitpid(stopped_jobs[i].pid, nullptr, WNOHANG)))
                helper2.push_back(stopped_jobs[i]);
        }

    stopped_jobs = helper2;
    vector<JobEntry> helper3;
        for (int i = 0; i < u_size; ++i) {
            if (!(waitpid(jobs[i].pid, nullptr, WNOHANG)))
                helper3.push_back(jobs[i]);
        }

    jobs = helper3;

}

void JobsList::killAllJobs() {
    std::vector<JobEntry> empty;
    all_jobs = empty;
    jobs = empty;
    stopped_jobs = empty;
    max_jobID = 0;
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    int size = all_jobs.size();

        for (int i = 0; i < size; ++i) {
            if ((all_jobs[i]).jobID == jobId){
                 return new JobEntry(all_jobs[i]);
            }
               
        }

    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    int all_size = all_jobs.size();
    int u_size = jobs.size();
    int s_size = stopped_jobs.size();
    
        if (!bdl_used_id(jobId, all_jobs)){
            return;
        }

    vector<JobEntry> helper;
    vector<JobEntry> helper1;
    vector<JobEntry> helper2;

        for (int i = 0; i < all_size; ++i) {
            if (all_jobs[i].jobID != jobId){
                    helper.push_back(all_jobs[i]);
            }
        }

    all_jobs = helper;

        for (int i = 0; i < u_size; ++i) {
            if (jobs[i].jobID != jobId){
                   helper1.push_back(all_jobs[i]);
            }
        }

    jobs = helper1;

    for (int i = 0; i < s_size; ++i) {
        if (stopped_jobs[i].jobID != jobId){
                helper2.push_back(stopped_jobs[i]);
        }
    }

    stopped_jobs = helper2;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    int size = jobs.size();

        if (size >= 1){
            return new JobEntry(jobs[size - 1]);
        }

    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    int size = stopped_jobs.size();

        if (size >= 1){
            return new JobEntry(stopped_jobs[size - 1]);
        }

    return nullptr;
}

void JobsList::sendSignalToJobs(int SIG) {

    int s_size = stopped_jobs.size();
    int size = jobs.size();

        for (int i = 0; i < s_size; ++i) {
            if (kill(stopped_jobs[i].pid, SIG) == -1){
                perror("smash error: kill failed");
            }
        }

        for (int i = 0; i < size; ++i) {
            if (kill(jobs[i].pid, SIG) == -1){
                perror("smash error: kill failed");
            }
        }
}

void JobsList::convertToUnstopped(int jobID) {
    int size = all_jobs.size();

        for (int i = 0; i < size; ++i) {
            if (all_jobs[i].jobID == jobID) {
//shouldnt the order be reversed???????//
                all_jobs[i].IS_STOPPED = false;

                    if (kill(all_jobs[i].pid, SIGCONT) == -1) {
                        perror("smash error: kill failed");
                        return;
                    }
                    
                break;
            }
        }

    size = stopped_jobs.size();
    vector<JobEntry> helper;

        for (int i = 0; i < size; ++i) {
                if (stopped_jobs[i].jobID == jobID) {
                        JobEntry to_change = stopped_jobs[i];
                        to_change.IS_STOPPED = false;
                        jobs.push_back(to_change);
                } else {
                       helper.push_back(stopped_jobs[i]);
                }
        }
    stopped_jobs = helper;
}

void JobsList::listUndo() {
    int size = all_jobs.size();

        if (all_jobs[size - 1].IS_STOPPED) {
            stopped_jobs.pop_back();
        } else {
            jobs.pop_back();
        }

    all_jobs.pop_back();
}

void JobsList::convertToStopped(int pid) {
    int size = all_jobs.size();

        for (int i = 0; i < size; ++i) {
                if (all_jobs[i].pid == pid) {

                    if (kill(all_jobs[i].pid, SIGTSTP) == -1) {
                        perror("smash error: kill failed");
                        return;
                    }

                    all_jobs[i].IS_STOPPED = true;
                    break;
                }
        }

    size = jobs.size();
    vector<JobEntry> helper;

        for (int i = 0; i < size; ++i) {
            if (jobs[i].pid == pid) {
                    JobEntry to_change = jobs[i];
                    to_change.IS_STOPPED = true;
                    stopped_jobs.push_back(to_change);
            } else {
                    helper.push_back(jobs[i]);
            }
        }

    jobs = helper;
    cout << "smash: process " << pid << " was stopped" << endl;
}

JobsList::JobEntry *JobsList::getJobByPid(int pid) {
    int size = all_jobs.size();

        for (int i = 0; i < size; ++i) {
            if ((all_jobs[i]).pid == pid)
                return new JobEntry(all_jobs[i]);
        }

    return nullptr;
}

bool jobIsStopped(int pid) {
    JobsList::JobEntry *job = smash.jobs_list->getJobByPid(pid);
       
        if (!job){
            return false;
        }

        if (job->IS_STOPPED) {
            delete job;
            return true;
        }

    delete job;
    return false;
}

void updateLastPid(int pid) {
    g_last_pid = pid;
}

int getLastPid() {
    return g_last_pid;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

string removeRedir(string s, int*x) {
    int len = s.length();
        if (len == 0){
            return "";
        }
        if(s[len - 2] == '>' && s[len - 1] == '>'){
            *x=2;
            return s.substr(0, len - 2);
        }
        if (s[len - 1] == '>'){
            *x=1;
            return s.substr(0, len - 1);
        }

    return s;
}

vector<string> splitred(string s1) {
    vector<string> vec(20,"");
    vec= splitS(s1);
    vector<string> vec1(24,"");
    int i=0,j=0;
    bool isContenoius =false , specialCase =false;
        while(vec[j]!=""){
            if(vec[j].find('>')!=string::npos && (vec[j].find(">>")!=vec[j].find('>'))){
                vec1[i] = vec[j].substr(0,vec[j].find('>')-1);
                vec1[i+1] =">";
                specialCase = true;
                    if(vec[j].find('>')!= vec[j].size()-1){
                        isContenoius =true;
                        vec1[i+2] = vec[j].substr(vec[j].find('>')+1,vec[j].size());
                    }

                    if(isContenoius){
                        i=+2;
                    }else{
                        i++;
                    }

                isContenoius = false;
            }

            if(vec[j].find(">>")!=string::npos && (vec[j].find(">>")!=vec[j].find('>'))){
                vec1[i] = vec[j].substr(0,vec[j].find('>')-1);
                vec1[i+1] =">>";
                specialCase = true;

                    if(vec[j].find(">>")!= vec[j].size()-1){
                        isContenoius =true;
                        vec1[i+2] = vec[j].substr(vec[j].find('>')+1,vec[j].size());
                    }

                    if(isContenoius){
                        i=+2;
                    }else{
                        i++;
                    }

                isContenoius = false;

            }

            if(!specialCase){
                vec1[i] =vec[j];
                specialCase = false;
            }

            ++i;
            ++j;
        }    
    return vec1;
}

Command *SmallShell::CreateCommand(const char *cmd_line) {
       string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    firstWord = firstWordRemoveUmprsnt(firstWord);
    jobs_list->removeFinishedJobs();
    vector<string> vec = splitS(cmd_line);
    g_last_pid = -1;
    int res = isPipe(vec);
    if (res != -1)
        return new PipeCommand(cmd_line, res);
    
    if (cmd_s.find('>') != string::npos){
        vec = splitred(cmd_line);
        res = isRedir(vec);
        return new RedirectionCommand(cmd_line, res);
    }
        
    if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, nullptr);
    } else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, nullptr);
    } else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, nullptr);
    } else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, nullptr);
    } else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, nullptr);
    } else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, nullptr);
    }else if (firstWord.compare("chmod") == 0) {
        return new ChmodCommand(cmd_line);
    }else if (firstWord.compare("getfiletype") == 0) {
        return new GetFileTypeCommand(cmd_line);
    }else if (firstWord.compare("setcore") == 0) {
        return new SetcoreCommand(cmd_line);
    }
    return new ExternalCommand(cmd_line);


}


void SmallShell::executeCommand(const char *cmd_line) {
    Command *cmd = CreateCommand(cmd_line);
    if (!cmd){
        return;
    }
    cmd->execute();
    delete cmd;
}

//////////////////////////////// built in commands ///////////////////////////////////////////////////////////////////////
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ChangeDirCommand::ChangeDirCommand(const char *cmd_line,char **plastPwd) : BuiltInCommand(cmd_line),plastPwd(plastPwd){}


void ShowPidCommand::execute() {
    cout << "smash pid is " << smash.pid << endl;
}

void GetCurrDirCommand::execute() {
    char buf[80];
    getcwd(buf, 80);
    cout << string(buf) << endl;
}

void ChangeDirCommand::execute() {
    JobsList *joblst = smash.jobs_list;
    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    vector<string> vec = splitS(string(comm));

        if (!cdIsValid(vec)){
            return;
        }

        if (joblst->current_dir == "") {
            char buf[80];
            getcwd(buf, 80);
            joblst->current_dir = buf;
        }

        if (vec[1] == "-") {
            if (chdir(joblst->last_dir.c_str()) >= 0) {
                std::string temp = joblst->last_dir;
                joblst->last_dir = joblst->current_dir;
                joblst->current_dir = temp;
            } else {
                perror("smash error: chdir failed"); 
            }
        } else {
                if (chdir((vec[1]).c_str()) < 0) {
                    perror("smash error: chdir failed"); 
                    return;
                }

            char buf[80];
            getcwd(buf, 80);
            joblst->last_dir = joblst->current_dir;
            joblst->current_dir = buf;
        }

}


void JobsCommand::execute() {
    smash.jobs_list->printJobsList();
}

void ForegroundCommand::execute() {

    char comm[strlen(cmd_line + 1)];
    strcpy(comm, cmd_line);
    _removeBackgroundSign(comm);
    smash.jobs_list->removeFinishedJobs();
    vector<string> vec = splitS(comm);// fg job-id invalid arguments

            if(vec[2] != ""){
                 cerr << "smash error: fg: invalid arguments" << endl;
                 return;
            }

    int jobId = 0;
            if(vec[1]== ""){
                jobId = fgGetMaxJobID(vec);
                    if(jobId == 0){
                        cerr << "smash error: fg: jobs list is empty" << endl;
                        return;
                    }
            }

            if(!isNumber(vec[1])){
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
            }

            if(vec[1]!= ""){
             jobId = atoi(vec[1].c_str());
            }

   

    JobsList::JobEntry *job = smash.jobs_list->getJobById(jobId);
        if (!job) {
            cerr << "smash error: fg: job-id " << jobId << " does not exist" << endl;
            return;
        }

   
        if (job->IS_STOPPED) {
            smash.jobs_list->convertToUnstopped(jobId);//reversed(for crtl+f),should the max id be updated?
            if (kill(job->pid, SIGCONT) == -1) {
                perror("smash error: kill failed");
                return;
            }
        }

    g_last_pid = job->pid;
    last_command = job->cmd_line;
    cout << job->cmd_line << " : " << job->pid << endl;
    waitpid(job->pid, nullptr, WUNTRACED);
    delete job;
}


void BackgroundCommand::execute() {

    smash.jobs_list->removeFinishedJobs();
    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    vector<string> vec = splitS(comm);

        if(vec[2] != ""){
                 cerr << "smash error: bg: invalid arguments" << endl;
                 return;
            }

    int jobId = 0;

        if(vec[1]== ""){
            jobId = bgGetMaxJobID(vec);
                if(jobId == 0){
                    cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
                    return;
                }
        }

        if(!isNumber(vec[1])){
            cerr << "smash error: bg: invalid arguments" << endl;
            return;
        }

        if(vec[1]!= ""){
            jobId = atoi(vec[1].c_str());
        }

    JobsList::JobEntry *job = smash.jobs_list->getJobById(jobId);
        if (!job) {
            cerr << "smash error: bg: job-id " << jobId << " does not exist" << endl;
            return;
        }
        if (!job->IS_STOPPED) {
            cerr << "smash error: bg: job-id " << jobId << " is already running in the background" << endl;
            return;
        }
        if (kill(job->pid, SIGCONT) == -1) {
            perror("smash error: kill failed");
            return;
        }

    cout << job->cmd_line << " : " << job->pid << endl;
    smash.jobs_list->convertToUnstopped(jobId);
    delete job;
}

void QuitCommand::execute() {

    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    smash.jobs_list->removeFinishedJobs();
    vector<string> vec = splitS(comm);


        if (vec[1] == "kill") {
            smash.jobs_list->sendSignalToJobs(SIGKILL);
            int jobs_num = smash.jobs_list->stopped_jobs.size() + smash.jobs_list->jobs.size();
            cout << "smash: sending SIGKILL signal to " << jobs_num << " jobs:" << endl;
            smash.jobs_list->printJobsListQuit();
        }
}


void KillCommand::execute() {

    vector<string> args = splitS(cmd_line);

            if(!KillIsValid(args)){
                return;//printing is done in the function
            }
    
    int jobid = atoi((args[2]).c_str());

        if (!bdl_used_id(jobid, smash.jobs_list->all_jobs)) {
            cerr << "smash error: kill: job-id " << jobid << " does not exist" << endl;
            return;
        }

    string first_arg = args[1];
    first_arg = first_arg.substr(1);
    int sig = atoi(first_arg.c_str());

    JobsList::JobEntry *job = smash.jobs_list->getJobById(jobid);
    int pid = job->pid;
    string helper = cmd_line;//?? what does this line do????
    delete job;

        if (kill(pid, sig) == -1) {
            perror("smash error: kill failed");
            return;
        } else {
            cout << "signal number " << sig << " was sent to pid " << pid << endl;
        }
}

//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////bonus part/////////////////////////////////////////////////////////
void TimeoutList::addTimeoutCommand(int pid, time_t duration, string cmd_line) {
    time_t now;

    if (time(&now) == ((time_t) - 1)) {
        perror("smash error: time failed");
        return;
    }

    int closest_alarm = alarm(0);
        if (closest_alarm == 0 || duration < closest_alarm){
            alarm(duration);
        }else{
            alarm(closest_alarm);
        }

    shared_ptr<TimeoutEntry> new_entry = shared_ptr<TimeoutEntry>(new TimeoutEntry(pid, now, duration, cmd_line));
    commands.push_back(new_entry);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ExternalCommand::execute() {

    vector<string> vec = splitS(cmd_line);
    bool umprsnt_flag = _isBackgroundComamnd(cmd_line);

    bool is_timeout = false;
    string timeout_duration, command_to_timeout;

        if(vec[0] == "timeout"){
                if(!isValidArgs(vec)){
                    return;
                }
            is_timeout = true;
            timeout_duration = vec[1];
            command_to_timeout = vec[2];
            vec.erase(vec.begin());
            vec.erase(vec.begin());          
        }

    int pid = fork();

        if (pid == -1) {
            perror("smash error: fork failed");
            return;
        }

        if (pid == 0) {
            setpgrp();
            char arg3[strlen(cmd_line) + 1];
            strcpy(arg3, cmd_line);
            _removeBackgroundSign(arg3);


      
        if (isSimple(vec)) {
            simpleCommandRemoveBG(vec);
            int args_num = getArgsNum(vec);
            char* args[20] = {NULL};
            if (!isExecutable(vec)) {
                vec[0] = "/bin/" + vec[0];
            }
            for (int i = 0; i < args_num; i++) {
                if (vec[i] == "") break;
                args[i] = (char*)vec[i].c_str();
            }

            if (execv( args[0] , args) == -1) {
                perror("smash error:  execv failed");
                exit(0);
            }
            }else{
                char *args[] = {(char *) "/bin/bash", (char *) "-c", arg3, nullptr};
                if (execv((char *) "/bin/bash", args) == -1){
                    perror("smash error: execv failed");
                    exit(0);
                }
            }
    }else{
         if(is_timeout){
            smash.timeouts->addTimeoutCommand(pid , (stoi(timeout_duration)) , cmd_line);// full command without timeout needs to be sent as parameter command_to_timeout
         }
        if (umprsnt_flag){
            smash.jobs_list->addJob(cmd_line, pid);
        }
        if (!umprsnt_flag) {
            last_command = cmd_line;
            g_last_pid = pid;
            waitpid(pid, nullptr, WUNTRACED);
        }

    }
}

int buffSize(char *buff) {
    for (int i = 0; i < 200; ++i) {
        if (buff[i] == '\0')
            return i + 1;
    }
    return 0; 
}

string makeSecSpecialComand(const char *cmd_line, int i) {
    vector<string> vec = splitS(cmd_line);
    string to_ret = "";
    i++;

    for (; ((i < 20) && (vec[i] != "")); ++i) {
        to_ret += vec[i];
        to_ret += " ";
    }
    int len = to_ret.length();
    if (len > 0)
        to_ret = to_ret.substr(0, len - 1);
    return to_ret;
}

int redirectionFirstOf(string sentence, string find) {
    int len = sentence.length();
    for (int i = 0; i < len; ++i) {
        if (sentence[i] == '>') {
            if (find == ">") {
                return i;
            } else if (sentence[i + 1] == '>') {
                return i + 1;
            }
        }
    }
    return -1;
}

string makeSecFile(const char *cmd_line, int index) {
    string to_ret = string(cmd_line);
    vector<string> vec = splitred(cmd_line);
    int j;
    if (vec[index] == ">"){
        j = redirectionFirstOf(to_ret, ">");
        
    }else
        j = redirectionFirstOf(to_ret, ">>");
    j++;

    to_ret = to_ret.substr(j);
    to_ret = _trim(to_ret);
    return to_ret;
}

string makeFirstSpecialCommand(const char *cmd_line, int index) {
    string s = "";
    if (!index)
        return " ";
    vector<string> vec = splitred(cmd_line);
    for (int i = 0; i < index; i++) {
        s += vec[i];
        s += " ";
    }
    s = s.substr(0, s.length() - 1);
    return s;
}

void PipeCommand::execute() {
    vector<string> vec = splitS(cmd_line);
    int my_pipe[2];
    pipe(my_pipe);
    int pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    int os = 1;
    if (pid == 0) {
        if (vec[index] == "|&")
            os = 2;
        setpgrp();
        close(my_pipe[0]);
        int helper = dup(os);
        dup2(my_pipe[1], os);
        if (index) {
            string helper = makeFirstSpecialCommand(cmd_line, index);
            char comm[helper.length() + 1];
            strcpy(comm, helper.c_str());
            smash.executeCommand(comm);
        } else { smash.executeCommand(""); }
        dup2(os, helper);
        close(helper);
        close(my_pipe[1]);
        exit(0);
    } else { // father
        int pid1 = fork();
        if (pid1 == -1) {
            perror("smash error: fork failed");
            return;
        }
        if (pid1 == 0) {
            setpgrp();
            close(my_pipe[1]);
            int helper1 = dup(0);
            dup2(my_pipe[0], 0);
            string s = makeSecSpecialComand(cmd_line, index);
            smash.executeCommand(s.c_str());
            dup2(0, helper1);
            close(my_pipe[0]);
            close(helper1);
            exit(0);
        } else {
            close(my_pipe[0]);
            close(my_pipe[1]);

            waitpid(pid, nullptr, WUNTRACED);
            waitpid(pid1, nullptr, WUNTRACED);
            kill(pid,SIGKILL);
            kill(pid1,SIGKILL);
        }
    }

}


void RedirectionCommand::execute() {
    vector<std::string> vec = splitred(cmd_line);
    string helper = makeSecFile(cmd_line, index);
    char file_name[helper.length() + 1];
    strcpy(file_name, helper.c_str());

    int to_change;
    if (vec[index] == ">") {
        to_change = open(helper.c_str() , O_WRONLY | O_CREAT | O_TRUNC , 0655);
        if (to_change == -1) {
            perror("smash error: open failed");
            return;
        }
    } else {
        to_change = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0655);
        if (to_change == -1) {
            perror("smash error: open failed");
            return;
        }
    }
    if (index) {
        int helper = dup(1);
        dup2(to_change, 1);
        string s = makeFirstSpecialCommand(cmd_line, index);
        char comm[s.length() + 2];
        strcpy(comm, s.c_str());
        smash.executeCommand(comm);
        dup2(helper, 1);
        close(helper);
    }
    close(to_change);
}




void SetcoreCommand::execute(){

    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    vector<string> vec = splitS(string(comm));

        if(vec.size()<3){
            cerr << "smash error: setcore: invalid arguments" << endl;
        }   
            try{
                smash.jobs_list->getJobById(stoi(vec[1]));
            }
            catch(const std::exception& e){
                cerr << "smash error: setcore: invalid arguments" << endl;
                return;
            }
         JobsList::JobEntry* jobPtr  = smash.jobs_list->getJobById(stoi(vec[1]));
                if(jobPtr == nullptr){
                    cerr << " setcore: job-id " + vec[1] + "does not exist" << endl;
                }
    
            try{
               stoi(vec[2]);
            }
            catch(const std::exception& e){
                cerr << "smash error: setcore: invalid arguments" << endl;
                return;
            }

        int coreNum =stoi(vec[2]);
        int jobPid =  smash.jobs_list->getJobById(stoi(vec[1]))->pid;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(coreNum, &cpuset);
        int ret = sched_setaffinity(jobPid, sizeof(cpu_set_t), &cpuset);
        if(ret==-1){
            cerr << "smash error: sched_setaffinity failed" << endl;
        }

}


void GetFileTypeCommand::execute(){
    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    vector<string> vec = splitS(string(comm));

         if(vec.size()<2){
            cerr << "smash error: gettype: invalid arguments" << endl;
        }   

      char* filePath = new char[vec[1].size()];
      strcpy(filePath, vec[1].c_str());
        struct stat file_stat;
            if (stat(filePath, &file_stat) == -1) {
                cerr << "smash error: gettype: invalid arguments" << endl;
                 return;
            }
        string fileType;
            if (S_ISREG(file_stat.st_mode)) {
                fileType = "regular file";
            } else if (S_ISDIR(file_stat.st_mode)) {
                fileType = "directory";
            } else if (S_ISCHR(file_stat.st_mode)) {
                fileType = "character device";
            } else if (S_ISBLK(file_stat.st_mode)) {
                fileType = "block device";
            } else if (S_ISFIFO(file_stat.st_mode)) {
                fileType = "FIFO/pipe";
            } else if (S_ISLNK(file_stat.st_mode)) {
                fileType = "symbolic link";
            } else if (S_ISSOCK(file_stat.st_mode)) {
               fileType = "socket";
            } else {
                 cerr << "smash error: gettype: invalid arguments" << endl;
                 return;
            }
            cout << vec[1] + " type is " + fileType + " and takes up " + to_string(file_stat.st_size) +" bytes "<< endl;

}


void ChmodCommand::execute(){
    
    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    vector<string> vec = splitS(string(comm));

         if(vec.size()<3){
            cerr << "smash error: gettype: invalid arguments" << endl;
        }   
            try{
               stoi(vec[1]);
            }
            catch(const std::exception& e){
                cerr << "smash error: chmod: invalid arguments" << endl;
                return;
            }

      char* filePath = new char[vec[2].size()];
      strcpy(filePath, vec[2].c_str());
    int result = chmod(filePath, stoi(vec[1]));
        if (result != 0) {
             cerr << "smash error: chmod: invalid arguments" << endl;
        } 

}



 