
/*
 * 
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"


void parse(std::vector<Token> &tokens)
{
    if (tokens[0].type == TOKEN_EOF){
        return;
    }
    Command currentCommand;
    SimpleCommand *currentSimpleCommand = new SimpleCommand();
    bool error = false;
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++)
    {
        TokenType type = tokens[i].type;

        if (type == TOKEN_COMMAND || type == TOKEN_ARGUMENT) {
            std::string word = tokens[i].value;
            if (word[0] == '"') {
                while (!(word.size() >= 2 && word.back() == '"') && tokens[i + 1].type != TOKEN_EOF) {
                    i++;
                    word += " " + tokens[i].value;
                }
                if (word.size() >= 2 && word.back() == '"') {
                    word = word.substr(1, word.size() - 2);
                }
            }
            currentSimpleCommand->insertArgument(strdup(word.c_str()));
        }
        else if (type == TOKEN_PIPE) {
            if (currentSimpleCommand->_numberOfArguments == 0) {
                fprintf(stderr, "myshell: syntax error near '|'\n");
                error = true;
                break;
            }
            currentCommand.insertSimpleCommand(currentSimpleCommand);
            currentSimpleCommand = new SimpleCommand();
        }
        else if (type == TOKEN_BACKGROUND) {
            currentCommand._background = 1;
        }
        else {
            TokenType next = tokens[i + 1].type;
            if (next != TOKEN_COMMAND && next != TOKEN_ARGUMENT) {
                fprintf(stderr, "myshell: syntax error: missing file name after redirection\n");
                error = true;
                break;
            }
            i++;
            char *file = strdup(tokens[i].value.c_str());
            if (type == TOKEN_REDIRECT) {
                currentCommand._outFile = file;
                currentCommand._append = 0;
            }
            else if (type == TOKEN_APPEND) {
                currentCommand._outFile = file;
                currentCommand._append = 1;
            }
            else if (type == TOKEN_INPUT) {
                currentCommand._inputFile = file;
            }
            else if (type == TOKEN_ERROR) {
                currentCommand._errFile = file;
            }
            else {
                currentCommand._outFile = file;
                currentCommand._errFile = strdup(file);
                currentCommand._append = 1;
                currentCommand._out_error = 1;
            }
          }
    }
    if (!error && currentSimpleCommand->_numberOfArguments == 0) {
        fprintf(stderr, "myshell: syntax error: missing command\n");
        error = true;
        }
    currentCommand.insertSimpleCommand(currentSimpleCommand);
    if (error){
        currentCommand.clear();
        }
    else {
        currentCommand.execute();
        }
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        delete(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{
    print();
    clear();
}

void Command::prompt()
{
    std::string input;
    while (true)
    {
        printf("myshell>");
        fflush(stdout);
        if (!std::getline(std::cin, input)) {
            printf("\n");
            break;
        }
        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    Command::_currentCommand.prompt();
    return 0;
}
