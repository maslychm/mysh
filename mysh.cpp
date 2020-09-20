#include <vector>
#include <iostream>
#include <cstring>
#include <regex>
#include <unistd.h>
#include <wait.h>

/**
 * Commands:
 * - movetodir
 *      change directory. internal variable. if doesn't exist -> error message
 * - whereami
 *      print current dir
 * - history [-c]
 *      no param -> print comm history
 *      param (-c) -> clear comm history
 * - byebye
 *      terminate shell
 * - start/run program [parameters]
 *      / for full path, else relative
 *      exec with params fork() + exec(), wait to terminate with waitpid()
 *      can't exec -> err msg
 * - background program [parameters]
 *      commands before, but print PID and returns to prompt
 * - exterminate PID
 *      kill() -> success/failure
 * - exterminateall
 *      print each murdered pid
 * - repeat n command
 *      repeat command n times
 */

enum ErrorCode {
    no_error = 0,
    command_does_not_exist = 1,
    incorrect_parameters = 2,
    dir_does_not_exist = 3,
    child_process_error = 4,
    could_not_kill = 5,
    request_exit = 10,
};

class Mysh {
    class Command {
    public:
        std::string keyword;

        bool InputParametersAreValid(std::vector<std::string> &iPs) {
            if (allowCustomParameters)
                return true;

            if (iPs.size() > this->validParameters.size())
                return false;

            for (const auto &ip : iPs) {
                if (std::find(std::begin(validParameters), std::end(validParameters), ip) ==
                    std::end(validParameters)) {
                    return false;
                }
            }
            return true;
        }

        virtual ErrorCode Execute(std::vector<std::string> &inputParameters) = 0;

    protected:
        Mysh *mysh;
        std::vector<std::string> validParameters;
        bool allowCustomParameters;

        Command() {
            mysh = nullptr;
            allowCustomParameters = false;
        }
    };

    class HistoryHandler {
        class History : public Command {
        public:
            explicit History(Mysh *mysh, HistoryHandler *inputHistory) {
                keyword = "history";
                validParameters = {"-c"};
                this->allowCustomParameters = false;
                this->mysh = mysh;
                this->inputHistory = inputHistory;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {

                for (const auto &parameter : inputParameters) {
                    if (parameter == "-c")
                        inputHistory->ClearInputHistoryLines();
                }

                inputHistory->PrintInputHistoryLines();
                return no_error;
            }

        private:
            HistoryHandler *inputHistory;
        };

    public:
        explicit HistoryHandler(Mysh *mysh) {
            this->mysh = mysh;
            mysh->commands->push_back(new History(mysh, this));
        }

        ~HistoryHandler() {
            this->ClearInputHistoryLines();
        }

        void UpdateInputHistory(const std::string &inputLine) {
            this->inputHistory.push_back(inputLine);
        }

    private:
        Mysh *mysh;
        std::vector<std::string> inputHistory;

        void ClearInputHistoryLines() {
            inputHistory.clear();
        }

        void PrintInputHistoryLines() {
            for (const auto &line : inputHistory)
                std::cout << "  " << line << std::endl;
        }
    };

    class ExitHandler {
        class Exit : public Command {
        public:
            explicit Exit(Mysh *mysh, ExitHandler *exitHandler) {
                this->keyword = "byebye";
                this->validParameters = {};
                this->allowCustomParameters = false;
                this->mysh = mysh;
                this->exitHandler = exitHandler;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {
                int running = mysh->processHandler->CountRunningProcesses();

                if (running > 0) {
                    std::cout << "There are " << running << " active jobs" << std::endl;

                    std::cout << "terminate before exit? (y/n)";
                    std::string input;
                    std::cin >> input;
                    // A bit ugly, but I want to avoid making KillAll() public,
                    // so all this following code is to run it
                    if (input == "y" or input == "yes") {
                        input = "exterminateall";
                        std::vector<std::string> vs = {};
                        for (auto command : *mysh->commands) {
                            if (command->keyword == input) {
                                command->Execute(vs);
                                break;
                            }
                        }
                        return request_exit;
                    }
                    else if (input == "n" or input == "no") {
                        return request_exit;
                    }

                    return no_error;
                }

                return request_exit;
            }

        private:
            ExitHandler *exitHandler;
        };

    public:
        explicit ExitHandler(Mysh *mysh) {
            this->mysh = mysh;
            mysh->commands->push_back(new Exit(mysh, this));
        }

        ~ExitHandler() = default;

    private:
        Mysh *mysh;
    };

