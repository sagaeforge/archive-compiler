# Operators

연산자 우선순위와 결합성을 정의합니다. 모든 연산자는 표현식이므로 값을 반환합니다.

## Operator Precedence (High to Low)

### EBNF

```ebnf
expression = assignment_expression ;

assignment_expression = ternary_expression [ assignment_operator assignment_expression ] ;
assignment_operator = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" | "~=" ;

ternary_expression = null_coalescing_expression [ "?" expression ":" expression ] ;

null_coalescing_expression = logical_or_expression { "??" logical_or_expression } ;

logical_or_expression = logical_and_expression { "or" logical_and_expression } ;

logical_and_expression = bitwise_or_expression { "and" bitwise_or_expression } ;

bitwise_or_expression = bitwise_xor_expression { "|" bitwise_xor_expression } ;

bitwise_xor_expression = bitwise_and_expression { "^" bitwise_and_expression } ;

bitwise_and_expression = equality_expression { "&" equality_expression } ;

equality_expression = relational_expression { ( "==" | "!=" ) relational_expression } ;

relational_expression = shift_expression { ( "<" | ">" | "<=" | ">=" ) shift_expression } ;

shift_expression = additive_expression { ( "<<" | ">>" ) additive_expression } ;

additive_expression = multiplicative_expression { ( "+" | "-" ) multiplicative_expression } ;

multiplicative_expression = unary_expression { ( "*" | "/" | "%" ) unary_expression } ;

unary_expression = ( "+" | "-" | "!" | "not" | "~" | "++" | "--" ) unary_expression
                 | postfix_expression ;

postfix_expression = primary_expression { postfix_operator } ;
postfix_operator = "++" 
                 | "--" 
                 | "." identifier
                 | "?." identifier  (* null-safe access *)
                 | "[" expression "]"
                 | "(" [ argument_list ] ")" ;
```

## Assignment Operators

### Examples

```nugdev
# 기본 대입
x = 42
name = "Alice"

# 복합 대입 - 모두 값을 반환
counter = 0
let a = (counter += 1)  # counter = 1, a = 1
let b = (counter *= 2)  # counter = 2, b = 2
let c = (counter -= 1)  # counter = 1, c = 1

# 비트 연산 복합 대입
flags = 0b1010
flags |= 0b0101   # OR 연산: 1111
flags &= 0b1100   # AND 연산: 1100
flags ^= 0b0011   # XOR 연산: 1111

# 복잡한 할당
data.field = processValue(input)
array[index] += calculateDelta()
```

## Ternary and Null Operators

### Examples

```nugdev
# 삼항 연산자
let status: string = age >= 18 ? "adult" : "minor"
let max: number = a > b ? a : b

# 중첩 삼항
let grade: string = score >= 90 ? "A" :
                   score >= 80 ? "B" :
                   score >= 70 ? "C" : "F"

# Null 병합 연산자
let name: string = user.name ?? "Anonymous"
let config: Config = userConfig ?? defaultConfig

# 체이닝
let value: any = data?.user?.profile?.name ?? "Unknown"
```

## Logical Operators

### EBNF

```ebnf
logical_or_expression = logical_and_expression { "or" logical_and_expression } ;
logical_and_expression = bitwise_or_expression { "and" bitwise_or_expression } ;
```

### Examples

```nugdev
# 논리 연산자 (단축 평가)
let canAccess: boolean = isLoggedIn and hasPermission
let shouldNotify: boolean = isImportant or isUrgent
let isValid: boolean = not isEmpty and not isExpired

# 복잡한 논리 표현식
let canEdit: boolean = isOwner or (isMember and hasEditPermission)

# 논리 연산자를 함수처럼 사용
let allValid: boolean = items.every((let item: Item) => isValid(item) and isActive(item))
```

## Arithmetic Operators

### EBNF

```ebnf
additive_expression = multiplicative_expression { ( "+" | "-" ) multiplicative_expression } ;
multiplicative_expression = unary_expression { ( "*" | "/" | "%" ) unary_expression } ;
unary_expression = ( "+" | "-" | "!" | "not" | "~" | "++" | "--" ) unary_expression
                 | postfix_expression ;
```

