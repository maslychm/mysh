#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <istream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

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

// FIXME issues right now with movetodir command
// some cases of .. concatenation are handled
// consider using regex

// ex:
// /home/mykola/projects/OS/homework3/mysh/.. # movetodir ..
// home/mykola/projects/OS/homework3/mysh # movetodir ../mysh
// /home/mykola/projects/OS/homework3 #

// This is supposed to stay in the same directory after the ../mysh call

// More issure with using the "go back" operator, but do we even need to implement it? :upside_down:?

class Mysh {
    class Command {
    public:
        std::string keyword;
        Mysh *mysh;

        virtual int Execute(std::vector<std::string> &parameters) = 0;

        virtual ~Command() = default;

    protected:
        Command() {
            mysh = nullptr;
        }
    };

    class History : public Command {
    public:
        explicit History(Mysh *mysh) {
            keyword = "history";
            this->mysh = mysh;
        }

        ~History() override {
            mysh->FreeInputHistory();
        }

        int Execute(std::vector<std::string> &parameters) override {
            if (parameters.size() > 1) {
                std::cout << "history: too many arguments\n";
                return 0;
            }

            for (const auto &parameter : parameters) {
                if (parameter == "-c") {
                    mysh->FreeInputHistory();
                    break;
                }
                return 0;
            }

            mysh->PrintInputHistory();
            return 0;
        }
    };

    class Exit : public Command {
    public:
        explicit Exit(Mysh *mysh) {
            keyword = "byebye";
            this->mysh = mysh;
        }

        int Execute(std::vector<std::string> &parameters) override {
            return 1;
        }
    };

    class MoveToDirectory : public Command {
    public:
        explicit MoveToDirectory(Mysh *mysh) {
            keyword = "movetodir";
            this->mysh = mysh;
        }

        int Execute(std::vector<std::string> &parameters) override {
            if (parameters.size() == 0 or parameters.size() > 1) {
                std::cout << "movetodir: incorrect number or arguments\n";
                return 0;
            }

            const char* pathname = parameters[0].c_str();

            mysh->ChangeCurrentDirectory(pathname);

            return 0;
        }
    };

    class WhereAmI : public Command {
    public:
        explicit WhereAmI(Mysh *mysh) {
            keyword = "whereami";
            this->mysh = mysh;
        }

        int Execute(std::vector<std::string> &parameters) override {
            mysh->PrintCurrentDirectory();
            return 0;
        }
    };

public:
    Mysh() {
        this->InitializeCommands();
        this->InitializeCurrentDirectory();
    };

    ~Mysh() {
        this->FreeCommands();
        this->FreeInputHistory();
        this->FreeCurrentDirectory();
    }

    void Start() {
        bool keepGoing = true;
        while (keepGoing) {
            printPrompt();
            keepGoing = ProcessInput();
        }
    }

private:
    std::vector<Command *> commands;
    std::vector<char *> inputHistory;
    char *currentDirectory;

    void UpdateInputHistory(std::string &inputLine) {
        char *inputHistoryEntry = new char;
        std::strcpy(inputHistoryEntry, inputLine.c_str());
        this->inputHistory.push_back(inputHistoryEntry);
    }

    void PrintInputHistory() {
        for (auto entry : this->inputHistory) {
            std::cout << entry << std::endl;
        }
    }

    void printPrompt() {
        std::cout << this->currentDirectory << " # ";
    }

    bool ProcessInput() {
        std::string inputLine;
        std::getline(std::cin, inputLine);

        UpdateInputHistory(inputLine);

        std::vector<std::string> tokens = TokenizeCommand(inputLine);
        std::string firstToken = tokens[0];

        Command *currentCommand = DetermineCommand(firstToken);

        if (currentCommand == nullptr) {
            std::cerr << firstToken << ": command not found" << std::endl;
            return true;
        }

        auto parameters = std::vector<std::string>(tokens.begin() + 1, tokens.end());

        int returnCode = currentCommand->Execute(parameters);

        if (returnCode == 1) {
            return false;
        }

        return true;
    }

    Command *DetermineCommand(const std::string &firstToken) const {
        Command *currentCommand = nullptr;
        for (auto command : commands) {
            if (firstToken == command->keyword) {
                currentCommand = command;
            }
        }
        return currentCommand;
    }

    static std::vector<std::string> TokenizeCommand(const std::string &line) {
        auto const regex = std::regex{R"(\s+)"};

        const auto tokens = std::vector<std::string>(
                std::sregex_token_iterator{begin(line), end(line), regex, -1},
                std::sregex_token_iterator{});

        return tokens;
    }

    void InitializeCommands() {
        commands.push_back(new History(this));
        commands.push_back(new Exit(this));
        commands.push_back(new WhereAmI(this));
        commands.push_back(new MoveToDirectory(this));
    }

    void InitializeCurrentDirectory() {
        int BUFFERSIZE = 1024;

        currentDirectory = new char[BUFFERSIZE];

        if (!getcwd(currentDirectory, BUFFERSIZE))
            perror("ERROR");
    }

    void FreeInputHistory() {
        for (auto &i : inputHistory) {
            delete i;
        }
        inputHistory.clear();
    }

    void FreeCurrentDirectory() {
        delete[] currentDirectory;
    }

    void FreeCommands() {
        for (auto &i : commands) {
            delete i;
        }
        commands.clear();
    }

    void PrintCommands() const {
        std::cout << "There are: " << this->commands.size() << " commands available" << std::endl;
        for (auto a : this->commands) {
            std::cout << a->keyword << std::endl;
        }
    }

    void ChangeCurrentDirectory(const char* newPath) const {
        char *pathToCheck = new char[1024];

        BuildPathToCheck(newPath, pathToCheck);

        if (chdir(pathToCheck) == 0){
            strcpy(currentDirectory, pathToCheck);
        } else {
            std::cout << pathToCheck << std::endl;
            std::cerr << "directory doesn't exist\n";
        }

        delete[] pathToCheck;
    }

    void BuildPathToCheck(const char *newPath, char* pathToCheck) const {
        if (newPath[0] == '/') {
            strcpy(pathToCheck, newPath);
        } else if (newPath[0] == '.' and newPath[1] == '.') {
            strcpy(pathToCheck, currentDirectory);
            GetOneUpPath(pathToCheck);
        } else {
            strcpy(pathToCheck, currentDirectory);
            if (pathToCheck[strlen(pathToCheck) - 1] != '/'){
                strcat(pathToCheck, "/");
            }
            strcat(pathToCheck, newPath);
        }
    }

    void GetOneUpPath(char* pathToCheck) const {
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
        for (int i = strlen(pathToCheck) - 1; i >= 0; i--) {
            if (pathToCheck[i] == '/') {
                pathToCheck[i] = '\0';
                break;
            }
        }
    }

    bool CheckIfRootPath() const {
        return strlen(currentDirectory) == 1 and currentDirectory[0] == '/';
    }

    bool CheckIfDirectoryExists(const char* pathname) const {
//        // check if exists
//        struct stat info;
//
//        if (stat(pathname, &info) != 0){
//            std::cout << "Cannot access " << pathname << std::endl;
//        } else if (info.st_mode & S_IFDIR) {
//            return true;
//        } else if (info.st_mode & S_IFREG) {
//            std::cout << "is a file\n";
//        } else {
//            std::cout << "is not a directory\n";
//        }
//
        return false;
    }

    void PrintCurrentDirectory() const {
        std::cout << currentDirectory << std::endl;
    }
};


int main() {
    Mysh *mysh = new Mysh();

    mysh->Start();

    delete mysh;
}