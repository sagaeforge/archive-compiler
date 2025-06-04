#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class ParametersTypesTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// Parameter Tests
TEST_F(ParametersTypesTest, ParameterBasic) {
  auto type = std::make_unique<SimpleType>("int");
  auto param = std::make_unique<Parameter>(Parameter::Mutability::LET, "x",
                                           std::move(type));

  ASSERT_NE(param, nullptr);
  EXPECT_EQ(param->get_node_type(), NodeType::PARAMETER);
  EXPECT_EQ(param->get_mutability(), Parameter::Mutability::LET);
  EXPECT_EQ(param->get_parameter_name(), "x");
  EXPECT_FALSE(param->has_default_value());
}

TEST_F(ParametersTypesTest, ParameterMutable) {
  auto type = std::make_unique<SimpleType>("string");
  auto param = std::make_unique<Parameter>(Parameter::Mutability::MUT,
                                           "message", std::move(type));

  EXPECT_EQ(param->get_mutability(), Parameter::Mutability::MUT);
  EXPECT_EQ(param->get_parameter_name(), "message");
}

TEST_F(ParametersTypesTest, ParameterWithDefaultValue) {
  auto type = std::make_unique<SimpleType>("bool");
  auto defaultValue = std::make_unique<BooleanLiteral>(true);
  auto param =
      std::make_unique<Parameter>(Parameter::Mutability::LET, "flag",
                                  std::move(type), std::move(defaultValue));

  EXPECT_TRUE(param->has_default_value());
  EXPECT_EQ(param->get_default_value()->get_node_type(),
            NodeType::BOOLEAN_LITERAL);
}

// SimpleType Tests
TEST_F(ParametersTypesTest, SimpleTypeBasic) {
  auto type = std::make_unique<SimpleType>("string");

  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->get_type_name(), "string");
  EXPECT_EQ(type->to_string(), "SimpleType(string)");
}

TEST_F(ParametersTypesTest, SimpleTypePrimitive) {
  auto intType = std::make_unique<SimpleType>("int");
  auto floatType = std::make_unique<SimpleType>("float");
  auto boolType = std::make_unique<SimpleType>("bool");

  EXPECT_EQ(intType->get_type_name(), "int");
  EXPECT_EQ(floatType->get_type_name(), "float");
  EXPECT_EQ(boolType->get_type_name(), "bool");
}

// FunctionType Tests
TEST_F(ParametersTypesTest, FunctionTypeBasic) {
  std::vector<std::unique_ptr<TypeLiteral>> paramTypes;
  paramTypes.push_back(std::make_unique<SimpleType>("int"));
  paramTypes.push_back(std::make_unique<SimpleType>("string"));

  auto returnType = std::make_unique<SimpleType>("bool");
  auto funcType = std::make_unique<FunctionType>(std::move(paramTypes),
                                                 std::move(returnType));

  ASSERT_NE(funcType, nullptr);
  EXPECT_EQ(funcType->get_node_type(), NodeType::FUNCTION_TYPE);
  EXPECT_EQ(funcType->get_parameter_types().size(), 2);
}

TEST_F(ParametersTypesTest, FunctionTypeNoParameters) {
  std::vector<std::unique_ptr<TypeLiteral>> paramTypes; // Empty
  auto returnType = std::make_unique<SimpleType>("int");
  auto funcType = std::make_unique<FunctionType>(std::move(paramTypes),
                                                 std::move(returnType));

  EXPECT_EQ(funcType->get_parameter_types().size(), 0);
  EXPECT_TRUE(funcType->get_parameter_types().empty());
}

// OptionalType Tests
TEST_F(ParametersTypesTest, OptionalTypeBasic) {
  auto innerType = std::make_unique<SimpleType>("int");
  auto optionalType = std::make_unique<OptionalType>(std::move(innerType));

  ASSERT_NE(optionalType, nullptr);
  EXPECT_EQ(optionalType->get_node_type(), NodeType::OPTIONAL_TYPE);
  EXPECT_EQ(optionalType->to_string(), "OptionalType(SimpleType(int))");
}

TEST_F(ParametersTypesTest, OptionalTypeNested) {
  auto baseType = std::make_unique<SimpleType>("string");
  auto firstOptional = std::make_unique<OptionalType>(std::move(baseType));
  auto secondOptional =
      std::make_unique<OptionalType>(std::move(firstOptional));

  EXPECT_EQ(secondOptional->get_inner_type().get_node_type(),
            NodeType::OPTIONAL_TYPE);
}

// TupleType Tests
TEST_F(ParametersTypesTest, TupleTypeBasic) {
  std::vector<std::unique_ptr<TypeLiteral>> elementTypes;
  elementTypes.push_back(std::make_unique<SimpleType>("int"));
  elementTypes.push_back(std::make_unique<SimpleType>("string"));

  auto tupleType = std::make_unique<TupleType>(std::move(elementTypes));

  ASSERT_NE(tupleType, nullptr);
  EXPECT_EQ(tupleType->get_node_type(), NodeType::TUPLE_TYPE);
  EXPECT_EQ(tupleType->get_element_count(), 2);
  EXPECT_EQ(tupleType->get_element_types().size(), 2);
}

