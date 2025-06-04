# Expressions

nugdev 언어의 표현식(Expression)을 정의합니다. 표현식은 값을 가지며, 다른 표현식의 일부로 사용될 수 있습니다.

## Expression Types

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

## Primary Expressions

기본적인 표현식 형태들입니다.

### EBNF

```ebnf
primary_expression = identifier
                   | literal
                   | "(" expression ")"
                   | array_literal
                   | object_literal
                   | block_expression
                   | if_expression
                   | when_expression
                   | for_expression
                   | function_expression
                   | lambda_expression ;
```

### Examples

```nugdev
# 식별자
x
myVariable
user.name

# 리터럴
42
"hello"
true
null

# 괄호로 묶인 표현식
(2 + 3) * 4

# 배열과 객체 리터럴
[1, 2, 3]
{ name: "Alice", age: 30 }
{ .name = "Alice", .age = 30 }  # shorthand syntax

# 함수 표현식
fun(let x: number): number = x * 2

# 람다 표현식
(let x: number) => x * 2
(let a: number, let b: number) => a + b
```

## Assignment Expressions

값을 할당하는 표현식입니다. 할당된 값을 반환합니다.

### Examples

```nugdev
# 기본 할당
x = 42;        # 42를 반환
y = x = 10;    # 연쇄 할당: y = 10, x = 10

# 복합 할당
counter += 1;   # 증가된 값을 반환
total *= 2;     # 곱한 결과를 반환
flags |= mask;  # 비트 OR 결과를 반환

# 할당 표현식을 값으로 사용
let result: number = (score += bonus);
let check: boolean = (status = "active") == "active";

# 조건부 할당
level = if (exp > 1000) "expert" else "beginner";
name = user?.name ?? "Anonymous";
```

## Block Expressions

구문들의 블록이며, 마지막 표현식의 값을 반환합니다.

### EBNF

```ebnf
block_expression = "{" { statement } [ expression ] "}" ;
```

### Examples

```nugdev
# 기본 블록 표현식
let result: number = {
    let a: number = 10;
    let b: number = 20;
    a + b  # 마지막 표현식이 반환값 (30)
};

# 조건부 블록
let value: string = if (condition) {
    let temp: string = process(input);
    validate(temp);
    temp.result
} else {
    defaultValue
};

# 복잡한 블록
let processedData: Array<ProcessedItem> = {
    let raw: Array<RawItem> = fetchData();
    let filtered: Array<RawItem> = raw.filter((let x: RawItem) => x.valid);
    let transformed: Array<ProcessedItem> = filtered.map((let x: RawItem) => transform(x));
    let sorted: Array<ProcessedItem> = transformed.sort((let a: ProcessedItem, let b: ProcessedItem) => a.priority - b.priority);
    sorted  # 반환값
};
```

## Control Flow Expressions

제어 흐름도 표현식으로 값을 반환합니다.

### If Expression

### EBNF

```ebnf
if_expression = "if" "(" expression ")" expression [ "else" expression ] ;
```

### Examples

```nugdev
# 기본 if 표현식
let status: string = if (age >= 18) "adult" else "minor";

# 블록을 사용한 if
let result: string = if (score >= 90) {
    "Excellent!"
} else if (score >= 70) {
    "Good!"
} else {
    "Try harder!"
};

# 중첩된 if
let category: string = if (temperature > 30) {
    if (humidity > 80) "hot_humid" else "hot_dry"
} else {
    "mild"
};
```

### When Expression (Pattern Matching)

### EBNF

```ebnf
when_expression = "when" "(" expression ")" "{" 
                  { when_clause } 
                  [ else_clause ] 
                  "}" ;

when_clause = when_condition "->" ( statement | expression ) [ "," ] ;
else_clause = "else" "->" ( statement | expression ) ;

when_condition = value_condition
               | range_condition  
               | type_condition
               | guard_condition
               | multiple_condition ;

value_condition = expression ;
range_condition = expression "in" expression ;
type_condition = expression "is" type_literal ;
guard_condition = when_condition "if" expression ;
multiple_condition = when_condition { "," when_condition } ;
```

### Examples

