//
// Created by lambda on 11/22/25.
//

#include "compiler.h"

#include <sstream>

#include "instruction.h"
#include "02_parsing/ast/expression/identifier_expression.h"
#include "02_parsing/ast/expression/infix_expression.h"
#include "02_parsing/ast/expression/number_expression.h"
#include "02_parsing/ast/expression/string_expression.h"
#include "02_parsing/ast/expression/call_expression.h"
#include "02_parsing/ast/expression/function_expression.h"
#include "02_parsing/ast/expression/if_expression.h"
#include "02_parsing/ast/expression/prefix_expression.h"
#include "02_parsing/ast/module/program.h"
#include "02_parsing/ast/statement/block_statement.h"
#include "02_parsing/ast/statement/expression_statement.h"
#include "02_parsing/ast/statement/return_statement.h"
#include "02_parsing/ast/statement/variable_statement.h"

Compiler::Compiler() : m_registerAllocator(*this) {
}

void Compiler::visit(const Node<const BlockStatement> &node) {
    for (const auto &stmt: node->statements()) {
        stmt->accept(*this);
    }
}

void Compiler::visit(const Node<const CallExpression> &node) {
    auto name = node->callee()->as<IdentifierExpression>()->value();

    std::vector<Register> argRegs = {
        Register("rdi"),
        Register("rsi"),
        Register("rdx"),
        Register("rcx"),
        Register("r8"),
        Register("r9"),
    };

    std::vector<Register> argValues;
    for (const auto &arg: node->args()) {
        arg->accept(*this);
        argValues.push_back(takeResultRegister());
    }

    auto saved = m_registerAllocator.saveCallerSaved(argValues);
    for (size_t i = 0; i < argValues.size() && i < argRegs.size(); ++i) {
        if (argValues[i] != argRegs[i]) {
            emit(std::make_shared<Move>(
                std::make_shared<Register>(argRegs[i]),
                std::make_shared<Register>(argValues[i])
            ));
        }
        m_registerAllocator.free(argValues[i]);
    }

    // 함수 호출
    emit(std::make_shared<Call>(name));

    // 레지스터 복원
    m_registerAllocator.restoreCallerSaved(saved);

    // 반환값은 rax에
    setResultRegister(Register("rax"));
}

void Compiler::visit(const Node<const InfixExpression> &node) {
    const auto op = node->opcode();

    node->left()->accept(*this);
    auto leftReg = takeResultRegister();

    const auto leftIsCall = node->left()->is<CallExpression>();
    const auto rightIsCall = node->right()->is<CallExpression>();

    if (leftIsCall || rightIsCall) {
        // 왼쪽 결과를 스택에 임시 저장
        emit(std::make_shared<Push>(std::make_shared<Register>(leftReg)));
        m_registerAllocator.free(leftReg);

        // 오른쪽 평가
        node->right()->accept(*this);
        auto rightReg = takeResultRegister();

        emit(std::make_shared<Pop>(std::make_shared<Register>("rcx")));

        if (op == "+") {
            emit(std::make_shared<Add>(
                std::make_shared<Register>("rcx"),
                std::make_shared<Register>(rightReg)
            ));
            m_registerAllocator.free(rightReg);
            setResultRegister(Register("rcx"));
        } else if (op == "-") {
            emit(std::make_shared<Sub>(
                std::make_shared<Register>("rcx"),
                std::make_shared<Register>(rightReg)
            ));
            m_registerAllocator.free(rightReg);
            setResultRegister(Register("rcx"));
        } else if (op == "*") {
            emit(std::make_shared<Mul>(
                std::make_shared<Register>("rcx"),
                std::make_shared<Register>(rightReg)
            ));
            m_registerAllocator.free(rightReg);
            setResultRegister(Register("rcx"));
        } else {
            // 나눗셈 처리가 조금 난감하네.
            throw std::runtime_error("unsupported op: " + op);
        }
    } else {
        // 일반 연산
        node->right()->accept(*this);
        auto rightReg = takeResultRegister();

        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            emitArithmeticOperate(op, leftReg, rightReg);
            m_registerAllocator.free(rightReg);
            setResultRegister(leftReg);
        } else if (op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=") {
            emitComparisonOperate(op, leftReg, rightReg);
            m_registerAllocator.free(rightReg);
            setResultRegister(leftReg);
        } else {
            throw std::runtime_error("Unknown operator: " + op);
        }
    }
}