// StructField Tests
TEST_F(ParametersTypesTest, StructFieldBasic) {
  auto type = std::make_unique<SimpleType>("int");
  auto field = std::make_unique<StructField>("age", std::move(type));

  ASSERT_NE(field, nullptr);
  EXPECT_EQ(field->get_node_type(), NodeType::STRUCT_FIELD);
  EXPECT_EQ(field->get_field_name(), "age");
}

TEST_F(ParametersTypesTest, StructFieldWithDefaultValue) {
  auto type = std::make_unique<SimpleType>("string");
  auto defaultValue = std::make_unique<StringLiteral>(
      "hello", StringLiteral::StringType::SIMPLE);
  auto field = std::make_unique<StructField>("name", std::move(type),
                                             std::move(defaultValue));

  EXPECT_TRUE(field->has_default_value());
  EXPECT_EQ(field->get_default_value()->get_node_type(),
            NodeType::STRING_LITERAL);
}

// ArgumentList Tests
TEST_F(ParametersTypesTest, ArgumentListBasic) {
  auto argList = std::make_unique<ArgumentList>();

  ASSERT_NE(argList, nullptr);
  EXPECT_EQ(argList->get_node_type(), NodeType::ARGUMENT_LIST);
  EXPECT_EQ(argList->get_argument_count(), 0);
  EXPECT_TRUE(argList->get_arguments().empty());
}

TEST_F(ParametersTypesTest, ArgumentListWithArguments) {
  auto argList = std::make_unique<ArgumentList>();

  auto arg1 = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto arg2 = std::make_unique<StringLiteral>(
      "hello", StringLiteral::StringType::SIMPLE);
  auto arg3 = std::make_unique<BooleanLiteral>(true);

  argList->add_argument(std::move(arg1));
  argList->add_argument(std::move(arg2));
  argList->add_argument(std::move(arg3));

  EXPECT_EQ(argList->get_argument_count(), 3);
  EXPECT_EQ(argList->get_arguments().size(), 3);
}

// Complex Type Combinations
TEST_F(ParametersTypesTest, ComplexTypeNesting) {
  // Optional<Tuple<int, string>>
  std::vector<std::unique_ptr<TypeLiteral>> tupleElements;
  tupleElements.push_back(std::make_unique<SimpleType>("int"));
  tupleElements.push_back(std::make_unique<SimpleType>("string"));
  auto tupleType = std::make_unique<TupleType>(std::move(tupleElements));

  auto optionalType = std::make_unique<OptionalType>(std::move(tupleType));

  EXPECT_EQ(optionalType->get_inner_type().get_node_type(),
            NodeType::TUPLE_TYPE);
}

// Visitor Pattern Tests
TEST_F(ParametersTypesTest, ParametersTypesAcceptVisitor) {
  class TestVisitor : public DefaultASTVisitor {
  public:
    int parameterCount = 0;
    int functionTypeCount = 0;
    int optionalTypeCount = 0;
    int tupleTypeCount = 0;
    int structFieldCount = 0;
    int argumentListCount = 0;

    void visit(Parameter &) override { parameterCount++; }
    void visit(FunctionType &) override { functionTypeCount++; }
    void visit(OptionalType &) override { optionalTypeCount++; }
    void visit(TupleType &) override { tupleTypeCount++; }
    void visit(StructField &) override { structFieldCount++; }
    void visit(ArgumentList &) override { argumentListCount++; }
  };

  TestVisitor visitor;

  // Test each type
  auto type = std::make_unique<SimpleType>("int");
  auto param = std::make_unique<Parameter>(Parameter::Mutability::LET, "x",
                                           std::move(type));
  param->accept(visitor);

  std::vector<std::unique_ptr<TypeLiteral>> paramTypes;
  auto returnType = std::make_unique<SimpleType>("void");
  auto funcType = std::make_unique<FunctionType>(std::move(paramTypes),
                                                 std::move(returnType));
  funcType->accept(visitor);

  auto innerType = std::make_unique<SimpleType>("bool");
  auto optionalType = std::make_unique<OptionalType>(std::move(innerType));
  optionalType->accept(visitor);

  std::vector<std::unique_ptr<TypeLiteral>> elements;
  elements.push_back(std::make_unique<SimpleType>("int"));
  elements.push_back(std::make_unique<SimpleType>("string"));
  auto tupleType = std::make_unique<TupleType>(std::move(elements));
  tupleType->accept(visitor);

  auto fieldType = std::make_unique<SimpleType>("double");
  auto structField =
      std::make_unique<StructField>("value", std::move(fieldType));
  structField->accept(visitor);

  auto argList = std::make_unique<ArgumentList>();
  argList->accept(visitor);

  EXPECT_EQ(visitor.parameterCount, 1);
  EXPECT_EQ(visitor.functionTypeCount, 1);
  EXPECT_EQ(visitor.optionalTypeCount, 1);
  EXPECT_EQ(visitor.tupleTypeCount, 1);
  EXPECT_EQ(visitor.structFieldCount, 1);
  EXPECT_EQ(visitor.argumentListCount, 1);
}