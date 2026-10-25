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
#include <readline/readline.h>
#include <cstring>

char* commandGenerator(const char* text, int state) {
  static std::vector<std::string> commands = {"cd", "echo", "exit", "pwd", "type"};
  static int index = 0;

  if (state == 0) {
    index = 0;
    const char* pathEnv = std::getenv("PATH");
    if (pathEnv != nullptr) {
      std::stringstream paths(pathEnv);
      std::string directory;
      while (std::getline(paths, directory, ':')) {
        std::error_code error;
        for (std::filesystem::directory_iterator entry(directory, error), end;
             !error && entry != end; entry.increment(error)) {
          std::string command = entry->path().filename().string(); //get current entry filename
          //if is a file, executable and not in commands vector
          if (std::filesystem::is_regular_file(entry->path(), error) &&
              access(entry->path().c_str(), X_OK) == 0 &&
              std::find(commands.begin(), commands.end(), command) == commands.end()) {
            commands.push_back(command);
          }
        }
      }
    }
  }

  while (index < static_cast<int>(commands.size())) {
    const std::string& command = commands[index++];
    if (command.compare(0, std::strlen(text), text) == 0) {
    char* result = static_cast<char*>(std::malloc(command.size() + 1));
    if (result != nullptr) {
      std::strcpy(result, command.c_str());
    }
    return result;
    }
  }

  return nullptr;
}

char** shellCompletion(const char* text, int start, int end) {
  (void)end;
  if (start != 0) {
    return nullptr;
  }
  return rl_completion_matches(text, commandGenerator);
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  rl_attempted_completion_function = shellCompletion;
  while (true){
    char* line = readline("$ ");
    if(line == nullptr) break;
    input = line;
    std::free(line);
    std::vector<std::string> words;
    std::string word;
    char quote = '\0';
    bool tokenStarted = false;
    bool escaping = false;
    for (int i = 0; i<input.size();i++) {
      char character = input[i];
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
        bool append = i+1 < input.size() && input[i+1] == '>';
        if (append) ++i;
        std::string op = append ? ">>" : ">";
        if (tokenStarted) {
          if (word == "1" || word == "2") {
            op = word + op; 
          } else {
            words.push_back(word);
          }
          word.clear();
          tokenStarted = false;
        }
        words.push_back(op);
      }else if (std::isspace(static_cast<unsigned char>(character))) {
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
    for(int i = 0; i+1 < words.size(); i++){
      if(words[i] == ">" || words[i] == "1>"){
        out = words[i+1];
        words.erase(words.begin()+i, words.begin()+i+2);
        break;
      }
    }
    std::string out2;
    for(int i = 0; i+1 < words.size(); i++){
      if(words[i] == ">>" || words[i] == "1>>"){
        out2 = words[i+1];
        words.erase(words.begin()+i, words.begin()+i+2);
        break;
      }
    }
    std::string err;
    for(int i = 0; i+1 < words.size(); i++){
      if(words[i] == "2>"){
        err = words[i+1];
        words.erase(words.begin()+i, words.begin()+i+2);
        break;
      }
    }
    std::string err2;
    for(int i = 0; i+1 < words.size(); i++){
      if(words[i] == "2>>"){
        err2 = words[i+1];
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
    int savedStdout2 = -1;
    if (!out2.empty()) {
      std::cout.flush();
      int fileFd = open(out2.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644); //allow to write/create/append files - permission mode 0644
      if (fileFd == -1) {
        perror("redirection");
        continue;
      }
      savedStdout2 = dup(STDOUT_FILENO);
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
    int savedStderr2 = -1;
    if (!err2.empty()) {
      std::cout.flush();
      int fileFd = open(err2.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644); //allow to write/create/truncate files - permission mode 0644
      if (fileFd == -1) {
        perror("redirection");
        continue;
      }
      savedStderr2 = dup(STDERR_FILENO);
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

        if (command == "exit" || command == "echo" || command == "type" || command == "pwd" || command == "complete") {
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
      
    }else if(words[0] == "complete"){
      if (words.size() < 3){
        std::cout<< "Complete: missing operand" << std::endl;
      }
      std::cout << "complete: "<< words[2] << " : no completion specification" <<std::endl;
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
    //restore append stdout back to original - terminal
    if (savedStdout2 != -1) {
      std::cout.flush();
      dup2(savedStdout2, STDOUT_FILENO);
      close(savedStdout2);
    }
    //restore stderr back to original
    if(savedStderr != -1){
      std::cout.flush();
      dup2(savedStderr, STDERR_FILENO);
      close(savedStderr);
    }
    //restore stderr2 back to original
    if(savedStderr2 != -1){
      std::cout.flush();
      dup2(savedStderr2, STDERR_FILENO);
      close(savedStderr2);
    }
  }
  return 0;
}