void Compiler::visit(const Node<const FunctionExpression> &node) {
    const auto name = node->name()->value();

    emitComment("=== Function: " + name + " ===");
    emitLabel(name);

    // 함수 프롤로그
    emit(std::make_shared<Push>(std::make_shared<Register>("rbp")));
    emit(std::make_shared<Move>(
        std::make_shared<Register>("rbp"),
        std::make_shared<Register>("rsp")
    ));

    // 매개변수 처리.
    std::vector<std::string> paramRegs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    m_localVariables.clear();
    m_stackOffset = 0;

    for (size_t i = 0; i < node->parameters().size(); ++i) {
        auto paramName = node->parameters()[i]->as<VariableStatement>()->name()->value();
        if (i < paramRegs.size()) {
            m_stackOffset += 8;
            emit(std::make_shared<Move>(
                std::make_shared<Memory>(
                    std::make_shared<Register>("rbp"),
                    m_stackOffset // 음수 오프셋
                ),
                std::make_shared<Register>(paramRegs[i])
            ));
            emitComment("param: " + paramName);
            m_localVariables[paramName] = m_stackOffset;
        }
    }

    // 스택 공간 예약
    auto reserved = std::max(64u, ((m_stackOffset + 63u) / 16u) * 16u);
    emit(std::make_shared<Sub>(
        std::make_shared<Register>("rsp"),
        std::make_shared<Immutable>(reserved)
    ));
    emitComment(" reserve stack space");

    node->body()->accept(*this);

    // 함수 에필로그 (return이 없는 경우)
    emit(std::make_shared<Move>(
        std::make_shared<Register>("rsp"),
        std::make_shared<Register>("rbp")
    ));
    emit(std::make_shared<Pop>(std::make_shared<Register>("rbp")));
    emit(std::make_shared<Ret>());
    emit(nullptr);
}

void Compiler::visit(const Node<const PrefixExpression> &node) {
    const auto op = node->value();

    node->right()->accept(*this);
    auto reg = getResultRegister();

    if (op == "-") {
        emit(std::make_shared<Neg>(std::make_shared<Register>(reg)));
        emitComment("unary minus");
    } else if (op == "!") {
        emit(std::make_shared<Test>(
            std::make_shared<Register>(reg),
            std::make_shared<Register>(reg)
        ));
        emit(std::make_shared<Setz>(std::make_shared<Register>("al")));
        emitComment("logical NOT");
        emit(std::make_shared<Movzx>(
            std::make_shared<Register>(reg),
            std::make_shared<Register>("al")
        ));
    } else {
        throw std::runtime_error("Unknown operator: " + op);
    }
}

void Compiler::visit(const Node<const Program> &node) {
    emitComment("=== Program Start ===");

    std::vector<Node<Expression> > topLevelExpressions;

    // 함수 정의와 최상위 표현식 분리
    for (const auto &stmt: node->statements()) {
        if (stmt->is<ExpressionStatement>()) {
            auto exprStmt = stmt->as<ExpressionStatement>();
            // 함수 정의가 아닌 표현식
            if (!exprStmt->expression()->is<FunctionExpression>()) {
                topLevelExpressions.push_back(exprStmt->expression());
                continue;
            }
        }
        // 함수 정의 처리
        stmt->accept(*this);
    }

    // 최상위 표현식이 있으면 _start 생성
    if (!topLevelExpressions.empty()) {
        emitLabel("_start");
        for (const auto &expr: topLevelExpressions) {
            expr->accept(*this);
        }

        // 마지막 결과를 exit code로
        if (m_currentResultRegister.has_value()) {
            auto resultReg = takeResultRegister();
            if (resultReg != Register("rax")) {
                emit(std::make_shared<Move>(
                    std::make_shared<Register>("rax"),
                    std::make_shared<Register>(resultReg)
                ));
            }
            emit(std::make_shared<Move>(
                std::make_shared<Register>("rdi"),
                std::make_shared<Register>("rax")
            ));
        } else {
            emit(std::make_shared<Xor>(
                std::make_shared<Register>("rdi"),
                std::make_shared<Register>("rdi")
            ));
        }

#ifdef __APPLE__
        emit(std::make_shared<Move>(
            std::make_shared<Register>("rax"),
            std::make_shared<Immutable>(0x2000001) // ✅ macOS sys_exit
        ));
#else
        emit(std::make_shared<Move>(
            std::make_shared<Register>("rax"),
            std::make_shared<Immutable>(60) // Linux sys_exit
        ));
#endif
        emit(std::make_shared<Syscall>());
    }
}