    class DirectoryHandler {
        class WhereAmI : public Command {
        public:
            explicit WhereAmI(Mysh *mysh, DirectoryHandler *directoryHandler) {
                this->keyword = "whereami";
                this->validParameters = {};
                this->allowCustomParameters = false;
                this->mysh = mysh;
                this->directoryHandler = directoryHandler;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {
                directoryHandler->PrintCurrentDirectory();
                return no_error;
            }

        private:
            DirectoryHandler *directoryHandler;
        };

        class MoveToDirectory : public Command {
        public:
            explicit MoveToDirectory(Mysh *mysh, DirectoryHandler *directoryHandler) {
                this->keyword = "movetodir";
                this->validParameters = {};
                this->allowCustomParameters = true;
                this->mysh = mysh;
                this->directoryHandler = directoryHandler;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {
                if (inputParameters.empty())
                    return incorrect_parameters;

                std::string inputPath = inputParameters[0];

                ErrorCode ec = directoryHandler->ChangeCurrentDirectory(inputPath);

                return ec;
            }

        private:
            DirectoryHandler *directoryHandler;
        };

    public:
        explicit DirectoryHandler(Mysh *mysh) {
            this->mysh = mysh;
            BUFFERSIZE = 1024;
            currentDirectory = (char *) malloc(sizeof(char) * BUFFERSIZE);

            if (!getcwd(currentDirectory, BUFFERSIZE)) {
                perror("Error getting current directory");
            }

            mysh->commands->push_back(new WhereAmI(mysh, this));
            mysh->commands->push_back(new MoveToDirectory(mysh, this));
        }

        ~DirectoryHandler() {
            delete[] currentDirectory;
        }

        std::string GetCurrentDirectoryString() {
            return std::string(this->currentDirectory);
        }

    private:
        Mysh *mysh;
        int BUFFERSIZE;
        char *currentDirectory;

        void PrintCurrentDirectory() const {
            std::cout << currentDirectory << std::endl;
        }

        ErrorCode ChangeCurrentDirectory(std::string &inputPath) {
            ErrorCode errorCode = no_error;

            char *pathname = new char[BUFFERSIZE];
            strcpy(pathname, inputPath.c_str());

            if (chdir(pathname) == 0) {
                if (!getcwd(pathname, BUFFERSIZE)) {
                    perror("Error getting current directory");
                }
                std::strcpy(currentDirectory, pathname);
            } else {
                errorCode = dir_does_not_exist;
            }

            delete[] pathname;
            return errorCode;
        }
    };

    class ProcessHandler {
        class RunForeground : public Command {
        public:
            explicit RunForeground(Mysh *mysh, ProcessHandler *ph) {
                this->mysh = mysh;
                this->processHandler = ph;
                this->keyword = "run";
                this->validParameters = {};
                this->allowCustomParameters = true;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {

                if (inputParameters.empty())
                    return incorrect_parameters;

                char **arguments = InputParametersToCharArguments(inputParameters);

                ErrorCode ec = Mysh::ProcessHandler::ForkExecWait(arguments);

                for (long unsigned i = 0; i < inputParameters.size() + 1; i++)
                    delete[] arguments[i];
                delete[] arguments;

                return ec;
            }

        private:
            ProcessHandler *processHandler;
        };

        class RunBackground : public Command {
        public:
            explicit RunBackground(Mysh *mysh, ProcessHandler *ph) {
                this->mysh = mysh;
                this->processHandler = ph;
                this->keyword = "background";
                this->validParameters = {};
                this->allowCustomParameters = true;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {
                if (inputParameters.empty())
                    return incorrect_parameters;

                char **arguments = InputParametersToCharArguments(inputParameters);

                pid_t pid;
                ErrorCode ec = Mysh::ProcessHandler::ForkExecBackground(arguments, &pid);

                if (pid > 0) {
                    printf("child (pid:%ld)\n", (long) pid);
                    processHandler->AddBackgroundPID(pid);
                }

                for (long unsigned i = 0; i < inputParameters.size() + 1; i++)
                    delete[] arguments[i];
                delete[] arguments;

                return ec;
            }

        private:
            ProcessHandler *processHandler;
        };

        class ExterminatePID : public Command {
        public:
            explicit ExterminatePID(Mysh *mysh, ProcessHandler *ph) {
                this->mysh = mysh;
                this->processHandler = ph;
                this->keyword = "exterminate";
                this->validParameters = {};
                this->allowCustomParameters = true;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {
                if (inputParameters.empty())
                    return incorrect_parameters;

                pid_t pid;
                try {
                    pid = (pid_t) std::stoi(inputParameters[0]);
                } catch (std::exception &err) {
                    return incorrect_parameters;
                }

                ErrorCode ec = Mysh::ProcessHandler::KillPID(pid);

                if (ec == no_error) {
                    if (processHandler->RemoveBackgroundPID(pid))
                        std::cout << "successfully killed " << pid << std::endl;
                    else {
                        std::cout << pid << " was already dead" << std::endl;
                    }
                }

                return ec;
            }

        private:
            ProcessHandler *processHandler;
        };

