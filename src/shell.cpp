#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstdlib>

int main()
{
    while (true) {
        std::string line;

        std::cout << "shell> ";

        // EOF Handling (Ctrl + D)
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        // Ignore empty input
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

        if (tokens[0] == "exit") {
            break;
        }
        else if (tokens[0] == "cd") {
            // handle chdir
            if (tokens.size() > 2) 
                std::cerr << "cd: too many arguments\n",
                continue;
            
            else if ( tokens.size() == 1) {
                const char* home = getenv("HOME");
                if (home == nullptr || chdir(home) == -1){
                    perror("cd");
                }
            }
            else {
                // "cd someDir"
                if (chdir(tokens[1].c_str()) == -1){
                    perror("cd");
                }
            }
            continue;
        }
        else if (tokens[0] == "pwd"){
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                std::cout << cwd << std::endl;
            }
            else {
                perror("pwd");
            }
            continue;
        }
        else {
            // Convert to char* array for exec
            std::vector<char*> args;
            for (auto& token : tokens) {
                args.push_back(const_cast<char*>(token.c_str()));
            }
            args.push_back(nullptr);  // Null terminate
    
            pid_t pid = fork();
    
            if (pid < 0) {
                perror("fork");
                continue;
            }
            else if (pid == 0) {
                // Child
                execvp(args[0], args.data());
    
                // Only runs if exec fails
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            else {
                // Parent
                waitpid(pid, nullptr, 0);
            }
        }
    }

    return 0;
}
