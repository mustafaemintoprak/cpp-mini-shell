#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstdlib>
#include <signal.h>

void handle_sigchld(int) {
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
    }
}

int main()
{
    struct sigaction s;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if(sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    while (true) {

        std::string line;
        std::cout << "shell> ";

        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        if (line.empty()) {
            continue;
        }

        // Tokenization
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string word;

        while (iss >> word) {
            tokens.push_back(word);
        }

        bool background = false;

        if (!tokens.empty() && tokens.back() == "&") {
            background = true;
            tokens.pop_back();
        }

        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "exit") {
            break;
        }

        else if (tokens[0] == "cd") {

            if (tokens.size() > 2) {
                std::cerr << "cd: too many arguments\n";
                continue;
            }

            if (tokens.size() == 1) {
                const char* home = getenv("HOME");
                if (!home || chdir(home) == -1) {
                    perror("cd");
                }
            } else {
                if (chdir(tokens[1].c_str()) == -1) {
                    perror("cd");
                }
            }

            continue;
        }

        else if (tokens[0] == "pwd") {

            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                std::cout << cwd << std::endl;
            } else {
                perror("pwd");
            }

            continue;
        }

        else {
            std::vector<char*> args;
            for (auto& token : tokens) {
                args.push_back(const_cast<char*>(token.c_str()));
            }
            args.push_back(nullptr);

            pid_t pid = fork();

            if (pid < 0) {
                perror("fork");
                continue;
            }
            else if (pid == 0) {
                execvp(args[0], args.data());
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            else {
                if (!background) {
                    waitpid(pid, nullptr, 0);
                } else {
                    std::cout << "[Background PID: " << pid << "]\n";
                }
            }
        }
    }

    return 0;
}
