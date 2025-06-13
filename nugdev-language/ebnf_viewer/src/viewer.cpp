#include "viewer.hpp"
#include <iostream>
#include <unordered_map>
#include <variant>

namespace ebnf {

// Helper for std::visit with lambdas
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Viewer::Viewer()
    : window_(sf::VideoMode(sf::Vector2u(1024, 768)), "EBNF Viewer",
              sf::Style::Default),
      zoom_(1.0f), offset_(0.0f, 0.0f), isDragging_(false) {

  auto fontPath = std::filesystem::path("C:/NanumGothic.ttf");
  std::cerr << "Trying to load font from: " << fontPath.string() << std::endl;
  if (!font_.openFromFile(fontPath)) {
    std::cerr << "Failed to load D2Coding font from Windows system folder"
              << std::endl;
  }

  // Add some test grammar rules
  auto rule1 = std::make_shared<ASTNode>(NodeType::Rule, "Rule1");
  auto rule2 = std::make_shared<ASTNode>(NodeType::Rule, "Rule2");
  auto rule3 = std::make_shared<ASTNode>(NodeType::Rule, "Rule3");

  rule1->addChild(rule2);
  rule1->addChild(rule3);

  std::unordered_map<std::string, ASTNodePtr> testRules;
  testRules["Rule1"] = rule1;
  testRules["Rule2"] = rule2;
  testRules["Rule3"] = rule3;

  setGrammar(testRules);
}

void Viewer::run() {
  while (window_.isOpen()) {
    handleEvents();
    update();
    render();
  }
}

void Viewer::setGrammar(
    const std::unordered_map<std::string, ASTNodePtr> &rules) {
  rules_ = rules;
}

void Viewer::handleEvents() {
  window_.handleEvents(
      [&](const sf::Event::Closed &) { window_.close(); },
      [&](const sf::Event::MouseButtonPressed &e) {
        if (e.button == sf::Mouse::Button::Left) {
          isDragging_ = true;
          lastMousePos_ = sf::Vector2f(static_cast<float>(e.position.x),
                                       static_cast<float>(e.position.y));
        }
      },
      [&](const sf::Event::MouseButtonReleased &e) {
        if (e.button == sf::Mouse::Button::Left) {
          isDragging_ = false;
        }
      },
      [&](const sf::Event::MouseMoved &e) {
        if (isDragging_) {
          sf::Vector2f currentPos(static_cast<float>(e.position.x),
                                  static_cast<float>(e.position.y));
          offset_ += (currentPos - lastMousePos_) / zoom_;
          lastMousePos_ = currentPos;
        }
      },
      [&](const sf::Event::MouseWheelScrolled &e) {
        if (e.delta > 0) {
          zoom_ *= 1.1f;
        } else {
          zoom_ /= 1.1f;
        }
      });
}

void Viewer::update() {
  // Update logic here if needed
}

void Viewer::render() {
  window_.clear(sf::Color(30, 30, 30));

  float startX = window_.getSize().x / 2.0f;
  float startY = 50.0f;

  for (const auto &[name, rule] : rules_) {
    drawNode(rule, startX, startY, 0);
    startY += 200.0f;
  }

  window_.display();
}

void Viewer::drawNode(const ASTNodePtr &node, float x, float y, float level) {
  if (!node)
    return;

  // Draw node
  sf::RectangleShape shape(sf::Vector2f(100, 40));
  shape.setPosition({x - 50, y - 20});
  shape.setFillColor(sf::Color(60, 60, 60));
  shape.setOutlineThickness(2);
  shape.setOutlineColor(sf::Color::White);
  window_.draw(shape);

  // Draw node text
  drawText(node->getValue(), x, y);

  // Draw children
  float childX = x;
  float childY = y + 80;
  float childSpacing = 150.0f;
  float totalWidth = childSpacing * (node->getChildren().size() - 1);
  float startX = x - totalWidth / 2;

  for (size_t i = 0; i < node->getChildren().size(); ++i) {
    float childX = startX + i * childSpacing;
    drawConnection(x, y + 20, childX, childY - 20);
    drawNode(node->getChildren()[i], childX, childY, level + 1);
  }
}

void Viewer::drawConnection(float x1, float y1, float x2, float y2) {
  sf::Vertex line[] = {{{x1, y1}, sf::Color::White},
                       {{x2, y2}, sf::Color::White}};
  window_.draw(line, 2, sf::PrimitiveType::Lines);
}

void Viewer::drawText(const std::string &text, float x, float y,
                      const sf::Color &color) {
  sf::Text sfText(font_, text);
  sfText.setCharacterSize(14);
  sfText.setFillColor(color);
  sf::FloatRect bounds = sfText.getLocalBounds();
  sfText.setPosition({x - bounds.size.x / 2, y - bounds.size.y / 2});
  window_.draw(sfText);
}

} // namespace ebnf