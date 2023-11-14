#include "signals.h"


int main(int argc, char *argv[]) {
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    

    struct sigaction sa;
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("smash error: failed to set alarm handler");
    }


    SmallShell &smash = SmallShell::getInstance();
    bool flag = false;
    smash.pid = getpid();
    std::string first_word = "";

    while (true) {
        if (!flag)
            std::cout << "smash> ";
        else
            std::cout << first_word << "> ";

        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        std::vector<std::string> words = splitS(cmd_line);

            if (words[0] == "chprompt") 
            {
                if (words[1] == "")
                    flag = false;
                else {
                    flag = true;
                    first_word = words[1];
                }
            } 
            else 
            {
                smash.executeCommand(cmd_line.c_str());
            }
            
            if (words[0] == "quit")
                break;
    }
    return 0;
}

