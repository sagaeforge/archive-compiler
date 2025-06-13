#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>


namespace ebnf {

enum class NodeType {
  Rule,
  Terminal,
  NonTerminal,
  Sequence,
  Alternative,
  Optional,
  Repetition,
  Group,
  Comment
};

class ASTNode {
public:
  ASTNode(NodeType type, const std::string &value = "");
  virtual ~ASTNode() = default;

  NodeType getType() const { return type_; }
  const std::string &getValue() const { return value_; }
  const std::vector<std::shared_ptr<ASTNode>> &getChildren() const {
    return children_;
  }

  void addChild(std::shared_ptr<ASTNode> child) { children_.push_back(child); }
  void setValue(const std::string &value) { value_ = value; }

private:
  NodeType type_;
  std::string value_;
  std::vector<std::shared_ptr<ASTNode>> children_;
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

} // namespace ebnf