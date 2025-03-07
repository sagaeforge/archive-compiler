```cpp
#include "BytecodeGenerator.h"

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNode.h"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNode.h"
#include "02_parsing/ast/expression/call/CallExpressionNode.h"
#include "02_parsing/ast/expression/function/FunctionExpressionNode.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNode.h"
#include "02_parsing/ast/expression/if/IfExpressionNode.h"
#include "02_parsing/ast/expression/index/IndexExpressionNode.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNode.h"
#include "02_parsing/ast/expression/number/NumberLiteralNode.h"
#include "02_parsing/ast/expression/post/PostExpressionNode.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNode.h"
#include "02_parsing/ast/expression/string/StringLiteralNode.h"
#include "02_parsing/ast/module/program/ProgramNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNode.h"
#include "02_parsing/ast/statement/break/BreakStatementNode.h"
#include "02_parsing/ast/statement/continue/ContinueStatementNode.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNode.h"
#include "02_parsing/ast/statement/for/ForStatementNode.h"
#include "02_parsing/ast/statement/let/LetStatementNode.h"
#include "02_parsing/ast/statement/return/ReturnStatementNode.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace nugdev::compiler::generation {

// 헬퍼 함수 선언
std::string opCodeToString(BytecodeOpCode opcode);
std::string operandToString(const BytecodeOperand &operand);

std::any BytecodeGenerator::visit_break_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // break 문장 처리
    // 현재 루프의 끝으로 점프하는 명령어 생성

    // 다운캐스팅을 통해 실제 break 문장 노드 접근
    auto breakNode = node->as<ast::statement::BreakStatementNode>();
    if (!breakNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 레이블 가져오기 (특정 루프로 점프하기 위한 레이블)
    const std::shared_ptr<ast::Expression> &labelExpr = breakNode->get_label();
    std::string targetLoopLabel;

    if (labelExpr) {
        // 레이블이 있는 경우
        auto idNode = labelExpr->as<ast::expression::IdentifierLiteralNode>();
        if (idNode) {
            icu::UnicodeString uniLabel = idNode->to_str();
            uniLabel.toUTF8String(targetLoopLabel);
        }
    }

    std::cerr << "Processing break statement" << (targetLoopLabel.empty() ? "" : " with label '" + targetLoopLabel + "'") << std::endl;

    // 대상 루프 컨텍스트 찾기
    LoopContext *targetLoop = findLoopContextByLabel(targetLoopLabel);

    // 대상 루프가 없으면 오류 처리
    if (!targetLoop) {
        int errorReg = allocateRegister();
        BytecodeInstruction errorInstr(BytecodeOpCode::LOAD_CONST);
        errorInstr.registers.push_back(errorReg);

        if (targetLoopLabel.empty()) {
            errorInstr.operands.push_back(std::string("error: break outside of loop"));
        } else {
            errorInstr.operands.push_back(std::string("error: break to unknown label '") + targetLoopLabel + "'");
        }

        addInstruction(errorInstr);
        return errorReg;
    }

    // 대상 루프의 끝 레이블 확인
    std::string endLabel = targetLoop->endLabel;
    std::cerr << "Break target label is '" << endLabel << "' for loop '" << targetLoopLabel << "'" << std::endl;

    // 점프 명령어 생성 - 임시로 큰 값 설정
    BytecodeInstruction jumpInstr(BytecodeOpCode::JMP);
    jumpInstr.operands.push_back(999999); // 패치 테이블을 통해 나중에 해결됨
    addInstruction(jumpInstr);

    // 패치 테이블에 추가
    int jumpInstrIndex = getCurrentSection().getInstructions().size() - 1;
    addJumpPatch(jumpInstrIndex, 0, endLabel);
    std::cerr << "Added break jump at " << jumpInstrIndex << " to target label '" << endLabel << "'" << std::endl;

    // break 이후의 코드는 실행되지 않지만, 일관성을 위해 레지스터를 반환
    int resultReg = allocateRegister();

    // 도달 불가능함을 나타내는 값 로드 (디버깅용)
    BytecodeInstruction unreachableInstr(BytecodeOpCode::LOAD_CONST);
    unreachableInstr.registers.push_back(resultReg);
    unreachableInstr.operands.push_back(std::string("unreachable"));
    addInstruction(unreachableInstr);

    return resultReg;
}

std::any BytecodeGenerator::visit_continue_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // continue 문장 처리
    // 현재 루프의 증감식 부분으로 점프하는 명령어 생성

    // 다운캐스팅을 통해 실제 continue 문장 노드 접근
    auto continueNode = node->as<ast::statement::ContinueStatementNode>();
    if (!continueNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 레이블 가져오기 (특정 루프로 점프하기 위한 레이블)
    const std::shared_ptr<ast::Expression> &labelExpr = continueNode->get_label();
    std::string targetLoopLabel;

    if (labelExpr) {
        // 레이블이 있는 경우 (특정 이름의 루프로 점프)
        auto idNode = labelExpr->as<ast::expression::IdentifierLiteralNode>();
        if (idNode) {
            icu::UnicodeString uniLabel = idNode->to_str();
            uniLabel.toUTF8String(targetLoopLabel);
        }
    }

    // 타겟 루프 컨텍스트 찾기
    LoopContext *targetLoop = findLoopContextByLabel(targetLoopLabel);

    if (!targetLoop) {
        // 루프 컨텍스트를 찾지 못한 경우
        int errorReg = allocateRegister();
        BytecodeInstruction errorInstr(BytecodeOpCode::LOAD_CONST);
        errorInstr.registers.push_back(errorReg);

        if (targetLoopLabel.empty()) {
            errorInstr.operands.push_back(std::string("error: continue outside loop"));
        } else {
            errorInstr.operands.push_back(std::string("error: continue to unknown label '") + targetLoopLabel + "'");
        }

        addInstruction(errorInstr);
        return errorReg;
    }

    // 대상 루프의 post 레이블 가져오기
    std::string postLabel = targetLoop->postLabel;

    // post 레이블을 확인 (없으면 오류)
    if (m_labels.find(postLabel) == m_labels.end()) {
        std::cerr << "Warning: Post label '" << postLabel << "' not found, defining it now." << std::endl;
        // 아직 정의되지 않았으면 미래 위치로 정의
        m_labels[postLabel] = -1; // 임시값
    }

    // 점프 명령어 생성 - 대상 루프의 post 레이블(증감식)로 점프
    BytecodeInstruction jumpInstr(BytecodeOpCode::JMP);
    jumpInstr.operands.push_back(0); // 임시 점프 위치 (나중에 업데이트)
    addInstruction(jumpInstr);

    // 패치 테이블에 추가
    int jumpInstrIndex = getCurrentSection().getInstructions().size() - 1;
    addJumpPatch(jumpInstrIndex, 0, postLabel);

    // continue 이후의 코드는 실행되지 않지만, 일관성을 위해 레지스터를 반환
    int resultReg = allocateRegister();

    // 도달 불가능함을 나타내는 값 로드 (디버깅용)
    BytecodeInstruction unreachableInstr(BytecodeOpCode::LOAD_CONST);
    unreachableInstr.registers.push_back(resultReg);
    unreachableInstr.operands.push_back(std::string("unreachable"));
    addInstruction(unreachableInstr);

    return resultReg;
}

std::any BytecodeGenerator::visit_expression_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 표현식 문장 처리
    // 표현식을 방문하여 바이트코드 생성

    // 다운캐스팅을 통해 실제 표현식 문장 노드 접근
    auto exprStmtNode = node->as<ast::statement::ExpressionStatementNode>();
    if (!exprStmtNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 표현식 가져오기
    const std::shared_ptr<ast::Expression> &expression = exprStmtNode->get_expression();
    if (!expression) {
        // 표현식이 없는 경우 - 빈 표현식 문 (;)
        // undefined 값을 반환
        int resultReg = allocateRegister();
        BytecodeInstruction undefinedInstr(BytecodeOpCode::LOAD_CONST);
        undefinedInstr.registers.push_back(resultReg);
        undefinedInstr.operands.push_back(std::string("undefined"));
        addInstruction(undefinedInstr);
        return resultReg;
    }

    // 표현식 방문하여 바이트코드 생성
    // 결과는 표현식의 결과 레지스터 번호
    return visit(expression, context);
}

std::any BytecodeGenerator::visit_for_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // for 문장 처리
    // 초기화, 조건, 증감식, 본문을 순차적으로 처리하여 바이트코드 생성

    // 다운캐스팅을 통해 실제 for 문장 노드 접근
    auto forNode = node->as<ast::statement::ForStatementNode>();
    if (!forNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 현재 섹션의 크기와 인덱스 저장 (디버깅용)
    int sectionSize = getCurrentSection().getInstructions().size();
    int currentSectionIndex = m_currentSectionIndex;
    std::cerr << "For statement in section " << m_sections[currentSectionIndex].getName() << " starting at instruction index " << sectionSize << std::endl;

    // 레이블 가져오기 (루프 식별을 위한 레이블)
    const std::shared_ptr<ast::Expression> &labelExpr = forNode->get_label();
    std::string loopLabel;

    if (labelExpr) {
        // 레이블이 있는 경우
        auto idNode = labelExpr->as<ast::expression::IdentifierLiteralNode>();
        if (idNode) {
            icu::UnicodeString uniLabel = idNode->to_str();
            uniLabel.toUTF8String(loopLabel);
        }
    }

    // 루프 컨텍스트 스택에 현재 루프 정보 추가
    pushLoopContext(loopLabel);
    LoopContext *currentLoop = getCurrentLoopContext();

    // 상위 스코프에서 참조 가능하도록 end 레이블 미리 정의 (중요)
    std::string endLabel = currentLoop->endLabel;
    // 레이블 맵에 미리 임시값으로 등록 (후에 실제 위치로 업데이트)
    m_labels[endLabel] = -1;
    std::cerr << "Pre-registering end label '" << endLabel << "' for loop '" << loopLabel << "'" << std::endl;

    // 루프 시작 위치 기록
    int loopStartIndex = getCurrentSection().getInstructions().size();
    currentLoop->loopStartInstrIndex = loopStartIndex;

    // 루프 레이블 미리 정의 (패치 테이블에서 참조할 수 있도록)
    // 시작 레이블은 현재 위치
    m_labels[currentLoop->startLabel] = loopStartIndex;
    std::cerr << "Defined label '" << currentLoop->startLabel << "' at position " << loopStartIndex << std::endl;

    // 초기화 표현식 처리
    const std::shared_ptr<ast::Expression> &initExpr = forNode->get_init();
    if (initExpr) {
        // 초기화 표현식 방문
        std::any initResult = visit(initExpr, context);

        // 결과가 레지스터라면 해제
        try {
            int initReg = std::any_cast<int>(initResult);
            freeRegister(initReg);
        } catch (const std::bad_any_cast &) {
            // 레지스터가 아닌 경우 무시
        }
    }

    // 조건 라벨 정의 (루프 시작 지점)
    int condIndex = getCurrentSection().getInstructions().size();
    m_labels[currentLoop->condLabel] = condIndex;
    std::cerr << "Defined label '" << currentLoop->condLabel << "' at position " << condIndex << std::endl;

    // 조건 표현식 처리
    const std::shared_ptr<ast::Expression> &condExpr = forNode->get_condition();

    // 종료 레이블을 저장할 위치를 미리 예약
    int endLabelInstrIndex = -1;

    if (condExpr) {
        // 조건 표현식 방문
        int condReg = std::any_cast<int>(visit(condExpr, context));

        // 조건이 거짓이면 루프 종료로 점프
        BytecodeInstruction jumpIfFalse(BytecodeOpCode::JMP_IF_FALSE);
        jumpIfFalse.registers.push_back(condReg);

        // 루프 종료 위치는 나중에 알 수 있으므로 임시값 설정
        // 나중에 실제 위치로 업데이트될 것
        jumpIfFalse.operands.push_back(99999);
        addInstruction(jumpIfFalse);

        // 조건부 점프 인덱스 기록 (나중에 패치에 사용)
        endLabelInstrIndex = getCurrentSection().getInstructions().size() - 1;

        // 패치 테이블에 추가 (중요: 명시적으로 패치 테이블에 추가)
        addJumpPatch(endLabelInstrIndex, 0, endLabel);
        std::cerr << "Added conditional jump to patch table for label: " << endLabel << std::endl;

        // 조건 레지스터 해제
        freeRegister(condReg);
    } else {
        // 조건이 없는 경우 항상 참으로 처리
        int tempReg = allocateRegister();
        BytecodeInstruction trueInstr(BytecodeOpCode::LOAD_CONST);
        trueInstr.registers.push_back(tempReg);
        trueInstr.operands.push_back(1); // true 값
        addInstruction(trueInstr);
        freeRegister(tempReg); // 사용 후 임시 레지스터 해제
    }

    // 본문 라벨 정의
    int bodyIndex = getCurrentSection().getInstructions().size();
    m_labels[currentLoop->bodyLabel] = bodyIndex;
    std::cerr << "Defined label '" << currentLoop->bodyLabel << "' at position " << bodyIndex << std::endl;

    // 본문 처리
    const std::shared_ptr<ast::Statement> &body = forNode->get_consequence();
    if (body) {
        // 본문 방문
        std::any bodyResult = visit(body, context);

        // 결과가 레지스터라면 해제 (for문 본문 결과는 사용되지 않음)
        try {
            int bodyReg = std::any_cast<int>(bodyResult);
            freeRegister(bodyReg);
        } catch (const std::bad_any_cast &) {
            // 레지스터가 아닌 경우 무시
        }
    }

    // 증감식 라벨 정의
    int postIndex = getCurrentSection().getInstructions().size();
    m_labels[currentLoop->postLabel] = postIndex;
    std::cerr << "Defined label '" << currentLoop->postLabel << "' at position " << postIndex << std::endl;

    // 증감식 처리
    const std::shared_ptr<ast::Expression> &postExpr = forNode->get_post();
    if (postExpr) {
        // 증감식 방문
        std::any postResult = visit(postExpr, context);

        // 결과가 레지스터라면 해제
        try {
            int postReg = std::any_cast<int>(postResult);
            freeRegister(postReg);
        } catch (const std::bad_any_cast &) {
            // 레지스터가 아닌 경우 무시
        }
    }

    // 조건 검사로 점프 (루프 반복)
    BytecodeInstruction jumpToStart(BytecodeOpCode::JMP);
    jumpToStart.operands.push_back(condIndex);
    addInstruction(jumpToStart);
    std::cerr << "Added jump to condition at position " << condIndex << std::endl;

    // 루프 종료 라벨 정의
    int endIndex = getCurrentSection().getInstructions().size();
    m_labels[endLabel] = endIndex; // 실제 위치로 업데이트
    std::cerr << "Defined end label '" << endLabel << "' at position " << endIndex << std::endl;

    // 현재 루프의 끝 위치 기록
    currentLoop->loopEndInstrIndex = endIndex;

    // for 문의 결과는 undefined (JavaScript 등의 언어에서 일반적)
    int resultReg = allocateRegister();
    BytecodeInstruction undefinedInstr(BytecodeOpCode::LOAD_CONST);
    undefinedInstr.registers.push_back(resultReg);
    undefinedInstr.operands.push_back(std::string("undefined"));
    addInstruction(undefinedInstr);

    // 루프 컨텍스트 스택에서 현재 루프 정보 제거
    // 중요: 루프 스택에서 제거하기 전에 모든 레이블이 올바르게 정의되었는지 확인
    for (const auto &[label, pos] : m_labels) {
        if (label.find(loopLabel) != std::string::npos && pos < 0) {
            std::cerr << "Warning: Label '" << label << "' related to loop '" << loopLabel << "' still has negative position before popping context"
                      << std::endl;
        }
    }

    // 루프 컨텍스트 스택에서 현재 루프 정보 제거하기 전에
    // 모든 break 문 패치가 처리되었는지 확인
    for (const auto &patch : m_jumpPatches) {
        if (patch.targetLabel == endLabel) {
            auto &instructions = const_cast<std::vector<BytecodeInstruction> &>(getCurrentSection().getInstructions());
            if (patch.instructionIndex >= 0 && patch.instructionIndex < static_cast<int>(instructions.size()) && patch.operandIndex >= 0 &&
                patch.operandIndex < static_cast<int>(instructions[patch.instructionIndex].operands.size())) {

                // 현재 점프 대상 확인
                int currentTarget = -1;
                try {
                    currentTarget = std::get<int>(instructions[patch.instructionIndex].operands[patch.operandIndex]);
                } catch (const std::bad_variant_access &) {
                    std::cerr << "Error reading jump target at " << patch.instructionIndex << std::endl;
                    continue;
                }

                // 필요한 경우 업데이트
                if (currentTarget != endIndex) {
                    // 타겟 레이블이 현재 루프의 끝 레이블이면, 직접 업데이트
                    instructions[patch.instructionIndex].operands[patch.operandIndex] = endIndex;
                    std::cerr << "Directly patched break jump at " << patch.instructionIndex << " from " << currentTarget << " to end position " << endIndex
                              << std::endl;
                }
            }
        }
    }

    popLoopContext();

    return resultReg;
}

std::any BytecodeGenerator::visit_let_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 변수 선언 처리
    // 변수 이름을 심볼 테이블에 등록하고 초기화 표현식 방문

    // 다운캐스팅을 통해 실제 let 문장 노드 접근
    auto letNode = node->as<ast::statement::LetStatementNode>();
    if (!letNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 변수 이름 가져오기
    const std::shared_ptr<ast::Expression> &nameExpr = letNode->get_name();
    if (!nameExpr) {
        // 변수 이름이 없는 경우 - 오류 상황
        int errorReg = allocateRegister();
        BytecodeInstruction errorInstr(BytecodeOpCode::LOAD_CONST);
        errorInstr.registers.push_back(errorReg);
        errorInstr.operands.push_back(std::string("error: missing variable name"));
        addInstruction(errorInstr);
        return errorReg;
    }

    // 변수 이름은 일반적으로 식별자 노드
    auto idNode = nameExpr->as<ast::expression::IdentifierLiteralNode>();
    if (!idNode) {
        // 잘못된 변수 이름 타입 - 오류 상황
        int errorReg = allocateRegister();
        BytecodeInstruction errorInstr(BytecodeOpCode::LOAD_CONST);
        errorInstr.registers.push_back(errorReg);
        errorInstr.operands.push_back(std::string("error: invalid variable name"));
        addInstruction(errorInstr);
        return errorReg;
    }

    // 변수 이름 문자열 추출
    icu::UnicodeString uniVarName = idNode->to_str();
    std::string varName;
    uniVarName.toUTF8String(varName);

    // 타입 체크 (선택적)
    const std::shared_ptr<ast::Expression> &typeExpr = letNode->get_type();
    std::string typeName;

    if (typeExpr) {
        // 타입 표현식이 있는 경우, 타입 이름 추출
        auto typeIdNode = typeExpr->as<ast::expression::IdentifierLiteralNode>();
        if (typeIdNode) {
            icu::UnicodeString uniTypeName = typeIdNode->to_str();
            uniTypeName.toUTF8String(typeName);
        }
    }

    // 초기화 표현식 가져오기
    const std::shared_ptr<ast::Expression> &valueExpr = letNode->get_value();

    // 초기화 값이 있는 경우 평가
    int valueReg = -1;
    if (valueExpr) {
        // 초기화 표현식 방문하여 바이트코드 생성
        valueReg = std::any_cast<int>(visit(valueExpr, context));
    } else {
        // 초기화 값이 없는 경우 기본값 (undefined 또는 null) 사용
        valueReg = allocateRegister();
        BytecodeInstruction defaultValueInstr(BytecodeOpCode::LOAD_CONST);
        defaultValueInstr.registers.push_back(valueReg);
        defaultValueInstr.operands.push_back(std::string("undefined")); // 기본값
        addInstruction(defaultValueInstr);
    }

    // 변수에 값 저장
    BytecodeInstruction storeInstr(BytecodeOpCode::STORE_VAR);
    storeInstr.registers.push_back(valueReg); // 저장할 값이 있는 레지스터
    storeInstr.operands.push_back(varName);   // 변수 이름
    // 변수 타입 정보 추가 (선택적)
    if (!typeName.empty()) {
        storeInstr.operands.push_back(typeName); // 타입 이름
    }
    addInstruction(storeInstr);

    // 심볼 테이블에 변수 등록
    m_variables[varName] = valueReg;

    // 값 레지스터 반환 (let 문장의 결과는 할당된 값)
    return valueReg;
}

std::any BytecodeGenerator::visit_return_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // return 문장 처리
    // 반환값이 있다면 해당 표현식 방문 후 반환 명령어 생성

    // 다운캐스팅을 통해 실제 return 문장 노드 접근
    auto returnNode = node->as<ast::statement::ReturnStatementNode>();
    if (!returnNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 레이블 검사 (지정된 함수로 반환하기 위한 레이블)
    const std::shared_ptr<ast::Expression> &labelExpr = returnNode->get_label();
    std::string targetFuncName;

    if (labelExpr) {
        // 레이블이 있는 경우 (특정 함수에서 반환)
        auto idNode = labelExpr->as<ast::expression::IdentifierLiteralNode>();
        if (idNode) {
            icu::UnicodeString uniLabel = idNode->to_str();
            uniLabel.toUTF8String(targetFuncName);
        }
    }

    // 대상 함수 컨텍스트 찾기
    FunctionContext *targetFunc = findFunctionContextByName(targetFuncName);

    if (!targetFunc) {
        // 함수 컨텍스트를 찾지 못한 경우
        int errorReg = allocateRegister();
        BytecodeInstruction errorInstr(BytecodeOpCode::LOAD_CONST);
        errorInstr.registers.push_back(errorReg);

        if (targetFuncName.empty()) {
            errorInstr.operands.push_back(std::string("error: return outside function"));
        } else {
            errorInstr.operands.push_back(std::string("error: return to unknown function '") + targetFuncName + "'");
        }

        addInstruction(errorInstr);
        return errorReg;
    }

    // 대상 함수의 return 레이블 가져오기
    std::string returnLabel = targetFunc->returnLabel;

    // return 레이블을 확인 (없으면 오류)
    if (m_labels.find(returnLabel) == m_labels.end()) {
        std::cerr << "Warning: Return label '" << returnLabel << "' not found, defining it now." << std::endl;
        // 아직 정의되지 않았으면 미래 위치로 정의
        m_labels[returnLabel] = -1; // 임시값
    }

    // 반환값 표현식 가져오기
    const std::shared_ptr<ast::Expression> &valueExpr = returnNode->get_value();

    // 반환값 레지스터 - 대상 함수의 반환 레지스터 사용
    int valueReg = targetFunc->returnRegister;

    if (valueExpr) {
        // 반환값 표현식 방문하여 바이트코드 생성
        int exprReg = std::any_cast<int>(visit(valueExpr, context));

        // 값을 함수의 반환 레지스터로 복사
        BytecodeInstruction movInstr(BytecodeOpCode::MOV_REG);
        movInstr.registers.push_back(valueReg);
        movInstr.registers.push_back(exprReg);
        addInstruction(movInstr);

        // 임시 레지스터 해제
        freeRegister(exprReg);
    } else {
        // 반환값이 없는 경우 기본값 (undefined) 사용
        BytecodeInstruction defaultValueInstr(BytecodeOpCode::LOAD_CONST);
        defaultValueInstr.registers.push_back(valueReg);
        defaultValueInstr.operands.push_back(std::string("undefined")); // 기본값
        addInstruction(defaultValueInstr);
    }

    // 반환 명령어 생성
    BytecodeInstruction retInstr(BytecodeOpCode::RET);
    retInstr.registers.push_back(valueReg); // 반환값이 있는 레지스터

    // 특정 함수로 점프하는 경우 함수 이름 추가
    if (!targetFuncName.empty() && targetFunc != getCurrentFunctionContext()) {
        // 비지역 반환(non-local return)의 경우
        retInstr.operands.push_back(targetFuncName);
    }

    addInstruction(retInstr);

    // 반환 이후의 코드는 실행되지 않지만, 일관성을 위해 레지스터를 반환
    int resultReg = allocateRegister();

    // 도달 불가능함을 나타내는 값 로드 (디버깅용)
    BytecodeInstruction unreachableInstr(BytecodeOpCode::LOAD_CONST);
    unreachableInstr.registers.push_back(resultReg);
    unreachableInstr.operands.push_back(std::string("unreachable"));
    addInstruction(unreachableInstr);

    return resultReg;
}

std::any BytecodeGenerator::visit_array_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 배열 리터럴 표현식 처리

    // 다운캐스팅을 통해 실제 배열 리터럴 노드 접근
    auto arrayNode = node->as<ast::expression::ArrayLiteralNode>();
    if (!arrayNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 레지스터 할당 (배열 참조를 저장)
    int resultReg = allocateRegister();

    // 배열 생성 명령어 (실제로는 특별한 명령어나 함수 호출이 필요할 수 있음)
    BytecodeInstruction createArrayInstr(BytecodeOpCode::LOAD_CONST);
    createArrayInstr.registers.push_back(resultReg);
    createArrayInstr.operands.push_back(std::string("[]")); // 배열 생성 표시 (예시)
    addInstruction(createArrayInstr);

    // 배열 요소 가져오기
    const std::vector<std::shared_ptr<ast::Expression>> &elements = arrayNode->get_elements();

    // 각 요소 평가 및 배열에 추가
    std::vector<int> elemRegs;
    for (const auto &elem : elements) {
        // 요소 표현식 평가
        int elemReg = std::any_cast<int>(visit(elem, context));
        elemRegs.push_back(elemReg);

        // 여기서는 배열에 요소를 추가하는 실제 명령어를 생성해야 합니다.
        // 실제 구현에서는 추가 명령어가 필요할 수 있습니다.
    }

    // 요소 레지스터 해제
    for (int reg : elemRegs) {
        freeRegister(reg);
    }

    // 결과 레지스터 번호 반환 (배열 참조)
    return resultReg;
}

std::any BytecodeGenerator::visit_boolean_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // Boolean 리터럴 AST 노드 처리
    // 상수 값을 레지스터에 로드

    // 다운캐스팅을 통해 실제 Boolean 리터럴 노드 접근
    auto boolNode = node->as<ast::expression::BooleanLiteralNode>();
    if (!boolNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 불리언 값 가져오기
    bool value = boolNode->get_value();

    // 상수 로드 명령어 생성
    BytecodeInstruction instr(BytecodeOpCode::LOAD_CONST);
    instr.registers.push_back(resultReg);              // 결과를 저장할 레지스터
    instr.operands.push_back(static_cast<int>(value)); // 불리언 값을 정수로 변환 (0 또는 1)

    // 명령어 추가
    addInstruction(instr);

    // 결과 레지스터 번호 반환 - 다른 표현식에서 이 레지스터를 사용할 수 있음
    return resultReg;
}

std::any BytecodeGenerator::visit_call_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 함수 호출 처리
    // 인자 표현식들을 방문한 후 함수 호출 명령어 생성

    // 다운캐스팅을 통해 실제 함수 호출 노드 접근
    auto callNode = node->as<ast::expression::CallExpressionNode>();
    if (!callNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 호출할 함수 표현식 가져오기
    std::shared_ptr<ast::Expression> callee = callNode->get_callee();
    if (!callee) {
        // 함수 표현식이 없는 경우
        return {};
    }

    // 함수 이름 추출 (식별자인 경우)
    std::string funcName;
    auto idNode = callee->as<ast::expression::IdentifierLiteralNode>();
    if (idNode) {
        icu::UnicodeString uniFuncName = idNode->to_str();
        uniFuncName.toUTF8String(funcName);
    }

    // 함수 표현식 방문하여 바이트코드 생성 (함수 참조를 레지스터에 로드)
    int calleeReg = std::any_cast<int>(visit(callee, context));

    // 인자 표현식들 가져오기
    const std::vector<std::shared_ptr<ast::Expression>> &arguments = callNode->get_arguments();

    // 각 인자 평가 및 레지스터에 로드
    std::vector<int> argRegs;
    for (const auto &arg : arguments) {
        // 인자 표현식 방문하여 바이트코드 생성
        int argReg = std::any_cast<int>(visit(arg, context));
        argRegs.push_back(argReg);
    }

    // 결과를 저장할 레지스터 할당
    int resultReg = allocateRegister();

    // 호출할 함수의 컨텍스트 찾기
    FunctionContext *targetFunc = findFunctionContextByName(funcName);
    if (targetFunc) {
        // 함수가 컨텍스트 스택에 있는 경우 매개변수 매핑

        // 매개변수 매핑 정보 추가
        if (argRegs.size() <= targetFunc->paramMap.size()) {
            BytecodeInstruction mapParamsInstr(BytecodeOpCode::LOAD_CONST);
            mapParamsInstr.registers.push_back(resultReg);
            mapParamsInstr.operands.push_back(std::string("params_mapping"));
            mapParamsInstr.operands.push_back(static_cast<int>(argRegs.size()));
            addInstruction(mapParamsInstr);
        }
    }

    // 함수 호출 명령어 생성
    BytecodeInstruction callInstr(BytecodeOpCode::CALL);
    callInstr.registers.push_back(resultReg); // 결과를 저장할 레지스터
    callInstr.registers.push_back(calleeReg); // 호출할 함수가 있는 레지스터

    // 인자 레지스터들 추가
    for (int argReg : argRegs) {
        callInstr.registers.push_back(argReg);
    }

    // 인자 개수 추가
    callInstr.operands.push_back(static_cast<int>(argRegs.size()));

    // 함수 이름이 있는 경우 추가 (디버깅 및 스택 추적용)
    if (!funcName.empty()) {
        callInstr.operands.push_back(funcName);
    }

    // 명령어 추가
    addInstruction(callInstr);

    // 함수 및 인자 레지스터 해제
    freeRegister(calleeReg);
    for (int argReg : argRegs) {
        freeRegister(argReg);
    }

    // 결과 레지스터 번호 반환 (함수 호출의 결과)
    return resultReg;
}

std::any BytecodeGenerator::visit_function_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 함수 리터럴 처리
    // 새로운 섹션 생성 후 함수 본문 방문

    // 다운캐스팅을 통해 실제 함수 리터럴 노드 접근
    auto funcNode = node->as<ast::expression::FunctionExpressionNode>();
    if (!funcNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 함수 이름 생성 (익명 함수의 경우 고유 이름 생성)
    std::string funcName = "func_" + std::to_string(m_nextLabelId++);

    // 함수 컨텍스트 스택에 추가
    pushFunctionContext(funcName);
    FunctionContext *currentFunc = getCurrentFunctionContext();

    // 새로운 함수 섹션 생성
    m_sections.emplace_back(funcName);
    int prevSectionIndex = m_currentSectionIndex;
    m_currentSectionIndex = static_cast<int>(m_sections.size()) - 1;

    // 함수 시작 라벨 정의
    m_labels[currentFunc->startLabel] = 0; // 함수 섹션의 시작

    // 반환값을 저장할 레지스터 할당
    currentFunc->returnRegister = allocateRegister();

    // 매개변수 정보 저장
    const auto &parameters = funcNode->get_parameters();

    // 매개변수 이름 추출 및 레지스터 할당
    for (const auto &param : parameters) {
        // 매개변수 이름 (첫 번째 요소)
        std::shared_ptr<ast::Expression> paramNameExpr = std::get<0>(param);
        if (paramNameExpr) {
            auto idNode = paramNameExpr->as<ast::expression::IdentifierLiteralNode>();
            if (idNode) {
                icu::UnicodeString uniParamName = idNode->to_str();
                std::string paramName;
                uniParamName.toUTF8String(paramName);

                // 매개변수용 레지스터 할당
                int paramReg = allocateRegister();

                // 매개변수 이름과 레지스터 매핑
                currentFunc->paramMap[paramName] = paramReg;

                // 함수의 지역 변수 목록에 추가
                currentFunc->localVars.push_back(paramReg);
            }
        }
    }

    // 함수 본문 방문
    std::shared_ptr<ast::Statement> body = funcNode->get_body();
    if (body) {
        visit(body, context);
    }

    // 명시적 반환이 없는 경우 기본 반환값(undefined) 처리
    // 마지막 명령어가 RET이 아닌지 확인
    auto &instructions = getCurrentSection().getInstructions();
    bool hasReturn = false;
    if (!instructions.empty()) {
        hasReturn = instructions.back().opcode == BytecodeOpCode::RET;
    }

    if (!hasReturn) {
        // undefined 값 반환
        BytecodeInstruction undefinedInstr(BytecodeOpCode::LOAD_CONST);
        undefinedInstr.registers.push_back(currentFunc->returnRegister);
        undefinedInstr.operands.push_back(std::string("undefined"));
        addInstruction(undefinedInstr);

        // 반환 명령어 추가
        BytecodeInstruction retInstr(BytecodeOpCode::RET);
        retInstr.registers.push_back(currentFunc->returnRegister);
        addInstruction(retInstr);
    }

    // 함수 끝 라벨 정의
    currentFunc->functionEndIndex = getCurrentSection().getInstructions().size();
    m_labels[currentFunc->endLabel] = currentFunc->functionEndIndex;
    m_labels[currentFunc->returnLabel] = currentFunc->functionEndIndex - 1; // 반환 명령어 위치

    // 컨텍스트에서 함수 제거
    popFunctionContext();

    // 함수 섹션 종료 후 이전 섹션으로 복귀
    m_currentSectionIndex = prevSectionIndex;

    // 결과를 저장할 레지스터 할당
    int resultReg = allocateRegister();

    // 함수 참조 로드 명령어 생성
    BytecodeInstruction loadFuncInstr(BytecodeOpCode::LOAD_CONST);
    loadFuncInstr.registers.push_back(resultReg);
    loadFuncInstr.operands.push_back(funcName); // 함수 이름
    addInstruction(loadFuncInstr);

    // 결과 레지스터 번호 반환 (함수 참조)
    return resultReg;
}

std::any BytecodeGenerator::visit_identifier_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 식별자 표현식 처리
    // 변수의 값을 레지스터로 로드

    // 다운캐스팅을 통해 실제 식별자 노드 접근
    auto idNode = node->as<ast::expression::IdentifierLiteralNode>();
    if (!idNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 변수 이름 가져오기
    icu::UnicodeString uniStr = idNode->to_str();
    std::string varName;
    uniStr.toUTF8String(varName);

    // 변수 값 로드 명령어 생성
    BytecodeInstruction instr(BytecodeOpCode::LOAD_VAR);
    instr.registers.push_back(resultReg); // 결과를 저장할 레지스터
    instr.operands.push_back(varName);    // 변수 이름

    // 명령어 추가
    addInstruction(instr);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_if_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // if 표현식 처리
    // 조건 평가 후 결과에 따라 분기

    // 다운캐스팅을 통해 실제 If 표현식 노드 접근
    auto ifNode = node->as<ast::expression::IfExpressionNode>();
    if (!ifNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 레지스터 할당
    int resultReg = allocateRegister();

    // 조건 평가 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Expression> condExpr = ifNode->get_condition();
    if (!condExpr) {
        // 조건이 없는 경우 항상 참으로 처리
        BytecodeInstruction trueInstr(BytecodeOpCode::LOAD_CONST);
        trueInstr.registers.push_back(resultReg);
        trueInstr.operands.push_back(1); // true 값
        addInstruction(trueInstr);
        return resultReg;
    }

    int condReg = std::any_cast<int>(visit(condExpr, context));

    // 라벨 생성
    std::string elseLabel = generateUniqueLabel();
    std::string endLabel = generateUniqueLabel();

    // 조건이 거짓이면 else 블록으로 점프
    BytecodeInstruction jumpIfFalse(BytecodeOpCode::JMP_IF_FALSE);
    jumpIfFalse.registers.push_back(condReg);
    jumpIfFalse.operands.push_back(0); // 임시 점프 위치 (패치 테이블을 통해 나중에 업데이트)
    addInstruction(jumpIfFalse);

    // 패치 테이블에 기록
    int jumpIfFalseIndex = getCurrentSection().getInstructions().size() - 1;
    addJumpPatch(jumpIfFalseIndex, 0, elseLabel);

    // 조건 레지스터 해제 (더 이상 필요 없음)
    freeRegister(condReg);

    // then 블록 실행 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Statement> consequence = ifNode->get_consequence();
    if (consequence) {
        // consequence 블록 방문 - 결과가 있으면 resultReg에 복사
        std::any thenResult = visit(consequence, context);

        // 결과가 레지스터라면 resultReg에 복사 후 해제
        try {
            int thenReg = std::any_cast<int>(thenResult);
            // MOV 명령어로 결과 복사
            BytecodeInstruction movInstr(BytecodeOpCode::MOV_REG);
            movInstr.registers.push_back(resultReg); // 대상 레지스터
            movInstr.registers.push_back(thenReg);   // 소스 레지스터
            addInstruction(movInstr);
            // 임시 레지스터 해제
            freeRegister(thenReg);
        } catch (const std::bad_any_cast &) {
            // 결과가 없는 경우 undefined 로드
            BytecodeInstruction undefinedInstr(BytecodeOpCode::LOAD_CONST);
            undefinedInstr.registers.push_back(resultReg);
            undefinedInstr.operands.push_back(std::string("undefined"));
            addInstruction(undefinedInstr);
        }
    } else {
        // consequence 블록이 없는 경우 undefined 로드
        BytecodeInstruction undefinedInstr(BytecodeOpCode::LOAD_CONST);
        undefinedInstr.registers.push_back(resultReg);
        undefinedInstr.operands.push_back(std::string("undefined"));
        addInstruction(undefinedInstr);
    }

    // then 블록 처리 후 end로 점프
    BytecodeInstruction jumpToEnd(BytecodeOpCode::JMP);
    jumpToEnd.operands.push_back(0); // 임시 점프 위치 (패치 테이블을 통해 나중에 업데이트)
    addInstruction(jumpToEnd);

    // 패치 테이블에 기록
    int jumpToEndIndex = getCurrentSection().getInstructions().size() - 1;
    addJumpPatch(jumpToEndIndex, 0, endLabel);

    // else 라벨 정의
    m_labels[elseLabel] = getCurrentSection().getInstructions().size();

    // else 블록 실행 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Statement> alternative = ifNode->get_alternative();
    if (alternative) {
        // alternative 블록 방문 - 결과가 있으면 resultReg에 복사
        std::any elseResult = visit(alternative, context);

        // 결과가 레지스터라면 resultReg에 복사 후 해제
        try {
            int elseReg = std::any_cast<int>(elseResult);
            // MOV 명령어로 결과 복사
            BytecodeInstruction movInstr(BytecodeOpCode::MOV_REG);
            movInstr.registers.push_back(resultReg); // 대상 레지스터
            movInstr.registers.push_back(elseReg);   // 소스 레지스터
            addInstruction(movInstr);
            // 임시 레지스터 해제
            freeRegister(elseReg);
        } catch (const std::bad_any_cast &) {
            // 결과가 없는 경우 undefined 로드
            BytecodeInstruction undefinedInstr(BytecodeOpCode::LOAD_CONST);
            undefinedInstr.registers.push_back(resultReg);
            undefinedInstr.operands.push_back(std::string("undefined"));
            addInstruction(undefinedInstr);
        }
    } else {
        // alternative 블록이 없는 경우 undefined 로드
        BytecodeInstruction undefinedInstr(BytecodeOpCode::LOAD_CONST);
        undefinedInstr.registers.push_back(resultReg);
        undefinedInstr.operands.push_back(std::string("undefined"));
        addInstruction(undefinedInstr);
    }

    // end 라벨 정의
    m_labels[endLabel] = getCurrentSection().getInstructions().size();

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_index_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 배열 인덱싱 표현식 처리

    // 다운캐스팅을 통해 실제 인덱스 표현식 노드 접근
    auto indexNode = node->as<ast::expression::IndexExpressionNode>();
    if (!indexNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 레지스터 할당
    int resultReg = allocateRegister();

    // 배열 표현식 평가 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Expression> leftExpr = indexNode->get_left();
    int arrayReg = std::any_cast<int>(visit(leftExpr, context));

    // 인덱스 표현식 평가 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Expression> indexExpr = indexNode->get_index();
    int indexReg = std::any_cast<int>(visit(indexExpr, context));

    // 인덱싱 연산 (실제로는 특별한 명령어나 함수 호출이 필요할 수 있음)
    // 여기서는 가상의 "INDEX" 명령어를 가정하여 시뮬레이션
    BytecodeInstruction indexingInstr(BytecodeOpCode::LOAD_CONST); // 실제로는 INDEX 등의 전용 명령어 필요
    indexingInstr.registers.push_back(resultReg);
    indexingInstr.registers.push_back(arrayReg);
    indexingInstr.registers.push_back(indexReg);
    addInstruction(indexingInstr);

    // 임시 레지스터 해제
    freeRegister(arrayReg);
    freeRegister(indexReg);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_infix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 이항 연산자 표현식 처리

    // 다운캐스팅을 통해 실제 이항 연산자 노드 접근
    auto infixNode = node->as<ast::expression::InfixExpressionNode>();
    if (!infixNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 연산자 가져오기
    icu::UnicodeString uniOp = infixNode->get_operator();
    std::string op;
    uniOp.toUTF8String(op);

    // 좌측 피연산자 평가 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Expression> leftExpr = infixNode->get_left();
    int leftReg = std::any_cast<int>(visit(leftExpr, context));

    // 우측 피연산자 평가 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Expression> rightExpr = infixNode->get_right();
    int rightReg = std::any_cast<int>(visit(rightExpr, context));

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 연산자에 따라 적절한 명령어 생성
    BytecodeOpCode opcode;

    if (op == "+") {
        opcode = BytecodeOpCode::ADD;
    } else if (op == "-") {
        opcode = BytecodeOpCode::SUB;
    } else if (op == "*") {
        opcode = BytecodeOpCode::MUL;
    } else if (op == "/") {
        opcode = BytecodeOpCode::DIV;
    } else if (op == "%") {
        opcode = BytecodeOpCode::MOD;
    } else if (op == "==") {
        opcode = BytecodeOpCode::CMP_EQ;
    } else if (op == "!=") {
        opcode = BytecodeOpCode::CMP_NE;
    } else if (op == "<") {
        opcode = BytecodeOpCode::CMP_LT;
    } else if (op == "<=") {
        opcode = BytecodeOpCode::CMP_LE;
    } else if (op == ">") {
        opcode = BytecodeOpCode::CMP_GT;
    } else if (op == ">=") {
        opcode = BytecodeOpCode::CMP_GE;
    } else if (op == "&&") {
        opcode = BytecodeOpCode::AND;
    } else if (op == "||") {
        opcode = BytecodeOpCode::OR;
    } else {
        // 지원하지 않는 연산자
        freeRegister(leftReg);
        freeRegister(rightReg);
        freeRegister(resultReg);
        return {};
    }

    // 연산 명령어 생성
    BytecodeInstruction instr(opcode);
    instr.registers.push_back(resultReg); // 결과 레지스터
    instr.registers.push_back(leftReg);   // 좌측 피연산자 레지스터
    instr.registers.push_back(rightReg);  // 우측 피연산자 레지스터

    // 명령어 추가
    addInstruction(instr);

    // 피연산자 레지스터 해제 (더 이상 필요 없음)
    freeRegister(leftReg);
    freeRegister(rightReg);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_number_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // Number 리터럴 AST 노드 처리
    // 상수 값을 레지스터에 로드

    // 다운캐스팅을 통해 실제 Number 리터럴 노드 접근
    auto numNode = node->as<ast::expression::NumberLiteralNode>();
    if (!numNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 숫자 값 가져오기
    icu::UnicodeString valueStr = numNode->to_str();
    std::string value;
    valueStr.toUTF8String(value);

    // 정수인지 실수인지 확인
    bool isInt = valueStr.indexOf('.') < 0;
    BytecodeOperand operand;

    if (isInt) {
        // 정수로 처리
        try {
            operand = std::stoi(value);
        } catch (...) {
            // 변환 실패 시 0으로 기본값 설정
            operand = 0;
        }
    } else {
        // 실수로 처리
        try {
            operand = std::stof(value);
        } catch (...) {
            // 변환 실패 시 0.0으로 기본값 설정
            operand = 0.0f;
        }
    }

    // 상수 로드 명령어 생성
    BytecodeInstruction instr(BytecodeOpCode::LOAD_CONST);
    instr.registers.push_back(resultReg); // 결과를 저장할 레지스터
    instr.operands.push_back(operand);    // 숫자 값

    // 명령어 추가
    addInstruction(instr);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_postfix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 후위 연산자 표현식 처리

    // 다운캐스팅을 통해 실제 후위 연산자 노드 접근
    auto postNode = node->as<ast::expression::PostExpressionNode>();
    if (!postNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 연산자 가져오기
    icu::UnicodeString uniOp = postNode->get_operator();
    std::string op;
    uniOp.toUTF8String(op);

    // 왼쪽 피연산자 평가 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Expression> leftExpr = postNode->get_left();
    int operandReg = std::any_cast<int>(visit(leftExpr, context));

    // 변수 이름 가져오기 (식별자 노드로 가정, 실제로는 타입 확인 필요)
    icu::UnicodeString varUniName;
    std::string varName;

    // 식별자 노드인지 확인 (여기서는 식별자만 ++/-- 연산 지원한다고 가정)
    auto idNode = leftExpr->as<ast::expression::IdentifierLiteralNode>();
    if (idNode) {
        varUniName = idNode->to_str();
        varUniName.toUTF8String(varName);
    } else {
        // 식별자가 아닌 경우 단순히 피연산자 결과 반환 (실제론 에러 처리 필요)
        return operandReg;
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();
    int updatedReg = allocateRegister();

    // 연산자에 따라 적절한 명령어 생성
    if (op == "++") {
        // 원래 값을 결과 레지스터에 복사
        BytecodeInstruction movInstr(BytecodeOpCode::MOV_REG);
        movInstr.registers.push_back(resultReg);  // 대상 레지스터
        movInstr.registers.push_back(operandReg); // 소스 레지스터
        addInstruction(movInstr);

        // 1을 담을 임시 레지스터
        int oneReg = allocateRegister();

        // 1 로드
        BytecodeInstruction loadOne(BytecodeOpCode::LOAD_CONST);
        loadOne.registers.push_back(oneReg);
        loadOne.operands.push_back(1);
        addInstruction(loadOne);

        // 덧셈 수행 (증가)
        BytecodeInstruction addInstr(BytecodeOpCode::ADD);
        addInstr.registers.push_back(updatedReg); // 업데이트된 값 레지스터
        addInstr.registers.push_back(operandReg); // 원래 값 레지스터
        addInstr.registers.push_back(oneReg);     // 1을 담은 레지스터
        addInstruction(addInstr);

        // 임시 레지스터 해제
        freeRegister(oneReg);

    } else if (op == "--") {
        // 원래 값을 결과 레지스터에 복사
        BytecodeInstruction movInstr(BytecodeOpCode::MOV_REG);
        movInstr.registers.push_back(resultReg);  // 대상 레지스터
        movInstr.registers.push_back(operandReg); // 소스 레지스터
        addInstruction(movInstr);

        // 1을 담을 임시 레지스터
        int oneReg = allocateRegister();

        // 1 로드
        BytecodeInstruction loadOne(BytecodeOpCode::LOAD_CONST);
        loadOne.registers.push_back(oneReg);
        loadOne.operands.push_back(1);
        addInstruction(loadOne);

        // 뺄셈 수행 (감소)
        BytecodeInstruction subInstr(BytecodeOpCode::SUB);
        subInstr.registers.push_back(updatedReg); // 업데이트된 값 레지스터
        subInstr.registers.push_back(operandReg); // 원래 값 레지스터
        subInstr.registers.push_back(oneReg);     // 1을 담은 레지스터
        addInstruction(subInstr);

        // 임시 레지스터 해제
        freeRegister(oneReg);

    } else {
        // 지원하지 않는 연산자
        freeRegister(operandReg);
        freeRegister(resultReg);
        freeRegister(updatedReg);
        return {};
    }

    // 변수에 업데이트된 값 저장
    BytecodeInstruction storeInstr(BytecodeOpCode::STORE_VAR);
    storeInstr.registers.push_back(updatedReg); // 저장할 값이 있는 레지스터
    storeInstr.operands.push_back(varName);     // 변수 이름
    addInstruction(storeInstr);

    // 피연산자 및 임시 레지스터 해제
    freeRegister(operandReg);
    freeRegister(updatedReg);

    // 결과 레지스터 번호 반환 (원래 값)
    return resultReg;
}

std::any BytecodeGenerator::visit_prefix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 전위 연산자 표현식 처리

    // 다운캐스팅을 통해 실제 전위 연산자 노드 접근
    auto prefixNode = node->as<ast::expression::PrefixExpressionNode>();
    if (!prefixNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 전위 연산자 가져오기
    icu::UnicodeString uniOp = prefixNode->get_operator();
    std::string op;
    uniOp.toUTF8String(op);

    // 오른쪽 피연산자 평가 (실제 AST 구조를 통해 가져옴)
    std::shared_ptr<ast::Expression> rightExpr = prefixNode->get_right();
    int operandReg = std::any_cast<int>(visit(rightExpr, context));

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 연산자에 따라 적절한 명령어 생성
    if (op == "-") {
        // 단항 마이너스 (부호 반전)
        // SUB 명령어를 사용하여 0에서 값을 뺌

        // 0을 담을 임시 레지스터
        int zeroReg = allocateRegister();

        // 0 로드
        BytecodeInstruction loadZero(BytecodeOpCode::LOAD_CONST);
        loadZero.registers.push_back(zeroReg);
        loadZero.operands.push_back(0);
        addInstruction(loadZero);

        // 뺄셈 수행
        BytecodeInstruction subInstr(BytecodeOpCode::SUB);
        subInstr.registers.push_back(resultReg);  // 결과 레지스터
        subInstr.registers.push_back(zeroReg);    // 0을 담은 레지스터
        subInstr.registers.push_back(operandReg); // 피연산자 레지스터
        addInstruction(subInstr);

        // 임시 레지스터 해제
        freeRegister(zeroReg);

    } else if (op == "!") {
        // 논리 NOT
        BytecodeInstruction notInstr(BytecodeOpCode::NOT);
        notInstr.registers.push_back(resultReg);  // 결과 레지스터
        notInstr.registers.push_back(operandReg); // 피연산자 레지스터
        addInstruction(notInstr);

    } else {
        // 지원하지 않는 연산자
        freeRegister(operandReg);
        freeRegister(resultReg);
        return {};
    }

    // 피연산자 레지스터 해제
    freeRegister(operandReg);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_string_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // String 리터럴 AST 노드 처리
    // 문자열 상수를 레지스터에 로드

    // 다운캐스팅을 통해 실제 String 리터럴 노드 접근
    auto strNode = node->as<ast::expression::StringLiteralNode>();
    if (!strNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 문자열 값 가져오기
    icu::UnicodeString uniStr = strNode->to_str();
    std::string value;
    uniStr.toUTF8String(value);

    // 따옴표 제거 (첫 번째와 마지막 문자)
    if (value.size() >= 2 && (value[0] == '"' || value[0] == '\'') && value[0] == value[value.size() - 1]) {
        value = value.substr(1, value.size() - 2);
    }

    // 상수 로드 명령어 생성
    BytecodeInstruction instr(BytecodeOpCode::LOAD_CONST);
    instr.registers.push_back(resultReg); // 결과를 저장할 레지스터
    instr.operands.push_back(value);      // 문자열 값

    // 명령어 추가
    addInstruction(instr);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_when_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // when 표현식 처리 (switch-case와 유사)
    // 조건과 각 케이스를 방문하여 바이트코드 생성

    return {};
}

void BytecodeGenerator::generate(const ast::ASTNodePtr &rootNode) {
    // AST 루트 노드부터 시작하여 바이트코드 생성
    visit(rootNode);

    // 임시 레이블(-1로 설정된 것) 검출
    std::vector<std::string> temporaryLabels;
    for (const auto &[label, position] : m_labels) {
        if (position < 0) {
            temporaryLabels.push_back(label);
            std::cerr << "Warning: Label '" << label << "' has temporary value." << std::endl;
        }
    }

    // 모든 임시 레이블은 실제 위치를 찾지 못한 것이므로 안전한 값(0)으로 설정
    for (const auto &label : temporaryLabels) {
        std::cerr << "Warning: Fixing temporary label '" << label << "' to position 0." << std::endl;
        m_labels[label] = 0;
    }

    // 모든 코드 생성이 완료된 후 점프 위치 해결
    resolveJumpPatches();

    // 패치 처리 후 점프 명령어 검증
    validateJumps();

    // 함수 컨텍스트 스택과 루프 컨텍스트 스택 검증
    if (!m_functionContextStack.empty()) {
        std::cerr << "Warning: Function context stack not empty after code generation. " << m_functionContextStack.size() << " contexts remain." << std::endl;

        // 남은 컨텍스트 모두 제거
        while (!m_functionContextStack.empty()) {
            popFunctionContext();
        }
    }

    if (!m_loopContextStack.empty()) {
        std::cerr << "Warning: Loop context stack not empty after code generation. " << m_loopContextStack.size() << " contexts remain." << std::endl;

        // 남은 컨텍스트 모두 제거
        while (!m_loopContextStack.empty()) {
            popLoopContext();
        }
    }
}

void BytecodeGenerator::validateJumps() {
    // 모든 섹션에 대해 점프 명령어 검증
    for (const auto &section : m_sections) {
        const auto &instructions = section.getInstructions();
        std::unordered_map<int, bool> validJumpTargets;

        // 유효한 점프 대상 위치 수집
        for (size_t i = 0; i < instructions.size(); i++) {
            validJumpTargets[static_cast<int>(i)] = true;
        }

        // 모든 라벨 위치도 유효한 점프 대상으로 취급
        for (const auto &[label, pos] : m_labels) {
            if (pos >= 0 && pos < static_cast<int>(instructions.size())) {
                validJumpTargets[pos] = true;
            }
        }

        for (size_t i = 0; i < instructions.size(); i++) {
            const auto &instr = instructions[i];

            // 점프 관련 명령어 확인
            if (instr.opcode == BytecodeOpCode::JMP || instr.opcode == BytecodeOpCode::JMP_IF_TRUE || instr.opcode == BytecodeOpCode::JMP_IF_FALSE) {
                // 점프 대상 확인
                if (!instr.operands.empty()) {
                    try {
                        int jumpTarget = std::get<int>(instr.operands[0]);

                        // 유효한 점프 위치인지 확인
                        if (jumpTarget < 0 || jumpTarget >= static_cast<int>(instructions.size())) {
                            std::cerr << "Error in section " << section.getName() << ": Jump instruction at " << i << " points to invalid target " << jumpTarget
                                      << " (outside section bounds 0-" << instructions.size() - 1 << ")" << std::endl;
                        } else if (validJumpTargets.find(jumpTarget) == validJumpTargets.end() || !validJumpTargets[jumpTarget]) {
                            std::cerr << "Warning in section " << section.getName() << ": Jump instruction at " << i << " points to " << jumpTarget
                                      << " which is not a known label position" << std::endl;
                        }

                        // 점프 대상이 명령어의 OpCode인지 확인 (실행 가능한지)
                        if (jumpTarget >= 0 && jumpTarget < static_cast<int>(instructions.size())) {
                            const auto &targetInstr = instructions[jumpTarget];
                            std::cerr << "Jump at " << i << " (" << opCodeToString(instr.opcode) << ") -> " << jumpTarget << " ("
                                      << opCodeToString(targetInstr.opcode) << ")" << std::endl;
                        }
                    } catch (const std::bad_variant_access &) {
                        std::cerr << "Error in section " << section.getName() << ": Jump instruction at " << i << " has non-integer target" << std::endl;
                    }
                } else {
                    std::cerr << "Error in section " << section.getName() << ": Jump instruction at " << i << " missing target operand" << std::endl;
                }
            }
        }

        // 루프 컨텍스트에 기록된 라벨이 실제로 정의되었는지 확인
        for (const auto &loopContext : m_loopContextStack) {
            const std::vector<std::pair<std::string, std::string>> labelChecks = {{"시작", loopContext.startLabel},
                                                                                  {"조건", loopContext.condLabel},
                                                                                  {"본문", loopContext.bodyLabel},
                                                                                  {"증감", loopContext.postLabel},
                                                                                  {"끝", loopContext.endLabel}};

            for (const auto &[desc, label] : labelChecks) {
                auto it = m_labels.find(label);
                if (it == m_labels.end()) {
                    std::cerr << "Error: Loop '" << loopContext.loopLabel << "' " << desc << " 라벨 '" << label << "' not defined in label map" << std::endl;
                } else if (it->second < 0) {
                    std::cerr << "Error: Loop '" << loopContext.loopLabel << "' " << desc << " 라벨 '" << label << "' has temporary value" << std::endl;
                }
            }
        }
    }
}

BytecodeSection &BytecodeGenerator::getCurrentSection() { return m_sections[m_currentSectionIndex]; }

BytecodeGenerator::FunctionContext *BytecodeGenerator::getCurrentFunctionContext() {
    if (m_functionContextStack.empty()) {
        return nullptr;
    }
    return &m_functionContextStack.back();
}

BytecodeGenerator::FunctionContext *BytecodeGenerator::findFunctionContextByName(const std::string &name) {
    // 이름이 없으면 현재 함수 반환
    if (name.empty()) {
        return getCurrentFunctionContext();
    }

    // 이름으로 함수 찾기 (스택 상단부터 검색)
    for (auto it = m_functionContextStack.rbegin(); it != m_functionContextStack.rend(); ++it) {
        if (it->functionName == name) {
            return &(*it);
        }
    }

    // 찾지 못한 경우
    return nullptr;
}

void BytecodeGenerator::addInstruction(const BytecodeInstruction &instruction) { getCurrentSection().addInstruction(instruction); }

int BytecodeGenerator::allocateRegister() {
    // 가용한 레지스터가 있다면 재사용
    if (!m_freeRegisters.empty()) {
        int reg = m_freeRegisters.back();
        m_freeRegisters.pop_back();
        return reg;
    }

    // 새 레지스터 할당
    return m_nextRegister++;
}

void BytecodeGenerator::freeRegister(int regId) {
    // 레지스터 ID 유효성 검사
    if (regId < 0) {
        return;
    }

    // 이미 반환된 레지스터가 아닌지 확인
    auto it = std::find(m_freeRegisters.begin(), m_freeRegisters.end(), regId);
    if (it != m_freeRegisters.end()) {
        // 이미 반환된 레지스터
        std::cerr << "Warning: Register " << regId << " already freed" << std::endl;
        return;
    }

    // 레지스터 반환 (재사용 가능하게)
    m_freeRegisters.push_back(regId);
}

void BytecodeGenerator::resetRegisters() {
    // 모든 레지스터 초기화
    m_nextRegister = 0;
    m_freeRegisters.clear();
}

const std::vector<BytecodeSection> &BytecodeGenerator::getSections() const { return m_sections; }

std::string BytecodeGenerator::visit() const {
    std::stringstream ss;

    // 각 섹션 출력
    for (size_t i = 0; i < m_sections.size(); ++i) {
        const auto &section = m_sections[i];
        ss << "Section: " << section.getName() << "\n";

        // 섹션 내 명령어 출력
        const auto &instructions = section.getInstructions();
        for (size_t j = 0; j < instructions.size(); ++j) {
            const auto &instr = instructions[j];

            // 명령어 주소(오프셋)
            ss << std::setw(4) << std::setfill('0') << j << ": ";

            // 명령어 유형
            ss << opCodeToString(instr.opcode) << " ";

            // 레지스터 정보
            for (size_t k = 0; k < instr.registers.size(); ++k) {
                ss << "r" << instr.registers[k];
                if (k < instr.registers.size() - 1) {
                    ss << ", ";
                }
            }

            // 추가 피연산자 정보 (선택적)
            if (!instr.operands.empty()) {
                if (!instr.registers.empty()) {
                    ss << ", ";
                }

                for (size_t k = 0; k < instr.operands.size(); ++k) {
                    ss << operandToString(instr.operands[k]);
                    if (k < instr.operands.size() - 1) {
                        ss << ", ";
                    }
                }
            }

            ss << "\n";
        }

        ss << "\n";
    }

    return ss.str();
}

std::string BytecodeGenerator::generateUniqueLabel() { return "L_" + std::to_string(m_nextLabelId++); }

void BytecodeGenerator::pushLoopContext(const std::string &label) {
    LoopContext context;

    // 레이블 설정 (없으면 빈 문자열)
    context.loopLabel = label;

    // 고유한 레이블 생성 (레이블이 있으면 사용, 없으면 자동 생성)
    std::string loopId = !label.empty() ? label : "loop_" + std::to_string(m_nextLabelId++);

    // 루프 관련 레이블 생성
    context.startLabel = "start_" + loopId;
    context.condLabel = "cond_" + loopId;
    context.bodyLabel = "body_" + loopId;
    context.postLabel = "post_" + loopId;
    context.endLabel = "end_" + loopId;

    // 현재 명령어 인덱스 기록
    context.loopStartInstrIndex = getCurrentSection().getInstructions().size();
    context.loopEndInstrIndex = -1; // 아직 모름

    // 디버깅 로그 추가
    std::cerr << "Created loop context with label '" << loopId << "', start index: " << context.loopStartInstrIndex << std::endl;

    // 라벨 맵에 시작 위치 기록
    m_labels[context.startLabel] = context.loopStartInstrIndex;

    // 스택에 추가
    m_loopContextStack.push_back(context);
}

void BytecodeGenerator::popLoopContext() {
    if (!m_loopContextStack.empty()) {
        m_loopContextStack.pop_back();
    }
}

BytecodeGenerator::LoopContext *BytecodeGenerator::getCurrentLoopContext() {
    if (m_loopContextStack.empty()) {
        return nullptr;
    }
    return &m_loopContextStack.back();
}

BytecodeGenerator::LoopContext *BytecodeGenerator::findLoopContextByLabel(const std::string &label) {
    // 레이블이 없으면 현재 루프 반환
    if (label.empty()) {
        // 현재 루프 컨텍스트가 없는 경우를 확인
        if (m_loopContextStack.empty()) {
            return nullptr;
        }
        return getCurrentLoopContext();
    }

    // 레이블로 루프 찾기 (스택 상단부터 검색)
    for (auto it = m_loopContextStack.rbegin(); it != m_loopContextStack.rend(); ++it) {
        if (it->loopLabel == label) {
            return &(*it);
        }
    }

    // 찾지 못한 경우
    return nullptr;
}

void BytecodeGenerator::pushFunctionContext(const std::string &name) {
    FunctionContext context;

    // 함수 이름 설정 (없으면 빈 문자열)
    context.functionName = name;

    // 고유한 레이블 생성 (이름이 있으면 사용, 없으면 자동 생성)
    std::string funcId = !name.empty() ? name : "func_" + std::to_string(m_nextLabelId++);

    // 함수 관련 레이블 생성
    context.startLabel = "start_" + funcId;
    context.endLabel = "end_" + funcId;
    context.returnLabel = "return_" + funcId;

    // 현재 명령어 인덱스 기록
    context.functionStartIndex = getCurrentSection().getInstructions().size();
    context.functionEndIndex = -1; // 아직 모름

    // 반환 레지스터 초기화
    context.returnRegister = -1; // 아직 할당되지 않음

    // 스택에 추가
    m_functionContextStack.push_back(context);
}

void BytecodeGenerator::popFunctionContext() {
    if (!m_functionContextStack.empty()) {
        // 함수에서 사용한 지역 변수 레지스터 해제
        FunctionContext &context = m_functionContextStack.back();
        for (int regId : context.localVars) {
            freeRegister(regId);
        }

        m_functionContextStack.pop_back();
    }
}

void BytecodeGenerator::addJumpPatch(int instructionIndex, int operandIndex, const std::string &targetLabel) {
    JumpPatch patch;
    patch.instructionIndex = instructionIndex;
    patch.operandIndex = operandIndex;
    patch.targetLabel = targetLabel;
    m_jumpPatches.push_back(patch);
}

void BytecodeGenerator::resolveJumpPatches() {
    for (auto &section : m_sections) {
        resolveJumpPatchesForSection(section);
    }
    // 패치 적용 후 패치 목록 비우기
    m_jumpPatches.clear();
}

void BytecodeGenerator::resolveJumpPatchesForSection(BytecodeSection &section) {
    auto &instructions = const_cast<std::vector<BytecodeInstruction> &>(section.getInstructions());
    const int sectionSize = static_cast<int>(instructions.size());

    if (sectionSize == 0) {
        std::cerr << "Warning: Empty section '" << section.getName() << "', no patches to resolve" << std::endl;
        return;
    }

    // 안전한 기본 위치는 섹션의 마지막 명령어 (종료 지점)
    const int safeDefaultPosition = sectionSize - 1;

    std::cerr << "Resolving patches for section '" << section.getName() << "' with " << m_jumpPatches.size() << " patches and " << m_labels.size()
              << "
        labels "
              << std::endl;

    // 먼저 모든 레이블의 임시값을 안전한 값으로 변경
    for (auto &[label, pos] : m_labels) {
        if (pos < 0) {
            std::cerr << "Replacing temporary position for label '" << label << "' with safe position " << safeDefaultPosition << std::endl;
            pos = safeDefaultPosition;
        } else if (pos >= sectionSize) {
            std::cerr << "Position " << pos << " for label '" << label << "' is beyond section size " << sectionSize << ", adjusting to safe position "
                      << safeDefaultPosition << std::endl;
            pos = safeDefaultPosition;
        } else {
            std::cerr << "Label '" << label << "' has valid position " << pos << std::endl;
        }
    }

    // 패치 처리
    for (const auto &patch : m_jumpPatches) {
        // 명령어 인덱스 유효성 검사
        if (patch.instructionIndex < 0 || patch.instructionIndex >= sectionSize) {
            std::cerr << "Error: Invalid instruction index " << patch.instructionIndex << " for jump patch in section " << section.getName() << std::endl;
            continue;
        }

        auto &instr = instructions[patch.instructionIndex];

        // 점프 명령 형식 검증
        if (instr.opcode != BytecodeOpCode::JMP && instr.opcode != BytecodeOpCode::JMP_IF_TRUE && instr.opcode != BytecodeOpCode::JMP_IF_FALSE) {
            std::cerr << "Error: Non-jump instruction at " << patch.instructionIndex << " with opcode " << opCodeToString(instr.opcode) << std::endl;
            continue;
        }

        // 피연산자 인덱스 유효성 검사
        if (patch.operandIndex < 0 || patch.operandIndex >= static_cast<int>(instr.operands.size())) {
            std::cerr << "Error: Invalid operand index " << patch.operandIndex << " for instruction at " << patch.instructionIndex << std::endl;
            continue;
        }

        // 현재 점프 대상 값 확인
        int currentTarget = -1;
        try {
            currentTarget = std::get<int>(instr.operands[patch.operandIndex]);
        } catch (const std::bad_variant_access &) {
            std::cerr << "Error: Jump target is not an integer at " << patch.instructionIndex << std::endl;
            continue;
        }

        // 레이블 위치 조회
        auto labelIt = m_labels.find(patch.targetLabel);
        if (labelIt != m_labels.end()) {
            int targetPos = labelIt->second;

            // 타겟 위치 유효성 검사
            if (targetPos >= 0 && targetPos < sectionSize) {
                // 현재 타겟과 다른 경우에만 업데이트 (추가 로그 감소)
                if (targetPos != currentTarget) {
                    std::cerr << "Patching jump at " << patch.instructionIndex << " from " << currentTarget << " to " << targetPos
                              << " (label: " << patch.targetLabel << ")" << std::endl;
                    instr.operands[patch.operandIndex] = targetPos;
                }
            } else {
                std::cerr << "Warning: Label '" << patch.targetLabel << "' has invalid position " << targetPos << ", using safe position "
                          << safeDefaultPosition << std::endl;
                instr.operands[patch.operandIndex] = safeDefaultPosition;
            }
        } else {
            std::cerr << "Error: Label '" << patch.targetLabel << "' not found in label map, using safe position " << safeDefaultPosition << std::endl;
            instr.operands[patch.operandIndex] = safeDefaultPosition;
        }
    }

    // 최종 점프 대상 검증 및 로그
    for (size_t i = 0; i < instructions.size(); i++) {
        const auto &instr = instructions[i];
        if (instr.opcode == BytecodeOpCode::JMP || instr.opcode == BytecodeOpCode::JMP_IF_TRUE || instr.opcode == BytecodeOpCode::JMP_IF_FALSE) {
            if (!instr.operands.empty()) {
                try {
                    int target = std::get<int>(instr.operands[0]);
                    if (target < 0 || target >= sectionSize) {
                        std::cerr << "FATAL ERROR: Jump at " << i << " has invalid target " << target << " (outside valid range 0-" << (sectionSize - 1) << ")"
                                  << std::endl;
                        // 안전한 대상으로 수정
                        instructions[i].operands[0] = safeDefaultPosition;
                        std::cerr << "Corrected jump target to " << safeDefaultPosition << std::endl;
                    } else {
                        // 목적지의 명령어 타입 로그
                        std::cerr << "Valid jump at " << i << " (" << opCodeToString(instr.opcode) << ") -> " << target << " ("
                                  << opCodeToString(instructions[target].opcode) << ")" << std::endl;
                    }
                } catch (const std::bad_variant_access &) {
                    std::cerr << "Error: Jump target is not an integer at " << i << std::endl;
                }
            } else {
                std::cerr << "Error: Jump instruction at " << i << " has no operands" << std::endl;
            }
        }
    }
}

// 헬퍼 함수 구현
std::string opCodeToString(BytecodeOpCode opcode) {
    switch (opcode) {
    case BytecodeOpCode::LOAD_CONST:
        return "LOAD_CONST";
    case BytecodeOpCode::LOAD_VAR:
        return "LOAD_VAR";
    case BytecodeOpCode::STORE_VAR:
        return "STORE_VAR";
    case BytecodeOpCode::MOV_REG:
        return "MOV_REG";
    case BytecodeOpCode::ADD:
        return "ADD";
    case BytecodeOpCode::SUB:
        return "SUB";
    case BytecodeOpCode::MUL:
        return "MUL";
    case BytecodeOpCode::DIV:
        return "DIV";
    case BytecodeOpCode::MOD:
        return "MOD";
    case BytecodeOpCode::CMP_EQ:
        return "CMP_EQ";
    case BytecodeOpCode::CMP_NE:
        return "CMP_NE";
    case BytecodeOpCode::CMP_LT:
        return "CMP_LT";
    case BytecodeOpCode::CMP_LE:
        return "CMP_LE";
    case BytecodeOpCode::CMP_GT:
        return "CMP_GT";
    case BytecodeOpCode::CMP_GE:
        return "CMP_GE";
    case BytecodeOpCode::AND:
        return "AND";
    case BytecodeOpCode::OR:
        return "OR";
    case BytecodeOpCode::NOT:
        return "NOT";
    case BytecodeOpCode::JMP:
        return "JMP";
    case BytecodeOpCode::JMP_IF_TRUE:
        return "JMP_IF_TRUE";
    case BytecodeOpCode::JMP_IF_FALSE:
        return "JMP_IF_FALSE";
    case BytecodeOpCode::CALL:
        return "CALL";
    case BytecodeOpCode::RET:
        return "RET";
    case BytecodeOpCode::NOP:
        return "NOP";
    default:
        return "UNKNOWN";
    }
}

std::string operandToString(const BytecodeOperand &operand) {
    return std::visit(
        [](const auto &value) -> std::string {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, int>) {
                return std::to_string(value);
            } else if constexpr (std::is_same_v<T, float>) {
                return std::to_string(value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + value + "\"";
            } else if constexpr (std::is_same_v<T, Tag>) {
                return "tag:" + std::to_string(value.hash());
            } else {
                return "unknown";
            }
        },
        operand);
}

} // namespace nugdev::compiler::generation
```