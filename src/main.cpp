#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  std::string input;
  while (true){
    if(input == "exit"){
      break;
    }
    std::cout << "$ ";
    std::getline(std::cin, input);
    std::cout << input << ": command not found" << std::endl;
  }
  return 0;
  
}
