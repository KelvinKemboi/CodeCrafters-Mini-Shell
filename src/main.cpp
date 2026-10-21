#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  std::string input;
  while (true){
    std::cout << "$ ";
    std::getline(std::cin, input);
    std::vector<std::string> words;
    std::string word;
    char quote = '\0';
    bool tokenStarted = false;
    for (char character : input) {
      if (quote != '\0') {
        if (character == quote) {
          quote = '\0';
        } else {
          word += character;
        }
        tokenStarted = true;
      } else if (character == '\'' || character == '"') {
        quote = character;
        tokenStarted = true;
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

        if (command == "exit" || command == "echo" || command == "type") {
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
    }else if(!words.empty()){
      std::vector<char*> argv;
      //transfer to char* vector for exec
      for(std::string &w: words){
        argv.push_back(w.data());
      }
      argv.push_back(nullptr);

      pid_t child = fork();
      if (child < 0) {
        perror("fork");
      } else if (child == 0) {
        execvp(argv[0], argv.data());
        std::cerr << words[0] << ": command not found" << std::endl;
        _exit(127);
      } else {
        waitpid(child, nullptr, 0);
      }
      
    }
    else{
      std::cout << input << ": command not found" << std::endl;
    }
  }
  return 0;
}
