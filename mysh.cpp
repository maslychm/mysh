#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <istream>

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

class Mysh {
    class Command {
    public:
        std::string keyword;
        Mysh *mysh;

        virtual void Execute(std::vector<std::string> &parameters) = 0;

    protected:
        Command() {
            mysh = nullptr;
        }

        virtual ~Command() = default;
    };

    class History : public Command {
    public:
        explicit History(Mysh *mysh) {
            keyword = "history";
            this->mysh = mysh;
        }

        void Execute(std::vector<std::string> &parameters) override {
            for (const auto &parameter : parameters) {
                if (parameter == "-c") {
                    mysh->ClearInputHistory();
                }
            }

            mysh->PrintInputHistory();
        }
    };

    class Exit : public Command {
    public:
        explicit Exit(Mysh *mysh) {
            keyword = "byebye";
            this->mysh = mysh;
        }

        void Execute(std::vector<std::string> &parameters) override {
            // TODO free commands and history before exit
            exit(EXIT_SUCCESS);
        }
    };

    class MoveToDirectory : Command {

    };

public:
    Mysh() {
        this->InitializeCommands();
    };

    void InitializeCommands() {
        commands.push_back(new History(this));
        commands.push_back(new Exit(this));
    }

    void PrintCommands() const {
        fflush(stdout);
        std::cout << "There are: " << this->commands.size() << " commands available" << std::endl;
        for (auto a : this->commands) {
            std::cout << a->keyword << std::endl;
        }
    }

    void Start() {

        bool keepGoing = true;

        while (keepGoing) {
            printOctothorpe();

            keepGoing = ProcessInput();
        }
    }

    static void printOctothorpe() { std::cout << "# "; }

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

        currentCommand->Execute(parameters);

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

    void ClearInputHistory() {
        for (auto &i : inputHistory) {
            delete i;
        }
        inputHistory.clear();
    }

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

private:
    std::vector<Command *> commands;
    std::vector<char *> inputHistory;
    char * currentDirectory;
};


int main() {
    Mysh *mysh = new Mysh();
    mysh->PrintCommands();
    mysh->Start();
}