void Compiler::visit(const Node<const IfExpression> &node) {
    std::string else_label = newLabel("else");
    std::string end_label = newLabel("endif");

    node->condition()->accept(*this);
    auto conditionReg = getResultRegister();

    // 조건 평가
    emit(std::make_shared<Cmp>(
        std::make_shared<Register>(conditionReg),
        std::make_shared<Immutable>(0)
    ));
    m_registerAllocator.free(conditionReg);

    // 거짓이면 점프
    if (node->alternative() != nullptr) {
        emit(std::make_shared<Je>(else_label));
    } else {
        emit(std::make_shared<Je>(end_label));
    }

    // then 블록
    node->consequence()->accept(*this);

    // else 블록
    if (node->alternative() != nullptr) {
        emit(std::make_shared<Jmp>(end_label));
        emitLabel(else_label);
        node->alternative()->accept(*this);
    }

    emitLabel(end_label);
}

void Compiler::visit(const Node<const StringExpression> &node) {
    auto content = node->value();

    string_t label;
    auto it = m_stringLiterals.find(content);
    if (it != m_stringLiterals.end()) {
        label = it->second;
    } else {
        static std::uint64_t m_stringLabelCounter = 0;
        label = "str_" + std::to_string(m_stringLabelCounter++);
        m_stringLiterals[content] = label;
    }

    auto reg = m_registerAllocator.allocate();
    emit(std::make_shared<Lea>(
        std::make_shared<Register>(reg),
        std::make_shared<Memory>(label)
    ));
    emitComment("load string address");
}

void Compiler::visit(const Node<const NumberExpression> &node) {
    auto reg = m_registerAllocator.allocate();
    emit(std::make_shared<Move>(
        std::make_shared<Register>(reg),
        std::make_shared<Immutable>(node->asUInt())
    ));
    setResultRegister(reg);
}

void Compiler::visit(const Node<const ExpressionStatement> &node) {
    node->expression()->accept(*this);

    if (m_currentResultRegister.has_value()) {
        m_registerAllocator.free(m_currentResultRegister.value());
        m_currentResultRegister.reset();
    }
}

void Compiler::visit(const Node<const TypeExpression> &node) {
}

void Compiler::visit(const Node<const VariableStatement> &node) {
    auto varName = node->name()->value();

    emitComment("var " + varName);

    if (node->value() != nullptr) {
        node->value()->accept(*this);
        auto valueReg = takeResultRegister();

        m_stackOffset += 8;
        m_localVariables[varName] = m_stackOffset;

        emit(std::make_shared<Move>(
            std::make_shared<Memory>(std::make_shared<Register>("rbp"), m_stackOffset),
            std::make_shared<Register>(valueReg)
        ));
        m_registerAllocator.free(valueReg);
    } else {
        m_stackOffset += 8;
        m_localVariables[varName] = m_stackOffset;
        emit(std::make_shared<Move>(
            "qword",
            std::make_shared<Memory>(
                std::make_shared<Register>("rbp"),
                static_cast<std::uint32_t>(m_stackOffset)
            ),
            std::make_shared<Immutable>(0)
        ));
    }
}

void Compiler::visit(const Node<const ReturnStatement> &node) {
    if (node->value() != nullptr) {
        node->value()->accept(*this);
        auto reg = takeResultRegister();

        if (reg != Register("rax")) {
            emit(std::make_shared<Move>(
                std::make_shared<Register>("rax"),
                std::make_shared<Register>(reg)
            ));
            m_registerAllocator.free(reg);
        }
    }

    // 함수 에필로그
    emit(std::make_shared<Move>(
        std::make_shared<Register>("rsp"),
        std::make_shared<Register>("rbp")
    ));
    emit(std::make_shared<Pop>(std::make_shared<Register>("rbp")));
    emit(std::make_shared<Ret>());
}

void Compiler::visit(const Node<const IdentifierExpression> &node) {
    auto variableName = node->value();

    if (!m_localVariables.contains(variableName)) {
        throw std::runtime_error("Undefined variable: " + variableName);
    }

    auto offset = m_localVariables[variableName];
    auto reg = m_registerAllocator.allocate();
    emit(std::make_shared<Move>(
        std::make_shared<Register>(reg),
        std::make_shared<Memory>(std::make_shared<Register>("rbp"), offset)
    ));
    emitComment("load " + variableName);
    setResultRegister(reg);
}

void Compiler::emitComment(const string_t &comment) {
    if (m_results.empty()) {
        m_results.push_back(Result{.comment = comment});
        return;
    }

    auto last = m_results.back();
    last.comment = comment;
    m_results.pop_back();
    m_results.push_back(last);
}

