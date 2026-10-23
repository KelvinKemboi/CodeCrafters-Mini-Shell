#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <algorithm>
#include <fcntl.h>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  while (true){
    std::cout << "$ ";
    std::getline(std::cin, input);
    std::vector<std::string> words;
    std::string word;
    char quote = '\0';
    bool tokenStarted = false;
    bool escaping = false;
    for (char character : input) {
      if (escaping) {
        word += character;
        escaping = false;
        tokenStarted = true;
      } else if (quote == '\'') {
        if (character == '\'') {
          quote = '\0';
        } else {
          word += character;
        }
        tokenStarted = true;
      } else if (character == '\\') {
        escaping = true;
        tokenStarted = true;
      } else if (quote != '\0') {
        if (character == quote) {
          quote = '\0';
        } else {
          word += character;
        }
        tokenStarted = true;
      } else if (character == '\'' || character == '"') {
        quote = character;
        tokenStarted = true;
      } else if(character == '>'){
        if (tokenStarted) {
          if (word == "1") {
            words.push_back("1>");
          } else if(word == "2"){
            words.push_back("2>");
          }else {
            words.push_back(word);
            words.push_back(">");
          }
          word.clear();
          tokenStarted = false;
        } else {
          words.push_back(">");
        }
      } else if (std::isspace(static_cast<unsigned char>(character))) {
        if (tokenStarted) {
          words.push_back(word);
          word.clear();
          tokenStarted = false;
        }
      } else {
        word += character;
        tokenStarted = true;
      }
    }
    if (tokenStarted) {
      words.push_back(word);
    }
    if(words.empty()){
      continue;
    }
    std::string out;
    std::string err;
    for(int i = 0; i+1 < words.size(); i++){
      if(words[i] == ">" || words[i] == "1>"){
        out = words[i+1];
        words.erase(words.begin()+i, words.begin()+i+2);
        break;
      }
    }
    for(int i = 0; i+1 < words.size(); i++){
      if(words[i] == "2>"){
        out = words[i+1];
        words.erase(words.begin()+i, words.begin()+i+2);
        break;
      }
    }
    int savedStdout = -1;
    if (!out.empty()) {
      std::cout.flush();
      int fileFd = open(out.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //allow to write/create/truncate files - permission mode 0644
      if (fileFd == -1) {
        perror("redirection");
        continue;
      }
      savedStdout = dup(STDOUT_FILENO);
      dup2(fileFd, STDOUT_FILENO);
      close(fileFd);
    }
    int savedStderr = -1;
    if (!err.empty()) {
      std::cout.flush();
      int fileFd = open(err.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); //allow to write/create/truncate files - permission mode 0644
      if (fileFd == -1) {
        perror("redirection");
        continue;
      }
      savedStderr = dup(STDERR_FILENO);
      dup2(fileFd, STDERR_FILENO);
      close(fileFd);
    }
    if(words[0] == "exit"){
      break;
    }
    else if(words[0] == "echo"){
      std::string joined;
      for (int i = 1; i < words.size(); i++) {
        if (!joined.empty()) joined += ' ';
        joined += words[i];
      }
      std::cout << joined << std:: endl;
    }else if (words[0] == "type") {
      if (words.size() < 2) {
        std::cout << "type: missing operand" << std::endl;
      } else {
        const std::string& command = words[1];

        if (command == "exit" || command == "echo" || command == "type" || command == "pwd") {
          std::cout << command << " is a shell builtin" << std::endl;
        } else {
          bool found = false;
          const char* pathEnv = std::getenv("PATH"); //get path env

          if (pathEnv) {
            std::stringstream paths(pathEnv);
            std::string directory;
            //parsing the path
            while (std::getline(paths, directory, ':')) {
              auto candidate = std::filesystem::path(directory) / command;
              //check if exists in path and is executable
              if (std::filesystem::is_regular_file(candidate) && access(candidate.c_str(), X_OK) == 0) {
                std::cout << command << " is " << candidate.string() << std::endl;
                found = true;
                break;
              }
            }
          }

          if (!found) {
            std::cout << command << ": not found" << std::endl;
          }
        }
      }
    }else if(words[0] == "pwd"){
      std::cout << std::filesystem::current_path().string() <<'\n';
    }else if(words[0] == "cd"){
      if(words.size() < 2){
        std::cout << "cd: missing operand" << std::endl;
      }
      
      try{
        if(words[1] == "~"){
          const char* home = std::getenv("HOME");
          std::filesystem::current_path(home);
        }else{
          std::filesystem::current_path(words[1]);
        }
      }catch(const std::filesystem::filesystem_error& error){
        std::cout << "cd: " << words[1] << ": No such file or directory" << std::endl;
      }
      
    }
    else if(!words.empty()){
      std::vector<char*> argv;
      //transfer to char* vector for exec
      for(std::string &w: words){
        argv.push_back(w.data());
      }
      argv.push_back(nullptr);
      bool found = false;
      const char* pathEnv = std::getenv("PATH");
      if (pathEnv) {
        std::stringstream paths(pathEnv);
        std::string directory;

        while (std::getline(paths, directory, ':')) {
          auto candidate = std::filesystem::path(directory) / argv[0];
          if (std::filesystem::is_regular_file(candidate) && access(candidate.c_str(), X_OK) == 0) {
            pid_t child = fork();
            if(child < 0){
              perror("fork");
            }
            else if(child == 0){
              execvp(argv[0], argv.data());
              perror("Failed");
              exit(1);
            }else{
              waitpid(child, nullptr, 0);
            }
            found = true;
            break;
          }
        }
      }

      if (!found) {
        std::cout << argv[0] << ": not found" << std::endl;
      }
      
    }
    else{
      std::cout << input << ": command not found" << std::endl;
    }
    //restore stdout back to original - terminal
    if (savedStdout != -1) {
      std::cout.flush();
      dup2(savedStdout, STDOUT_FILENO);
      close(savedStdout);
    }
    //restore stderr back to original
    if(savedStderr != -1){
      std::cout.flush();
      dup2(savedStderr, STDERR_FILENO);
      close(savedStderr);
    }
  }
  return 0;
}
