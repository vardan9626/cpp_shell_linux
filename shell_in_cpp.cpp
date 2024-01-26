#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;

void handler(int signal)
{ cout<<endl;return;}

string trim(const string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r\f\v"); // Whitespaces
    if (first == string::npos)return ""; // String is all whitespaces
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

pid_t shell_pid = getpid();
set<pid_t> fg_process, bg_process;

int main()
{
    string input, curr_dir, tmp;
    vector<string> tokens;
    vector<vector<string>> commands;
    signal(2, handler);
    while (true)
    {
        curr_dir = getcwd(NULL, 0);
        cout << "\033[1;31mvardan@host:\033[0m"<< "\033[32m" << curr_dir << "$ \033[0m";
        getline(cin, input, '\n');
        input = trim(input);
        stringstream IN(input);
        // Split the input into tokens
        while (getline(IN, tmp, ' ')){
            tmp = trim(tmp);
            if(tmp.size()>0)tokens.push_back(tmp);
        }
        // Check if the input is empty
        if (tokens.size() == 0)continue;
        vector<string> commands_tokens;
        tmp = "&&";
        for (int i = 0; i < int(tokens.size()); i++)
        {
            if (strcmp(tokens[i].c_str(), ("&&")) == 0 || strcmp(tokens[i].c_str(), "&&&") == 0)
            {
                tmp = tokens[i];
                commands.push_back(commands_tokens);
                commands_tokens.clear();
            }
            else commands_tokens.push_back(tokens[i]);
        }
        commands.push_back(commands_tokens);
        int code = 0;
        for (vector<string> command : commands)
        {
            if (code != 0 && strcmp(tmp.c_str(), "&&") == 0)
                break;

            // Check for exit
            if (strcmp(command[0].c_str(), "exit") == 0)
            {
                for (auto it : bg_process)kill(it, 9);
                kill(-getpgid(shell_pid), 9);
                exit(0);
            }
            // Check for cd
            else if (strcmp(command[0].c_str(), "cd") == 0)
            {
                if (command.size() == 1)chdir(getenv("HOME"));
                else chdir(command[1].c_str());
                continue;
            }

            bool in_backGround = false;

            if ((command[command.size() - 1][0]) == '&'){
                // cout << "background process detected\n";
                in_backGround = true;
                command.pop_back();
            }

            pid_t pid = fork();
            if (pid < 0)
            {
                cout << "Error in forking\n";
                exit(1);
            }
            else if (pid == 0)
            {
                // Child Process
                pid_t set_pgid_result;
                if (in_backGround == true)set_pgid_result = setpgid(0, 0);
                char **ARGS = new char *[command.size() + 1];
                for (int i = 0; i < command.size(); i++)
                    ARGS[i] = const_cast<char *>(command[i].c_str());
                execvp(ARGS[0], ARGS);
                cout << "error executing command\n";
                exit(-1);
            }
            else
            {
                // Parent Process
                if (in_backGround)
                    // bg_process
                    bg_process.insert(pid);
                else
                {
                    // fg_process
                    fg_process.insert(pid);
                    if (strcmp(tmp.c_str(), "&&") == 0){
                        waitpid(pid, &code, 0);
                        fg_process.erase(pid);
                    }
                    else pid_t ret = waitpid(0, NULL, WNOHANG);
                }
                pid_t ret = waitpid(-1, NULL, WNOHANG);
                if (ret > 0)
                {
                    if (ret == getpgid(ret))
                    {
                        cout << "Process with PID " << ret << " exitted normally\nWas running in background\n";
                        bg_process.erase(ret);
                    }
                    else
                    {
                        cout << "Process with PID " << ret << " exitted normally\nWas Running in Foreground\n";
                        fg_process.erase(ret);
                    }
                }
            }
        }

        while (fg_process.size() != 0)
        {
            pid_t ret = waitpid(-getpgid(shell_pid), NULL, WNOHANG);
            if (ret > 0)
            {
                // cout << "Process with PID " << ret << " exitted normally\nWas Running in Foreground\n";
                fg_process.erase(ret);
            }
        }

        // Clear the vectors
        tokens.clear();
        commands_tokens.clear();
        commands.clear();
    }
}
