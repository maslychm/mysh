#include <vector>
#include <iostream>
#include <cstring>
#include <regex>
#include <unistd.h>

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
 * -
 */

enum ErrorCode {
    no_error = 0,
    command_does_not_exist = 1,
    incorrect_parameters = 2,
    dir_does_not_exist = 3,
    request_exit = 10,
};

class Mysh {
    class Command {
    public:
        std::string keyword;
        std::vector<std::string> validParameters;
        bool allowCustomParameters;
        Mysh *mysh;

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

        void UpdateInputHistory(std::string &inputLine) {
            char *inputHistoryEntry = new char;
            std::strcpy(inputHistoryEntry, inputLine.c_str());
            this->inputHistory.push_back(inputHistoryEntry);
        }

    private:
        Mysh *mysh;
        std::vector<char *> inputHistory;

        void ClearInputHistoryLines() {
            for (auto line : inputHistory)
                delete line;

            inputHistory.clear();
        }

        void PrintInputHistoryLines() {
            for (auto line : inputHistory)
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
                // FIXME ask if to interrupt background jobs

                // End background jobs

                // return non 0 for exit
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
                const char *pathname;

                if (inputParameters.size() == 0) {
                    pathname = "/";
                } else {
                    pathname = inputParameters[0].c_str();
                }
                ErrorCode ec = directoryHandler->ChangeCurrentDirectory(pathname);

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

        ErrorCode ChangeCurrentDirectory(const char *newPath) {
            char *pathToCheck = new char[BUFFERSIZE];

            BuildPathToCheck(newPath, pathToCheck);

            if (chdir(pathToCheck) == 0) {
                delete[] currentDirectory;
                currentDirectory = strdup(pathToCheck);
            } else {
                return dir_does_not_exist;
            }

            delete[] pathToCheck;
            return no_error;
        }

        void BuildPathToCheck(const char *newPath, char *pathToCheck) const {
            if (newPath[0] == '/') {
                strcpy(pathToCheck, newPath);
            } else if (newPath[0] == '.' and newPath[1] == '.') {
                strcpy(pathToCheck, currentDirectory);
                GetOneUpPath(pathToCheck);
            } else {
                strcpy(pathToCheck, currentDirectory);
                if (pathToCheck[strlen(pathToCheck) - 1] != '/') {
                    strcat(pathToCheck, "/");
                }
                strcat(pathToCheck, newPath);
            }
        }

        void GetOneUpPath(char *pathToCheck) const {
            if (CheckIfRootPath())
                return;

            // Comments as a way to excuse bad code
            // Count how many slashes we have to know how close to root we are
            // if just one away, go to root
            int slashCount = 0;
            for (long unsigned int i = 0; i < strlen(currentDirectory) - 1; i++) {
                if (currentDirectory[i] == '/') {
                    slashCount++;
                }
            }
            if (slashCount == 1) {
                strcpy(pathToCheck, "/");
                return;
            }

            // to move up one directory, null terminate after last slash
            for (auto i = strlen(pathToCheck) - 1; i >= 0; i--) {
                if (pathToCheck[i] == '/') {
                    pathToCheck[i] = '\0';
                    break;
                }
            }
        }

        bool CheckIfRootPath() const {
            return strlen(currentDirectory) == 1 and currentDirectory[0] == '/';
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

            this->errorCodeHandler->HandleErrorCode(ec);

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
    // other handlers will go here

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

        const auto tokens = std::vector<std::string>(
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