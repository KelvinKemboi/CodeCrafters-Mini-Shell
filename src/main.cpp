#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <algorithm>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  std::string input;
  while (true){
    std::cout << "$ ";
    std::getline(std::cin, input);
    std::stringstream stream(input);
    std::vector<std::string> words;
    std::string word;
    while(stream >> word){
      words.push_back(word);
    }
    if(words.empty()){
      continue;
    }
    if(words[0] == "exit"){
      break;
    }
    else if(words[0] == "echo"){
      std::string joined;
      if (words[1][0] == '\''){
        for (int i = 1; i < words.size(); i++) {
            if(!joined.empty()) joined += ' ';
            joined += words[i];
        }
        joined.erase(std::remove(joined.begin(), joined.end(), '\''), joined.end());
      }else{
        for (int i = 1; i < words.size(); i++) {
            joined += words[i];
        }
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
  }
  return 0;
}
