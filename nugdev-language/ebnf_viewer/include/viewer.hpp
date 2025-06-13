#pragma once

#include "ast.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace ebnf {

class Viewer {
public:
  Viewer();
  ~Viewer() = default;

  void run();
  void setGrammar(const std::unordered_map<std::string, ASTNodePtr> &rules);

private:
  void handleEvents();
  void update();
  void render();
  void drawNode(const ASTNodePtr &node, float x, float y, float level);
  void drawConnection(float x1, float y1, float x2, float y2);
  void drawText(const std::string &text, float x, float y,
                const sf::Color &color = sf::Color::White);

  sf::RenderWindow window_;
  sf::Font font_;
  std::unordered_map<std::string, ASTNodePtr> rules_;
  float zoom_;
  sf::Vector2f offset_;
  bool isDragging_;
  sf::Vector2f lastMousePos_;
};

} // namespace ebnf