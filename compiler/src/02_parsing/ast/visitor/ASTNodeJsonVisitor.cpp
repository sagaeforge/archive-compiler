#include "02_parsing/ast/visitor/ASTNodeJsonVisitor.h"

namespace nugdev::compiler::ast {

ASTNodeJsonVisitor::ASTNodeJsonVisitor() : m_document() {}

lib::String ASTNodeJsonVisitor::to_str(const json::JsonValue &value) {
    json::JsonStringBuffer buffer;
    json::JsonFormatter formatter(buffer);
    m_document.SetObject();
    m_document.CopyFrom(value, get_allocator());
    m_document.Accept(formatter);
    return lib::String(buffer.GetString());
}

json::JsonAllocator &ASTNodeJsonVisitor::get_allocator() { return m_document.GetAllocator(); }

json::JsonValue ASTNodeJsonVisitor::visit_program(const NodePtr<ast::module::ProgramNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Program"), get_allocator());
    value.AddMember("statements", json::JsonValue(json::Type::kArrayType), get_allocator());

    for (const auto &statement : node->get_statements()) {
        auto json = statement->accept<json::JsonValue>(self());
        value["statements"].PushBack(json, get_allocator());
    }

    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_block_statement(const NodePtr<ast::statement::BlockStatementNode> &node) {
    json::JsonValue json(json::Type::kObjectType);
    json.AddMember("type", json::JsonValue("BlockStatement"), get_allocator());
    json.AddMember("statements", json::JsonValue(json::Type::kArrayType), get_allocator());

    for (auto &statement : node->get_statements()) {
        json["statements"].PushBack(statement->accept<json::JsonValue>(self()), get_allocator());
    }

    return json;
}

json::JsonValue ASTNodeJsonVisitor::visit_break_statement(const NodePtr<ast::statement::BreakStatementNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Break"), get_allocator());
    value.AddMember("label", node->get_label() != nullptr ? node->get_label()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_continue_statement(const NodePtr<ast::statement::ContinueStatementNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Continue"), get_allocator());
    value.AddMember("label", node->get_label() != nullptr ? node->get_label()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_expression_statement(const NodePtr<ast::statement::ExpressionStatementNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Expression"), get_allocator());
    value.AddMember("expression", node->get_expression()->accept<json::JsonValue>(self()), get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_for_statement(const NodePtr<ast::statement::ForStatementNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("For"), get_allocator());
    value.AddMember("label", node->get_label() != nullptr ? node->get_label()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    value.AddMember("init", node->get_init() != nullptr ? node->get_init()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    value.AddMember("condition",
                    node->get_condition() != nullptr ? node->get_condition()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    value.AddMember("post", node->get_post() != nullptr ? node->get_post()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    value.AddMember("consequence",
                    node->get_consequence() != nullptr ? node->get_consequence()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_let_statement(const NodePtr<ast::statement::LetStatementNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Let"), get_allocator());
    value.AddMember("name", node->get_name()->accept<json::JsonValue>(self()), get_allocator());
    value.AddMember("type_expression", node->get_type() != nullptr ? node->get_type()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    value.AddMember("value", node->get_value() != nullptr ? node->get_value()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_return_statement(const NodePtr<ast::statement::ReturnStatementNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Return"), get_allocator());
    value.AddMember("label", node->get_label() != nullptr ? node->get_label()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    value.AddMember("value", node->get_value() != nullptr ? node->get_value()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_array_literal_expression(const NodePtr<ast::expression::ArrayLiteralNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("ArrayLiteral"), get_allocator());
    value.AddMember("elements", json::JsonValue(json::Type::kArrayType), get_allocator());

    for (const auto &element : node->get_elements()) {
        value["elements"].PushBack(element->accept<json::JsonValue>(self()), get_allocator());
    }

    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_boolean_literal_expression(const NodePtr<ast::expression::BooleanLiteralNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("BooleanLiteral"), get_allocator());
    value.AddMember("value", node->get_value(), get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_call_expression(const NodePtr<ast::expression::CallExpressionNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("CallExpression"), get_allocator());
    value.AddMember("callee", node->get_callee()->accept<json::JsonValue>(self()), get_allocator());
    value.AddMember("arguments", json::JsonValue(json::Type::kArrayType), get_allocator());

    for (const auto &argument : node->get_arguments()) {
        value["arguments"].PushBack(argument->accept<json::JsonValue>(self()), get_allocator());
    }

    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_function_expression(const NodePtr<ast::expression::FunctionExpressionNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("FunctionLiteral"), get_allocator());
    value.AddMember("parameters", json::JsonValue(json::Type::kArrayType), get_allocator());
    value.AddMember("body", node->get_body()->accept<json::JsonValue>(self()), get_allocator());

    for (const auto &parameter : node->get_parameters()) {
        const auto &[name, type, defaultValue] = parameter;

        json::JsonValue parameterValue(json::Type::kObjectType);
        parameterValue.AddMember("name", name->accept<json::JsonValue>(self()), get_allocator());
        parameterValue.AddMember("type", type->accept<json::JsonValue>(self()), get_allocator());
        parameterValue.AddMember("defaultValue", defaultValue ? defaultValue->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                                 get_allocator());
        value["parameters"].PushBack(parameterValue, get_allocator());
    }

    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_identifier_literal_expression(const NodePtr<ast::expression::IdentifierLiteralNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("IdentifierLiteral"), get_allocator());

    std::string value_str;
    node->get_value().toUTF8String(value_str);
    json::JsonValue value_json(json::Type::kStringType);
    value_json.SetString(value_str.c_str(), get_allocator());
    value.AddMember("value", value_json, get_allocator());

    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_if_expression(const NodePtr<ast::expression::IfExpressionNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("IfExpression"), get_allocator());
    value.AddMember("condition", node->get_condition()->accept<json::JsonValue>(self()), get_allocator());
    value.AddMember("consequence", node->get_consequence()->accept<json::JsonValue>(self()), get_allocator());
    value.AddMember("alternative", node->get_alternative() ? node->get_alternative()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_index_expression(const NodePtr<ast::expression::IndexExpressionNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("IndexExpression"), get_allocator());
    value.AddMember("left", node->get_left()->accept<json::JsonValue>(self()), get_allocator());
    value.AddMember("index", node->get_index()->accept<json::JsonValue>(self()), get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_infix_expression(const NodePtr<ast::expression::InfixExpressionNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("InfixExpression"), get_allocator());
    value.AddMember("left", node->get_left()->accept<json::JsonValue>(self()), get_allocator());
    std::string operator_str;
    node->get_operator().toUTF8String(operator_str);
    json::JsonValue operator_json(json::Type::kStringType);
    operator_json.SetString(operator_str.c_str(), get_allocator());
    value.AddMember("operator", operator_json, get_allocator());
    value.AddMember("right", node->get_right()->accept<json::JsonValue>(self()), get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_number_literal_expression(const NodePtr<ast::expression::NumberLiteralNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("NumberLiteral"), get_allocator());
    std::string value_str;
    node->get_value().toUTF8String(value_str);
    json::JsonValue value_json(json::Type::kStringType);
    value_json.SetString(value_str.c_str(), get_allocator());
    value.AddMember("value", value_json, get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_postfix_expression(const NodePtr<ast::expression::PostExpressionNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("PostfixExpression"), get_allocator());
    value.AddMember("left", node->get_left()->accept<json::JsonValue>(self()), get_allocator());
    std::string operator_str;
    node->get_operator().toUTF8String(operator_str);
    json::JsonValue operator_json(json::Type::kStringType);
    operator_json.SetString(operator_str.c_str(), get_allocator());
    value.AddMember("operator", operator_json, get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_prefix_expression(const NodePtr<ast::expression::PrefixExpressionNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("PrefixExpression"), get_allocator());
    std::string operator_str;
    node->get_operator().toUTF8String(operator_str);
    json::JsonValue operator_json(json::Type::kStringType);
    operator_json.SetString(operator_str.c_str(), get_allocator());
    value.AddMember("operator", operator_json, get_allocator());
    value.AddMember("right", node->get_right()->accept<json::JsonValue>(self()), get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_string_literal_expression(const NodePtr<ast::expression::StringLiteralNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("StringLiteral"), get_allocator());
    std::string value_str;
    node->get_value().toUTF8String(value_str);
    json::JsonValue value_json(json::Type::kStringType);
    value_json.SetString(value_str.c_str(), get_allocator());
    value.AddMember("value", value_json, get_allocator());
    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_type_literal_expression(const NodePtr<ast::expression::TypeLiteralNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("TypeLiteral"), get_allocator());

    auto [type, size] = node->get_meta();
    value.AddMember("type-name", json::JsonValue(static_cast<int>(type)), get_allocator());
    value.AddMember("size", json::JsonValue(static_cast<unsigned int>(size)), get_allocator());

    return value;
}

json::JsonValue ASTNodeJsonVisitor::visit_when_expression(const NodePtr<ast::expression::WhenExpressionNode> &node) {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("When"), get_allocator());
    value.AddMember("target", node->get_target() != nullptr ? node->get_target()->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());

    json::JsonValue conditionsArray(json::Type::kArrayType);
    for (const auto &[condition, consequence] : node->get_conditions()) {
        json::JsonValue conditionObj(json::Type::kObjectType);
        conditionObj.AddMember("condition", condition->accept<json::JsonValue>(self()), get_allocator());

        if (std::holds_alternative<std::shared_ptr<Expression>>(consequence)) {
            conditionObj.AddMember("consequence", std::get<std::shared_ptr<Expression>>(consequence)->accept<json::JsonValue>(self()), get_allocator());
        } else if (std::holds_alternative<std::shared_ptr<Statement>>(consequence)) {
            conditionObj.AddMember("consequence", std::get<std::shared_ptr<Statement>>(consequence)->accept<json::JsonValue>(self()), get_allocator());
        }

        conditionsArray.PushBack(conditionObj, get_allocator());
    }

    value.AddMember("conditions", conditionsArray, get_allocator());

    std::shared_ptr<ast::ASTNode> alternative = nullptr;
    if (std::holds_alternative<std::shared_ptr<Expression>>(node->get_alternative())) {
        alternative = std::get<std::shared_ptr<Expression>>(node->get_alternative());
    } else if (std::holds_alternative<std::shared_ptr<Statement>>(node->get_alternative())) {
        alternative = std::get<std::shared_ptr<Statement>>(node->get_alternative());
    }

    value.AddMember("alternative", alternative != nullptr ? alternative->accept<json::JsonValue>(self()) : json::JsonValue(json::Type::kNullType),
                    get_allocator());

    return value;
}

} // namespace nugdev::compiler::ast
