#include "parser.hpp"
#include "viewer.hpp"
#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char *argv[]) {
  std::cerr << "main started" << std::endl;
  try {
    if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " <ebnf_file>" << std::endl;
      return 1;
    }
    std::cerr << "opening file..." << std::endl;
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << argv[1] << std::endl;
      return 1;
    }
    std::cerr << "reading file..." << std::endl;
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::cerr << "parsing..." << std::endl;
    ebnf::Parser parser;
    parser.parse(content);

    std::cerr << "creating viewer..." << std::endl;
    ebnf::Viewer viewer;
    std::cerr << "setting grammar..." << std::endl;
    viewer.setGrammar(parser.getRules());
    std::cerr << "before viewer.run()" << std::endl;
    viewer.run();
    std::cerr << "after viewer.run()" << std::endl;
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "Exception: " << ex.what() << std::endl;
    return 2;
  } catch (...) {
    std::cerr << "Unknown exception occurred." << std::endl;
    return 3;
  }
}