        /**
         * Extra Credit: Murder all background processes
         */
        class ExterminateAll : public Command {
        public:
            explicit ExterminateAll(Mysh *mysh, ProcessHandler *ph) {
                this->mysh = mysh;
                this->processHandler = ph;
                this->keyword = "exterminateall";
                this->validParameters = {};
                this->allowCustomParameters = false;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {
                return processHandler->KillAllPIDs();
            }

        private:
            ProcessHandler *processHandler;
        };

        /**
         * Extra Credit: Repeat command N times
         */
        class Repeat : public Command {
        public:
            explicit Repeat(Mysh *mysh, ProcessHandler *ph) {
                this->mysh = mysh;
                this->processHandler = ph;
                this->keyword = "repeat";
                this->validParameters = {};
                this->allowCustomParameters = true;
            }

            ErrorCode Execute(std::vector<std::string> &inputParameters) override {
                ErrorCode errorCode = no_error;

                int n;
                try {
                    n = (int) std::stoi(inputParameters[0]);
                } catch (std::exception &err) {
                    return incorrect_parameters;
                }

                inputParameters.erase(inputParameters.begin());
                char **arguments = InputParametersToCharArguments(inputParameters);
                errorCode = processHandler->RepeatCommand(arguments, n);

                return errorCode;
            }

        private:
            ProcessHandler *processHandler;
        };

    public:
        explicit ProcessHandler(Mysh *mysh) {
            this->mysh = mysh;

            mysh->commands->push_back(new RunForeground(mysh, this));
            mysh->commands->push_back(new RunBackground(mysh, this));
            mysh->commands->push_back(new ExterminatePID(mysh, this));
            mysh->commands->push_back(new ExterminateAll(mysh, this));
            mysh->commands->push_back(new Repeat(mysh, this));
        }

        int CountRunningProcesses() {
            return backgroundPIDs.size();
        }

    private:
        Mysh *mysh;
        std::vector<pid_t> backgroundPIDs;

        static ErrorCode ForkExecWait(char **arguments) {
            pid_t c_pid, w;
            int wait_status;

            c_pid = fork();

            if (c_pid < 0) {
                perror("forking failed\n");
                exit(EXIT_FAILURE);
            }

            if (c_pid == 0) {
                int execError = execvp(arguments[0], arguments);

                if (execError == -1) {
                    std::cerr << "could not execute " << arguments[0] << std::endl;
                    exit(1);
                }
            } else {
                w = waitpid(c_pid, &wait_status, 0);

                if (w == -1) {
                    perror("waitpid");
                    return child_process_error;
                }
            }

            return no_error;
        }

        static ErrorCode ForkExecBackground(char **arguments, pid_t *pid) {
            pid_t c_pid;

            c_pid = fork();

            if (c_pid < 0) {
                perror("forking failed\n");
                exit(EXIT_FAILURE);
            }

            if (c_pid == 0) {
                // Set new process group to stop capturing input from caller shell
                setpgid(0, 0);

                int execError = execvp(arguments[0], arguments);

                if (execError == -1) {
                    std::cerr << "could not execute " << arguments[0] << std::endl;
                    exit(1);
                }
            } else {
                *pid = c_pid;
            }

            return no_error;
        }

        static ErrorCode KillPID(pid_t pid) {
            int kill_err = kill(pid, SIGINT);

            if (kill_err == 0)
                return no_error;

            kill_err = kill(pid, SIGTERM);

            if (kill_err == 0)
                return no_error;

            kill_err = kill(pid, SIGKILL);

            if (kill_err == 0)
                return no_error;

            if (kill_err == -1)
                return could_not_kill;

            return no_error;
        }

        ErrorCode KillAllPIDs() {
            ErrorCode errorCode = no_error;

            std::cout << "Murdering " << backgroundPIDs.size() << " processes: ";
            for (auto pid : backgroundPIDs) {
                std::cout << pid << " ";
                errorCode = KillPID(pid);
            }

            this->ClearBackgroundPIDs();

            std::cout << std::endl;
            return errorCode;
        }

        ErrorCode RepeatCommand(char **arguments, int n) {
            ErrorCode errorCode = no_error;
            pid_t pid;
            std::string ret = "PIDs: ";
            fflush(stdout);
            for (int i = 0; i < n; i++) {
                errorCode = ForkExecBackground(arguments, &pid);
                AddBackgroundPID(pid);
                ret += std::to_string(pid);
                ret += ", ";
            }
            std::cout << ret << std::endl;
            return errorCode;
        }

