# nugdev Language Grammar

nugdev 언어는 **Program → Module → Statement → Expression** 구조를 따르는 모던 시스템 프로그래밍 언어입니다.

## Architecture Overview

```
Program
├── Module (파일 단위)
│   ├── Statement (값을 반환하지 않음)
│   │   ├── Variable Declaration
│   │   ├── Function Declaration  
│   │   ├── Control Statement (break/continue/return)
│   │   └── Expression Statement
│   └── Expression (값을 반환함)
│       ├── Literals, Variables, Operators
│       ├── Function/Lambda Expressions
│       ├── Control Flow (if/when/for expressions)
│       └── Collections (Array/Object)
```

### Key Distinctions

```nugdev
# Statement: 값을 반환하지 않음
let x: number = 42
fun compute(): number { return 1 }

# Expression: 값을 반환함
x + 1
if (condition) "yes" else "no"
when (value) { 1 -> "one", else -> "other" }

# Expression Statement: 표현식을 구문으로 사용
print("Hello")
```

## Language Structure

```
Program → Module → Statement → Expression
```

- **Program**: 여러 모듈의 집합
- **Module**: 하나의 소스 파일을 의미
- **Statement**: 구문이지만 값으로 표현될 수 없는 구조
- **Expression**: 값을 표현하는 식

## Core Philosophy

Statement와 Expression을 명확히 구분하여 언어의 의미를 명확하게 합니다.

```nugdev
// Statement: 값을 반환하지 않음
let x = 42;
mut counter = 0;

// Expression: 값을 반환함
let result = if (x > 0) "positive" else "negative";
let sum = add(10, 20);
let doubled = numbers.map(|x| x * 2);

// Expression Statement: 표현식을 구문으로 사용
print("Hello, World!");
counter += 1;
```

## Statement Types

- `let` / `mut` - 변수 선언문
- `continue` / `break` / `return` - 제어 흐름문
- Expression Statement - 표현식을 구문으로 사용

## Grammar Documentation Structure

- [Program Structure](./program.md) - 프로그램과 모듈 구조
- [Statements](./statements.md) - 구문 정의
- [Expressions](./expressions.md) - 표현식 정의
- [Control Flow](./control-flow.md) - 제어 흐름 표현식
- [Functions](./functions.md) - 함수 정의와 호출
- [Type System](./type-system.md) - 구조체와 인터페이스 (타입 안전성)
- [Type Casting](./type-casting.md) - 타입 검사와 캐스팅 (is, as, as?)
- [Literals](./literals.md) - 리터럴과 기본 타입
- [Operators](./operators.md) - 연산자 우선순위
- [Collections](./collections.md) - 배열, 객체 리터럴
- [Examples](./examples.md) - 종합 예시

## EBNF Root Production

```ebnf
program = { module } ;
module = { statement } ;
statement = variable_declaration | control_statement | expression_statement ;
expression = assignment_expression ;
```

모든 nugdev 프로그램은 모듈들의 집합이며, 각 모듈은 구문들의 연속입니다. 

# Control Flow 예시
let result: string = if (age >= 18) "adult" else "minor"

let grade: string = when (score) {
    in 90..101 -> "A",
    in 80..90 -> "B", 
    in 70..80 -> "C",
    else -> "F"
} 