# Literals and Basic Types

리터럴은 소스 코드에서 직접 표현되는 값들입니다. 모든 리터럴은 표현식입니다.

## Number Literals

### EBNF

```ebnf
number_literal = integer_literal | floating_point_literal ;

integer_literal = decimal_integer 
                | binary_integer 
                | octal_integer 
                | hexadecimal_integer ;

decimal_integer = DIGIT { DIGIT | "_" } ;
binary_integer = "0b" BINARY_DIGIT { BINARY_DIGIT | "_" } ;
octal_integer = "0o" OCTAL_DIGIT { OCTAL_DIGIT | "_" } ;
hexadecimal_integer = "0x" HEX_DIGIT { HEX_DIGIT | "_" } ;

floating_point_literal = DIGIT { DIGIT | "_" } "." { DIGIT | "_" } [ exponent ]
                       | DIGIT { DIGIT | "_" } exponent ;

exponent = ( "e" | "E" ) [ "+" | "-" ] DIGIT { DIGIT } ;
```

### Examples

```nugdev
# 정수 리터럴
let decimal: number = 42;
let large: number = 1_000_000;  # 언더스코어로 가독성 향상

# 다양한 진법
let binary: number = 0b1010_1100;
let octal: number = 0o755;
let hex: number = 0xFF_FF_FF;

# 부동소수점 리터럴
let pi: number = 3.14159;
let avogadro: number = 6.022e23;
let planck: number = 6.626_070_15e-34;

# 과학적 표기법
let lightSpeed: number = 2.998E8;
let electronMass: number = 9.109e-31;
```

## String Literals

### EBNF

```ebnf
string_literal = simple_string | raw_string | template_string ;

simple_string = "\"" { string_character } "\"" ;
string_character = ESCAPE_SEQUENCE | ~( "\"" | "\\" | NEWLINE ) ;

raw_string = "r\"" { ~"\"" } "\"" ;

template_string = "`" { template_character | template_expression } "`" ;
template_character = ESCAPE_SEQUENCE | ~( "`" | "$" ) ;
template_expression = "${" expression "}" ;

ESCAPE_SEQUENCE = "\\" ( "n" | "t" | "r" | "\\" | "\"" | "'" | "0" | "u" HEX_DIGIT{4} ) ;
```

### Examples

```nugdev
# 기본 문자열
let message: string = "Hello, World!";
let path: string = "C:\\Users\\nugdev";

# 이스케이프 시퀀스
let escaped: string = "Line 1\nLine 2\tTabbed";
let quote: string = "He said \"Hello\"";
let unicode: string = "Unicode: \u03A9";  # Ω

# 원시 문자열 (이스케이프 처리 안함)
let regex: string = r"\d+\.\d+";
let windowsPath: string = r"C:\Users\nugdev\Documents";

# 문자열 보간 (템플릿 리터럴)
let name: string = "nugdev";
let age: number = 25;
let greeting: string = `Hello, ${name}! You are ${age} years old.`;

# 표현식 포함
let calculation: string = `2 + 3 = ${2 + 3}`;
let conditional: string = `Status: ${isActive ? "Active" : "Inactive"}`;
```

## Boolean Literals

### EBNF

```ebnf
boolean_literal = "true" | "false" ;
```

### Examples

```nugdev
# 불린 리터럴
let isActive: boolean = true;
let isComplete: boolean = false;

# 불린 표현식과 결합
let canAccess: boolean = true and hasPermission;
let shouldExit: boolean = false or hasError;

# 조건문에서 사용
let status: string = if (isActive) "running" else "stopped";
```

## Null and None Literals

null 값과 None 값을 나타내는 리터럴입니다.

### EBNF

```ebnf
null_literal = "null" ;
none_literal = "None" ;
```

### Examples

```nugdev
# null 값 (JavaScript/TypeScript 호환)
let data: any = null;
let emptyRef: object? = null;

# None 값 (옵셔널/Option 타입용)
let result: string? = None;
let user: User? = None;

# 함수가 void 반환 (값이 아님, 타입만)
let logMessage: (string) -> void = fun(let msg: string): void {
    console.log(msg);
};

# void는 값이 아니므로 변수에 할당 불가
# let nothing: void = ...;  // 컴파일 에러
```

## Character Literals

### EBNF

```ebnf
character_literal = "'" ( ESCAPE_SEQUENCE | ~( "'" | "\\" | NEWLINE ) ) "'" ;
```

### Examples

```nugdev
# 문자 리터럴
let letter: string = 'A';
let digit: string = '5';
let newline: string = '\n';
let tab: string = '\t';
let unicode: string = '\u03A9';  # Ω

# 문자 배열
let chars: Array<string> = ['H', 'e', 'l', 'l', 'o'];
let vowels: Array<string> = ['a', 'e', 'i', 'o', 'u'];
```

## Range Literals

### EBNF

```ebnf
range_literal = expression ".." [ expression ] ;
```

### Examples

```nugdev
# 범위 리터럴 (exclusive - 마지막 값 제외)
let numbers: any = 1..10;       # 1부터 9까지 (10 제외)
let chars: any = 'a'..'z';      # 'a'부터 'y'까지 ('z' 제외)
let unbounded: any = 10..;      # 10부터 무한대

# for 루프에서 사용
for (i in 1..5) {
    print(i);  # 1, 2, 3, 4 출력 (5는 제외)
}

# 10까지 포함하려면 +1
for (i in 1..11) {
    print(i);  # 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 출력
}

# 배열 슬라이싱
let slice: Array<any> = array[2..5];    # 인덱스 2, 3, 4 (5 제외)
let tail: Array<any> = array[1..];      # 인덱스 1부터 끝까지

# 문자열 슬라이싱  
let substring: string = text[0..5];     # 0, 1, 2, 3, 4번째 문자 (5번째 제외)
```

# 개정된 타입 시스템

## void vs None vs null 구분

```nugdev
# void: 함수가 의미 있는 값을 반환하지 않음 (반환 타입 전용)
fun logMessage(let message: string): void {
    console.log(message)
}

# None: 옵셔널 값이 없음을 나타냄 (값으로 사용 가능)
let result: User? = None
let user: User? = findUser(id) ?? None

# null: 명시적 null 참조 (JavaScript/TypeScript 호환)
let data: any = null
```

## None Literal

값이 존재하지 않음을 나타내는 리터럴입니다.

### EBNF

```ebnf
none_literal = "None" ;
```

### Examples

```nugdev
# Option 타입과 함께 사용
let maybeUser: User? = None
let result: User? = None

# 함수에서 반환
fun findById(let id: number): User? {
    if (id < 0) return None
    return database.find(id)
}

# 패턴 매칭에서 사용
let message: string = when (maybeUser) {
    None -> "User not found",
    user -> `Hello, ${user.name}!`
}
``` 