//
// Created by nugde on 25. 10. 9..
//

#pragma once

#include "00_core/pointerable.h"
#include "01_tokenize/token/token.h"
#include "common.h"

class ASTVisitor;

class ASTNode : public pointerable<ASTNode>, public comparable<std::shared_ptr<ASTNode> > {
public:
    virtual void accept(ASTVisitor &visitor) const = 0;

    [[nodiscard]] virtual Token token() const = 0;
};

template<typename T>
using Node = std::shared_ptr<T>;

class Expression : public ASTNode {
};

class Module : public ASTNode {
};

class Statement : public ASTNode {
};
