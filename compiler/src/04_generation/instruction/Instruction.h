#pragma once

#include "00_app/lib/PointerHelper.hpp"
#include "00_app/tag/Tag.h"
#include "04_generation/register/Register.hpp"

#include <cstdint>
#include <unicode/unistr.h>

namespace nugdev::compiler::generation {
/**
 * @brief 데이터 섹션 필드
 * @note 출력시: <tag> <type> <literal|array|string>
 */
struct DataSectionField {
    struct DataSectionFieldValue : lib::PointerHelper<DataSectionFieldValue> {
        virtual ~DataSectionFieldValue(){};
        enum class Type {
            Array,
            Literal,
            String,
        };
        Type m_type;
        union {
            // 64비트 미만의 리터럴 표현식
            std::uint64_t m_literal;
            // array 표현식
            std::vector<std::shared_ptr<DataSectionFieldValue>> m_array;
            // 문자열 표현식
            icu::UnicodeString m_string;
        };
    };
    struct DataScetionFieldTag : public Tag {};

    DataScetionFieldTag m_tag;
    std::shared_ptr<DataSectionFieldValue> m_value;
};

/**
 * @brief 메모리 표현식
 */
struct MemoryExpression : public lib::PointerHelper<MemoryExpression> {};
/**
 * @brief 데이터 섹션에 대한 표현식
 * @note Memory[field_tag: DataScetionFieldTag]
 */
struct DataSectionMemoryExpression : public MemoryExpression {
    DataSectionField::DataScetionFieldTag field_tag;
};
/**
 * @brief 랜덤 액세스 메모리 표현식
 * @note Memory[address: std::uint64_t]
 */
struct RandomAccessMemoryExpression : public MemoryExpression {
    std::uint64_t address;
};

struct Instruction : public lib::PointerHelper<Instruction> {};

/**
 * @brief 레지스터 간 이동
 * @param R(x): 목적지 레지스터
 * @param R(y): 출발지 레지스터
 * @note R(x) <- R(y)
 */
struct Move : public Instruction {
    RegisterTag destination;
    RegisterTag source;
};

/**
 * @brief 데이터 섹션 로드
 * @param R(x): 목적지 레지스터
 * @param DataSectionMemoryExpression: 데이터 섹션 표현식
 * @note R(x) <- DataSectionMemoryExpression
 */
struct Load : public Instruction {
    RegisterTag destination;
    DataSectionMemoryExpression dataSection;
};

/**
 * @brief 더하기.
 * @param R(x): 목적지 레지스터
 * @param R(y): 출발지 레지스터
 * @note R(x) <- R(x) + R(y)
 */
struct Add : public Instruction {
    RegisterTag destination;
    RegisterTag source;
};

/**
 * @brief 빼기.
 * @param R(x): 목적지 레지스터
 * @param R(y): 출발지 레지스터
 * @note R(x) <- R(x) - R(y)
 */
struct Sub : public Instruction {
    RegisterTag destination;
    RegisterTag source;
};

/**
 * @brief 곱하기.
 * @param R(x): 목적지 레지스터
 * @param R(y): 출발지 레지스터
 * @note R(x) <- R(x) * R(y)
 */
struct Mul : public Instruction {
    RegisterTag destination;
    RegisterTag source;
};

/**
 * @brief 나누기.
 * @param R(x): 목적지 레지스터
 * @param R(y): 출발지 레지스터
 * @note R(x) <- R(x) / R(y)
 */
struct Div : public Instruction {
    RegisterTag destination;
    RegisterTag source;
};

/**
 * @brief 나머지.
 * @param R(x): 목적지 레지스터
 * @param R(y): 출발지 레지스터
 * @note R(x) <- R(x) % R(y)
 */
struct Mod : public Instruction {
    RegisterTag destination;
    RegisterTag source;
};

/**
 * @brief 증가.
 * @param R(x): 목적지 레지스터
 * @note R(x) <- R(x) + 1
 */
struct Inc : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 감소.
 * @param R(x): 목적지 레지스터
 * @note R(x) <- R(x) - 1
 */
struct Dec : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 비교.
 * @param R(left): 왼쪽 레지스터
 * @param R(right): 오른쪽 레지스터
 * @return -> flag 레지스터
 * @note R(left) - R(right)
 */
struct Cmp : public Instruction {
    RegisterTag left;
    RegisterTag right;
};

/**
 * @brief 비트 비교
 * @param R(left): 왼쪽 레지스터
 * @param R(right): 오른쪽 레지스터
 * @return -> flag 레지스터
 * @note R(left) & R(right)
 */
struct Test : public Instruction {
    RegisterTag left;
    RegisterTag right;
};

/**
 * @brief jump
 * @param R(x): 목적지 레지스터
 * @note PC <- R(x)
 */
struct Jump : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 비교 결과가 같으면 jump
 * @param R(x): 목적지 레지스터
 * @note PC <- (if ZF=1) 이전 CMP 결과가 같으면 점프
 */
struct JumpEq : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 비교 결과가 같지 않으면 jump
 * @param R(x): 목적지 레지스터
 * @note PC <- (if ZF=0) 이전 CMP 결과가 같지 않으면 점프
 */
struct JumpNe : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 비교 결과가 작으면 jump
 * @param R(x): 목적지 레지스터
 * @note PC <- (if SF≠OF) 이전 CMP 결과 R(x) < R(y)이면 점프
 */
struct JumpLt : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 비교 결과가 크면 jump
 * @param R(x): 목적지 레지스터
 * @note PC <- (if ZF=0 AND SF=OF) 이전 CMP 결과 R(x) > R(y)이면 점프
 */
struct JumpGt : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 비교 결과가 작거나 같으면 jump
 * @param R(x): 목적지 레지스터
 * @note PC <- (if ZF=1 OR SF≠OF) 이전 CMP 결과 R(x) <= R(y)이면 점프
 */
struct JumpLe : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 비교 결과가 크거나 같으면 jump
 * @param R(x): 목적지 레지스터
 * @note PC <- (if SF=OF) 이전 CMP 결과 R(x) >= R(y)이면 점프
 */
struct JumpGe : public Instruction {
    RegisterTag destination;
};

/**
 * @brief 함수 호출
 * @param section_name: 함수 이름
 * @note PC <- section_name
 */
struct Call : public Instruction {
    icu::UnicodeString section_name;
};

/**
 * @brief 함수 반환
 * @note PC <- POP(); R(x) <- R(y) # 무조건 R(y)가 있음.
 */
struct Return : public Instruction {
    RegisterTag destination;
};

} // namespace nugdev::compiler::generation