```nugdev
# 기본 when 표현식 (코틀린 스타일)
let dayType: string = when (dayOfWeek) {
    1, 2, 3, 4, 5 -> "weekday",
    6, 7 -> "weekend",
    else -> "invalid"
}

# 값 매칭
let message: string = when (status) {
    200 -> "OK",
    404 -> "Not Found", 
    500 -> "Server Error",
    else -> "Unknown Status"
}

# 범위 매칭
let grade: string = when (score) {
    in 90..101 -> "A",
    in 80..90 -> "B", 
    in 70..80 -> "C",
    in 60..70 -> "D",
    else -> "F"
}

# 타입 매칭 (향후 확장)
let result: string = when (value) {
    is string -> "Text: ${value}",
    is number -> "Number: ${value}",
    is boolean -> "Boolean: ${value}",
    else -> "Unknown type"
}

# 가드 조건
let category: string = when (age) {
    x if x < 13 -> "child",
    x if x < 20 -> "teenager", 
    x if x < 65 -> "adult",
    else -> "senior"
}

# 블록 본문
let processedResult: any = when (inputType) {
    "json" -> {
        let parsed = JSON.parse(input)
        validate(parsed)
        transform(parsed)
    },
    "xml" -> {
        let parsed = parseXML(input) 
        convertToJson(parsed)
    },
    "csv" -> {
        let rows = parseCSV(input)
        rows.map((let row: any) => processRow(row))
    },
    else -> {
        throw Error("Unsupported input type: ${inputType}")
    }
}

# 복잡한 조건 매칭
let action: string = when (user) {
    u if u.role == "admin" && u.active -> "full_access",
    u if u.role == "user" && u.verified -> "limited_access", 
    u if u.role == "guest" -> "read_only",
    else -> "no_access"
}

# 여러 값 매칭
let httpMethod: string = when (request.method) {
    "GET", "HEAD" -> "read",
    "POST", "PUT", "PATCH" -> "write",
    "DELETE" -> "delete",
    else -> "unknown"
}
```

### Loop Expressions

### EBNF

```ebnf
for_expression = "for" "(" identifier "in" expression ")" expression ;
```

### Examples

```nugdev
# for 표현식 - 컬렉션 생성
let doubled: Array<number> = for (x in numbers) x * 2;
let filtered: Array<Item> = for (item in items) {
    if (item.active) item else continue
};

# 복잡한 루프 표현식
let processed: Array<ProcessedUser> = for (user in users) {
    let validated: ValidationResult = validate(user);
    if (!validated.valid) continue;
    
    {
        id: user.id,
        name: validated.name,
        email: validated.email
    }
};
```

## Function Expressions

함수 정의도 표현식입니다.

### EBNF

```ebnf
function_expression = [ label ] "fun" "(" [ parameter_list ] ")" ":" type_literal function_expression_body ;
function_expression_body = "{" { statement } [ expression ] "}"
                         | "=" expression ;

lambda_expression = [ label ] "(" [ lambda_parameter_list ] ")" "=>" expression ;
lambda_parameter_list = lambda_parameter { "," lambda_parameter } ;
lambda_parameter = ( "let" | "mut" ) identifier ":" type_literal [ "=" expression ] ;

parameter_list = parameter { "," parameter } ;
parameter = ( "let" | "mut" ) identifier ":" type_literal [ "=" expression ] ;

label = identifier "@" ;
```

### Examples

```nugdev
# 익명 함수 표현식 (블록 본문)
let add: (number, number) -> number = fun(let a: number, let b: number): number {
    return a + b
}

# 익명 함수에서 레이블이 필요한 경우에만 사용
let calculator: (number, number, string) -> number = calculation@ fun(let a: number, let b: number, let op: string = "+"): number {
    when (op) {
        "+" -> return@calculation a + b,
        "-" -> return@calculation a - b,
        "*" -> return@calculation a * b,
        "/" -> {
            if (b == 0) return@calculation 0
            return@calculation a / b
        },
        else -> return@calculation 0
    }
}

# 기본값 매개변수가 있는 익명 함수
let greet: (string, string) -> string = fun(let name: string, let greeting: string = "Hello"): string {
    return `${greeting}, ${name}!`
}

# 익명 함수 표현식 (표현식 본문)
let multiply: (number, number) -> number = fun(let x: number, let y: number = 1): number = x * y

# 라벨이 있는 함수 표현식
let calculator: (number, number, string) -> number = calculation@ fun(let a: number, let b: number, let op: string = "+"): number {
    when (op) {
        "+": return@calculation a + b,
        "-": return@calculation a - b,
        "*": return@calculation a * b,
        "/": {
            if (b == 0) return@calculation 0
            return@calculation a / b
        },
        else: return@calculation 0
    }
}

# 람다 표현식
let square: (number) -> number = (let x: number) => x * x
let increment: (number) -> number = (let x: number, let step: number = 1) => x + step
let sum: (number, number) -> number = (let a: number, let b: number = 0) => a + b

# 기본값이 있는 람다 표현식
let formatter: (string, string, boolean) -> string = (let text: string, let prefix: string = "[INFO]", let uppercase: boolean = false) => {
    let result: string = `${prefix} ${text}`
    return uppercase ? result.toUpperCase() : result
}

# 라벨이 있는 람다 표현식
let validator: (string, number) -> boolean = validation@ (let input: string, let maxLength: number = 100) => {
    if (input.length == 0) return@validation false
    if (input.length > maxLength) return@validation false
    return@validation true
}

# 복잡한 람다 표현식 with 기본값
let filter: (Array<any>, (any) -> boolean, boolean) -> Array<any> = (
    let arr: Array<any>, 
    let pred: (any) -> boolean = (let x: any) => true, 
    let reverse: boolean = false
) => {
    mut result: Array<any> = []
    for (item in arr) {
        if (pred(item)) result.push(item)
    }
    return reverse ? result.reverse() : result
}

# 고차 함수 with 기본값
let createAdder: (number, number) -> (number) -> number = fun(let base: number, let multiplier: number = 1): (number) -> number = {
    (let x: number) => (x + base) * multiplier
}

let add10: (number) -> number = createAdder(10)  # multiplier=1 기본값
let add10AndDouble: (number) -> number = createAdder(10, 2)  # 기본값 덮어쓰기
let result: number = add10(5)  # 15

# 클로저 with 기본값
let createCounter: (number, number) -> () -> number = fun(let start: number = 0, let step: number = 1): () -> number {
    mut count: number = start
    () => {
        count += step
        count
    }
}

# 다양한 기본값 패턴
let processData: (any, ProcessOptions, (string) -> void) -> any = fun(
    let data: any,
    let options: ProcessOptions = { strict: false, timeout: 5000 },
    let logger: (string) -> void = (let msg: string) => console.log(msg)
): any = {
    logger("Starting data processing...")
    
    let processed: any = when (options.strict) {
        true: strictProcess(data),
        false: relaxedProcess(data)
    }
    
    logger("Data processing completed")
    return processed
}

# 조건부 기본값
let connect: (string, number, boolean) -> Connection = fun(
    let host: string,
    let port: number = if (isDevelopment) 3000 else 80,
    let secure: boolean = port == 443
): Connection = {
    return createConnection(host, port, secure)
}
```