        void AddBackgroundPID(pid_t pid) {
            backgroundPIDs.push_back(pid);
        }

        bool RemoveBackgroundPID(pid_t pid) {
            for (long unsigned i = 0; i < backgroundPIDs.size(); i++) {
                if (pid == backgroundPIDs[i]) {
                    backgroundPIDs.erase(backgroundPIDs.begin() + i);
                    return true;
                }
            }

            ClearBackgroundPIDs();
            return false;
        }

        void ClearBackgroundPIDs() {
            backgroundPIDs.clear();
        }

        void ListBackgroundPIDs() {
            for (auto pid : backgroundPIDs) {
                std::cout << "bg:" << pid << std::endl;
            }
        }

        static char **InputParametersToCharArguments(std::vector<std::string> &inputParameters) {
            char **arguments = nullptr;
            int iPLen = inputParameters.size();

            arguments = new char *[iPLen + 1];
            for (auto i = 0; i < iPLen; i++) {
                arguments[i] = strdup(inputParameters[i].c_str());
            }
            arguments[iPLen] = nullptr;

            return arguments;
        }
    };

    class ErrorCodeHandler {
    public:
        static void HandleErrorCode(ErrorCode ec) {
            switch (ec) {
                case no_error:
                    return;
                case command_does_not_exist:
                    std::cout << "command not found" << std::endl;
                    break;
                case incorrect_parameters:
                    std::cout << "incorrect parameters" << std::endl;
                    break;
                case dir_does_not_exist:
                    std::cout << "directory does not exist" << std::endl;
                    break;
                case child_process_error:
                    std::cout << "child process error" << std::endl;
                    break;
                case could_not_kill:
                    std::cout << "could not kill specified pid" << std::endl;
                default:
                    break;
            }
        }
    };

public:
    Mysh() {
        this->commands = new std::vector<Command *>();

        historyHandler = new HistoryHandler(this);
        exitHandler = new ExitHandler(this);
        directoryHandler = new DirectoryHandler(this);
        processHandler = new ProcessHandler(this);

        errorCodeHandler = new ErrorCodeHandler();
    }

    ~Mysh() {
        delete historyHandler;
        delete exitHandler;
        delete directoryHandler;

        delete errorCodeHandler;

        delete commands;
    }

    void Start() {
        bool keepGoing = true;
        while (keepGoing) {
            printPrompt();
            ErrorCode ec = ProcessInput();

            Mysh::ErrorCodeHandler::HandleErrorCode(ec);

            if (ec == 10) {
                keepGoing = false;
                std::cout << "ret code: " << ec << std::endl;
            }
        }
    }

private:
    HistoryHandler *historyHandler;
    ExitHandler *exitHandler;
    DirectoryHandler *directoryHandler;
    ProcessHandler *processHandler;

    ErrorCodeHandler *errorCodeHandler;

    std::vector<Command *> *commands;

    void printPrompt() {
        std::cout << this->directoryHandler->GetCurrentDirectoryString() << "# ";
    }

    ErrorCode ProcessInput() {
        std::string inputLine;
        std::getline(std::cin, inputLine);

        historyHandler->UpdateInputHistory(inputLine);

        std::vector<std::string> tokens = TokenizeInput(inputLine);

        if (tokens[0].length() == 0) {
            return no_error;
        }

        Command *currentCommand = DetermineCommand(tokens[0]);

        if (currentCommand == nullptr) {
            return command_does_not_exist;
        }

        auto parameters = std::vector<std::string>(tokens.begin() + 1, tokens.end());

        if (!currentCommand->InputParametersAreValid(parameters)) {
            return incorrect_parameters;
        }

        return currentCommand->Execute(parameters);
    }

    static std::vector<std::string> TokenizeInput(const std::string &line) {
        auto const regex = std::regex{R"(\s+)"};

        auto tokens = std::vector<std::string>(
                std::sregex_token_iterator{begin(line), end(line), regex, -1},
                std::sregex_token_iterator{});

        return tokens;
    }

    Command *DetermineCommand(const std::string &firstToken) const {
        Command *currentCommand = nullptr;
        for (auto command : *commands) {
            if (firstToken == command->keyword) {
                currentCommand = command;
            }
        }
        return currentCommand;
    }
};


int TestMysh() {
    Mysh *mysh = new Mysh();

    mysh->Start();

    delete mysh;

    return 0;
}

int main() {
    TestMysh();
}