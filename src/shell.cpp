#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <array>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstdlib>
#include <signal.h>

void handle_sigchld(int) {
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
        // Reap all finished background children
    }
}

int main() {

    // Install SIGCHLD handler
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
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

        if (line.empty()) continue;

        // Tokenize
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string word;

        while (iss >> word)
            tokens.push_back(word);

        if (tokens.empty()) continue;

        // Background detection
        bool background = false;
        if (tokens.back() == "&") {
            background = true;
            tokens.pop_back();
        }

        if (tokens.empty()) continue;

        // Built-ins
        if (tokens[0] == "exit") {
            break;
        }

        if (tokens[0] == "cd") {
            if (tokens.size() > 2) {
                std::cerr << "cd: too many arguments\n";
                continue;
            }

            if (tokens.size() == 1) {
                const char* home = getenv("HOME");
                if (!home || chdir(home) == -1)
                    perror("cd");
            } else {
                if (chdir(tokens[1].c_str()) == -1)
                    perror("cd");
            }

            continue;
        }

        if (tokens[0] == "pwd") {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr)
                std::cout << cwd << std::endl;
            else
                perror("pwd");
            continue;
        }

        // Check for pipeline
        bool has_pipe = false;
        for (const auto& t : tokens) {
            if (t == "|") {
                has_pipe = true;
                break;
            }
        }

        // ================= PIPELINE =================
        if (has_pipe) {

            if (background) {
                std::cerr << "background pipelines not supported yet\n";
                continue;
            }

            // Split into commands
            std::vector<std::vector<std::string>> commands;
            commands.emplace_back();

            bool syntax_error = false;

            for (const auto& t : tokens) {
                if (t == "|") {
                    if (commands.back().empty()) {
                        syntax_error = true;
                        break;
                    }
                    commands.emplace_back();
                } else {
                    commands.back().push_back(t);
                }
            }

            if (syntax_error || commands.back().empty()) {
                std::cerr << "syntax error near unexpected token `|`\n";
                continue;
            }

            int n = commands.size();

            // Create pipes
            std::vector<std::array<int,2>> pipes(n - 1);

            bool error = false;

            for (int i = 0; i < n - 1; ++i) {
                if (pipe(pipes[i].data()) == -1) {
                    perror("pipe");
                    error = true;
                    break;
                }
            }

            if (error) {
                for (int i = 0; i < n - 1; ++i) {
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }
                continue;
            }

            std::vector<pid_t> pids;

            for (int i = 0; i < n; ++i) {

                std::vector<char*> argv;
                for (auto& s : commands[i])
                    argv.push_back(const_cast<char*>(s.c_str()));
                argv.push_back(nullptr);

                pid_t pid = fork();

                if (pid < 0) {
                    perror("fork");
                    error = true;
                    break;
                }

                if (pid == 0) {

                    // Connect input
                    if (i > 0)
                        dup2(pipes[i - 1][0], STDIN_FILENO);

                    // Connect output
                    if (i < n - 1)
                        dup2(pipes[i][1], STDOUT_FILENO);

                    // Close all pipe fds
                    for (int k = 0; k < n - 1; ++k) {
                        close(pipes[k][0]);
                        close(pipes[k][1]);
                    }

                    execvp(argv[0], argv.data());
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }

                pids.push_back(pid);
            }

            // Parent closes pipes
            for (int i = 0; i < n - 1; ++i) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            if (!error) {
                for (pid_t pid : pids)
                    waitpid(pid, nullptr, 0);
            }

            continue;
        }

        // ================= NORMAL COMMAND =================
        std::vector<char*> args;
        for (auto& t : tokens)
            args.push_back(const_cast<char*>(t.c_str()));
        args.push_back(nullptr);

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            execvp(args[0], args.data());
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        if (!background) {
            waitpid(pid, nullptr, 0);
        } else {
            std::cout << "[Background PID: " << pid << "]\n";
        }
    }

    return 0;
}
