#include <iostream>
#include <string>
#include <sstream>
#include <vector>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  std::string input;
  while (true){
    std::cout << "$ ";
    std::getline(std::cin, input);
    if(input == "exit"){
      return 0;
    }
    std::stringstream stream(input);
    std::vector<std::string> words;
    std::string word;

    while(stream >> word){
      words.push_back(word);
    }
    if(words[0] == "echo"){
      std::string joined;
      for (int i = 1; i < words.size(); i++) {
          if (!joined.empty()) joined += ' ';
          joined += words[i];
      }
      std::cout << joined << std:: endl;
    }
    std::cout << input << ": command not found" << std::endl;
  }
  return 0;
}