void Compiler::emitLabel(const string_t &label) {
    m_results.push_back(Result{.label = label});
}

void Compiler::emit(std::shared_ptr<Instruction> instruction) {
    m_results.push_back(Result{.instruction = std::move(instruction)});
}

string_t Compiler::generate() {
    std::stringstream ss;
    const string_t tab = "    ";

    ss << "section .data" << std::endl;

    for (const auto &[content, label]: m_stringLiterals) {
        ss << label << ": db \"" << content << "\", 0" << std::endl;
    }

    ss << std::endl << "section .text" << std::endl;
    ss << "global _start" << std::endl << std::endl;

    // 프로그램 시작점
    for (const auto &result: m_results) {
        if (result.label.has_value()) {
            ss << result.label.value() << ": ";
        } else if (result.instruction != nullptr) {
            ss << tab << result.instruction;
        }

        if (result.label.has_value()) {
            ss << "; " << result.label.value();
        }

        ss << std::endl;
    }

    return ss.str();
}

void Compiler::setResultRegister(const Register &reg) {
    m_currentResultRegister = reg;
}

Register Compiler::getResultRegister() {
    if (m_currentResultRegister.has_value()) {
        return m_currentResultRegister.value();
    }

    throw std::runtime_error("Can't find register");
}

Register Compiler::takeResultRegister() {
    auto reg = getResultRegister();
    m_currentResultRegister.reset();
    return reg;
}

void Compiler::emitArithmeticOperate(const string_t &op, const Register &leftReg, const Register &rightReg) {
    if (op == "+") {
        emit(std::make_shared<Add>(
            std::make_shared<Register>(leftReg),
            std::make_shared<Register>(rightReg)
        ));
    } else if (op == "-") {
        emit(std::make_shared<Sub>(
            std::make_shared<Register>(leftReg),
            std::make_shared<Register>(rightReg)
        ));
    } else if (op == "*") {
        emit(std::make_shared<Mul>(
            std::make_shared<Register>(leftReg),
            std::make_shared<Register>(rightReg)
        ));
    } else if (op == "/") {
        if (leftReg != Register("rax")) {
            emit(std::make_shared<Move>(
                std::make_shared<Register>("rax"),
                std::make_shared<Register>(rightReg)
            ));
        }
        emit(std::make_shared<Cqo>());
        emitComment("sign-extend rax to rdx:rax");
        emit(std::make_shared<IDiv>(std::make_shared<Register>(rightReg)));
        if (leftReg != Register("rax")) {
            emit(std::make_shared<Move>(
                std::make_shared<Register>(leftReg),
                std::make_shared<Register>("rax")
            ));
        }
    } else if (op == "%") {
        if (leftReg != Register("rax")) {
            emit(std::make_shared<Move>(
                std::make_shared<Register>("rax"),
                std::make_shared<Register>(rightReg)
            ));
        }
        emit(std::make_shared<Cqo>());
        emit(std::make_shared<IDiv>(std::make_shared<Register>(rightReg)));
        emit(std::make_shared<Move>(
            std::make_shared<Register>(leftReg),
            std::make_shared<Register>("rdx")
        ));
        emitComment("remainder");
    }
}

void Compiler::emitComparisonOperate(const string_t &op, const Register &leftReg, const Register &rightReg) {
    emit(std::make_shared<Cmp>(
        std::make_shared<Register>(leftReg),
        std::make_shared<Register>(rightReg)
    ));

    if (op == "==") {
        emit(std::make_shared<Sete>(std::make_shared<Register>("al")));
    } else if (op == "!=") {
        emit(std::make_shared<Setne>(std::make_shared<Register>("al")));
    } else if (op == ">") {
        emit(std::make_shared<Setg>(std::make_shared<Register>("al")));
    } else if (op == ">=") {
        emit(std::make_shared<Setge>(std::make_shared<Register>("al")));
    } else if (op == "<") {
        emit(std::make_shared<Setl>(std::make_shared<Register>("al")));
    } else if (op == "<=") {
        emit(std::make_shared<Setle>(std::make_shared<Register>("al")));
    }

    emit(std::make_shared<Movzx>(
        std::make_shared<Register>(leftReg),
        std::make_shared<Register>("al")
    ));
}

string_t Compiler::newLabel(const string_t &prefix) {
    static std::uint32_t m_labelCounter = 0;
    return prefix + std::to_string(m_labelCounter++);
}