### Examples

```nugdev
# 기본 산술 연산
let sum: number = a + b
let difference: number = a - b
let product: number = a * b
let quotient: number = a / b
let remainder: number = a % b

# 단항 연산자
let positive: number = +value
let negative: number = -value
let incremented: number = ++counter  # 전위 증가
let decremented: number = counter--  # 후위 감소

# 복잡한 산술 표현식
let result: number = (a + b) * (c - d) / (e + f)
let average: number = (x + y + z) / 3
let distance: number = Math.sqrt(dx * dx + dy * dy)
```

## Comparison Operators

### EBNF

```ebnf
equality_expression = relational_expression { ( "==" | "!=" ) relational_expression } ;
relational_expression = shift_expression { ( "<" | ">" | "<=" | ">=" ) shift_expression } ;
```

### Examples

```nugdev
# 관계 연산자
let isEqual: boolean = a == b
let isNotEqual: boolean = a != b
let isLess: boolean = a < b
let isGreater: boolean = a > b
let isLessOrEqual: boolean = a <= b
let isGreaterOrEqual: boolean = a >= b

# 체이닝 비교 (향후 확장)
let inRange: boolean = 0 <= value <= 100  # 0 <= value and value <= 100

# 타입 비교 (향후 확장)
let isString: boolean = value is string
let isNumber: boolean = value is number
```

## Bitwise Operators

### EBNF

```ebnf
bitwise_or_expression = bitwise_xor_expression { "|" bitwise_xor_expression } ;
bitwise_xor_expression = bitwise_and_expression { "^" bitwise_and_expression } ;
bitwise_and_expression = equality_expression { "&" equality_expression } ;
shift_expression = additive_expression { ( "<<" | ">>" ) additive_expression } ;
```

### Examples

```nugdev
# 비트 연산자
let bitwiseAnd: number = a & b      # AND
let bitwiseOr: number = a | b       # OR
let bitwiseXor: number = a ^ b      # XOR
let bitwiseNot: number = ~a         # NOT
let leftShift: number = a << 2      # 왼쪽 시프트
let rightShift: number = a >> 2     # 오른쪽 시프트

# 비트 마스킹
let flags: number = 0b1010
let hasFlag: boolean = (flags & 0b0010) != 0
let setFlag: number = flags | 0b0100
let clearFlag: number = flags & ~0b0010
let toggleFlag: number = flags ^ 0b1000
```

## Member Access Operators

### EBNF

```ebnf
postfix_operator = "++" 
                 | "--" 
                 | "." identifier
                 | "?." identifier  (* null-safe access *)
                 | "[" expression "]"
                 | "(" [ argument_list ] ")" ;
```

### Examples

```nugdev
# 멤버 접근
let name: string = user.name
let score: number = game.player.score

# 안전한 멤버 접근 (null-safe)
let email: string? = user?.profile?.email
let length: number = text?.length ?? 0

# 배열/인덱스 접근
let firstItem: any = array[0]
let lastItem: any = array[array.length - 1]

# 동적 멤버 접근
let property: any = object[propertyName]
let computed: any = data[`${prefix}_${key}`]

# 체이닝
let result: any = api.getData()
    .filter((let item: any) => item.active)
    .map((let item: any) => item.name)
    .join(", ")
```

## Operator Overloading (향후 확장)

```nugdev
# 사용자 정의 타입에 대한 연산자 오버로딩
struct Vector {
    x: number
    y: number
    
    operator +(let other: Vector): Vector {
        Vector { x: this.x + other.x, y: this.y + other.y }
    }
    
    operator *(let scalar: number): Vector {
        Vector { x: this.x * scalar, y: this.y * scalar }
    }
    
    operator ==(let other: Vector): boolean {
        this.x == other.x and this.y == other.y
    }
}

let v1: Vector = Vector { x: 1, y: 2 }
let v2: Vector = Vector { x: 3, y: 4 }
let sum: Vector = v1 + v2  # Vector { x: 4, y: 6 }
``` 