## Call Expressions

함수나 메서드를 호출하는 표현식입니다.

### EBNF

```ebnf
call_expression = primary_expression "(" [ argument_list ] ")" ;
method_call = primary_expression "." identifier "(" [ argument_list ] ")" ;
argument_list = expression { "," expression } ;
```

### Examples

```nugdev
# 함수 호출
let sum: number = add(10, 20);
let result: CalculationResult = calculate(x, y, z);

# 메서드 호출
let length: number = text.length();
let upper: string = text.toUpperCase();

# 체이닝
let processed: string = data
    .filter((let x: Item) => x.active)
    .map((let x: Item) => x.name)
    .sort()
    .join(", ");

# 안전한 호출
let safeResult: any = obj?.method?.(arg) ?? defaultValue;

# 람다를 인자로 전달
let mapped: Array<string> = users.map((let u: User) => u.name);
let filtered: Array<User> = users.filter((let u: User) => u.active);
let sorted: Array<User> = users.sort((let a: User, let b: User) => a.name.localeCompare(b.name));
```

## Member Access Expressions

객체의 프로퍼티나 배열의 요소에 접근하는 표현식입니다.

### Examples

```nugdev
# 점 표기법
let name: string = user.name;
let email: string = user.profile.email;

# 대괄호 표기법
let value: any = object["property"];
let item: any = array[index];

# 동적 접근
let key: string = "status";
let status: UserStatus = user[key];

# 안전한 접근
let safeName: string? = user?.name;
let safeEmail: string? = user?.profile?.email;

# 배열 슬라이싱
let slice: Array<number> = array[1..5];
let tail: Array<number> = array[1..];
let head: Array<number> = array[..3];
```

## Expression Composition

표현식들의 복합 사용 예시입니다.

### Examples

```nugdev
# 복잡한 표현식 조합
let result: Array<EnhancedUser> = users
    .filter((let u: User) => u.active and u.verified)
    .map((let u: User) => {
        let score: number = calculateScore(u);
        let level: string = if (score > 100) "expert" else "beginner";
        { 
            id: u.id,
            name: u.name,
            email: u.email,
            score: score, 
            level: level 
        }
    })
    .sort((let a: EnhancedUser, let b: EnhancedUser) => b.score - a.score)
    .take(10);

# 조건부 표현식 체이닝
let value: ProcessedData = input
    |> validateInput
    |> ((let x: ValidatedInput) => { 
        if (x.valid) x.data else throw Error("Invalid") 
    })
    |> transform
    |> ((let x: TransformedData) => x ?? defaultValue);

# 중첩된 표현식
let config: Config = {
    environment: when (process.env.NODE_ENV) {
        "production": "prod",
        "development": "dev", 
        else: "test"
    },
    database: {
        .host = process.env.DB_HOST ?? "localhost",
        .port = parseInt(process.env.DB_PORT ?? "5432"),
        .name = if (isTest) "test_db" else "main_db"
    },
    features: [
        for (feature in availableFeatures) {
            if (isEnabled(feature)) feature
        }
    ]
};
``` 