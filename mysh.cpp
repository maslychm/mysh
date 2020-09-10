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

        virtual void Execute() {
            std::cout << "ight " << std::endl;
        };
    protected:
        Command() {
            std::cout << "base called\n";
        }
    };

    class HistoryCommand : public Command {
    public:
        HistoryCommand(Mysh *mysh) {
            keyword = "history";
            this->mysh = mysh;
        }

        void Execute() {
            mysh->PrintInputHistory();
        }
    };

    class ExitCommand : public Command {
    public:
        ExitCommand(Mysh *mysh) {
            keyword = "byebye";
            this->mysh = mysh;
        }

        void Execute() {
            // FIXME fail for now
            exit(EXIT_FAILURE);
        }
    };

public:
    Mysh() {
        commands = new std::vector<Command *>();
    }

    void InitializeCommands() {
        commands->push_back(new HistoryCommand(this));
        commands->push_back(new ExitCommand(this));
    }

    void PrintCommands() const {
        fflush(stdout);
        std::cout << "There are: " << this->commands->size() << " commands available" << std::endl;
        for (auto a : *this->commands) {
            std::cout << a->keyword << std::endl;
        }
    }

    void Start() {

        bool keepGoing = true;

        while (keepGoing) {
            std::cout << "# ";

            ProcessInput();

//            keepGoing =
        }
    }

    void ProcessInput() {
        std::string inputLine;
        std::getline(std::cin, inputLine);

        char *inputHistoryEntry = new char;
        std::strcpy(inputHistoryEntry, inputLine.c_str());

        inputHistory.push_back(inputHistoryEntry);

        // TODO refactor - too many lines
        std::vector<std::string> tokens = TokenizeCommand(inputLine);
        std::string firstToken = tokens[0];

        Command *currentCommand = nullptr;
        for (auto command : *commands) {
            if (firstToken.compare(command->keyword) == 0) {
                currentCommand = command;
            }
        }

        if (currentCommand == nullptr) {
            std::cerr << "Command was not found" << std::endl;
            return;
        }

        // FIXME get parameters

        currentCommand->Execute();
    }

    std::vector<std::string> TokenizeCommand(const std::string &line) {
        auto const regex = std::regex{R"(\s+)"};

        const std::vector<std::string> tokens = std::vector<std::string>(
                std::sregex_token_iterator{begin(line), end(line), regex, -1},
                std::sregex_token_iterator{});

        return tokens;
    }

    void PrintInputHistory() {
        for (auto entry : this->inputHistory) {
            std::cout << entry << std::endl;
        }
    }

private:
    std::vector<Command *> *commands;
    std::vector<char *> inputHistory;
};


int main() {
    Mysh *mysh = new Mysh();
    mysh->InitializeCommands();
    mysh->PrintCommands();
    mysh->Start();
}