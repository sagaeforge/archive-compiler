# Statements

nugdev 언어의 구문(Statement)을 정의합니다. 구문은 값을 반환하지 않으며, 세미콜론은 선택사항입니다.

## Statement Types

### EBNF

```ebnf
statement = variable_declaration
          | function_declaration
          | control_statement  
          | expression_statement ;

variable_declaration = ( "let" | "mut" ) identifier ":" type_literal [ ";" ]
                     | ( "let" | "mut" ) identifier "=" expression [ ";" ]
                     | ( "let" | "mut" ) identifier ":" type_literal "=" expression [ ";" ] ;

function_declaration = "fun" identifier "(" [ parameter_list ] ")" ":" type_literal function_body ;
function_body = "{" { statement } "}"
              | "=" expression [ ";" ] ;

parameter_list = parameter { "," parameter } ;
parameter = ( "let" | "mut" ) identifier ":" type_literal [ "=" expression ] ;

control_statement = break_statement | continue_statement | return_statement ;
break_statement = "break" [ "@" identifier ] [ ";" ] ;
continue_statement = "continue" [ "@" identifier ] [ ";" ] ;
return_statement = "return" [ "@" identifier ] [ expression ] [ ";" ] ;

expression_statement = expression [ ";" ] ;

label = identifier "@" ;
```

## Variable Declaration Statements

변수를 선언하는 구문입니다.

### EBNF

```ebnf
variable_declaration = ( "let" | "mut" ) identifier ":" type_literal [ ";" ]
                     | ( "let" | "mut" ) identifier "=" expression [ ";" ]
                     | ( "let" | "mut" ) identifier ":" type_literal "=" expression [ ";" ] ;
```

### Examples

```nugdev
// 타입만 명시 (초기화 없음)
let name: string;
let age: number;
mut counter: number;

// 값만 명시 (타입 추론)
let greeting = "Hello, World!";
let count = 42;
mut isActive = true;

// 타입과 값 모두 명시
let message: string = "Welcome";
let score: number = 95;
mut config: object = { debug: false };

// 복잡한 타입
let users: Array<User> = [];
mut cache: Map<string, any> = new Map();
let handler: (string, number) -> boolean = validateInput;

// 객체와 배열 초기화
let user: User = {
    name: "Alice",
    email: "alice@example.com",
    roles: ["user", "admin"]
};

let matrix: Array<Array<number>> = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];
```

## Function Declaration Statements

함수를 정의하는 구문입니다.

### EBNF

```ebnf
function_declaration = "fun" identifier "(" [ parameter_list ] ")" ":" type_literal function_body ;
function_body = "{" { statement } "}"
              | "=" expression [ ";" ] ;

parameter_list = parameter { "," parameter } ;
parameter = ( "let" | "mut" ) identifier ":" type_literal [ "=" expression ] ;
label = identifier "@" ;
```

### Examples

```nugdev
// 블록 본문 함수
fun add(let a: number, let b: number): number {
    return a + b;
}

// 표현식 본문 함수
fun multiply(let x: number, let y: number): number = x * y;

// 가변 매개변수
fun updateCounter(mut counter: number, let increment: number): number {
    counter += increment;
    return counter;
}

// 복잡한 타입의 매개변수
fun processUsers(let users: Array<User>, let filter: (User) -> boolean): Array<User> {
    mut result: Array<User> = [];
    for (user in users) {
        if (filter(user)) {
            result.push(user);
        }
    }
    return result;
}

// 고차 함수
fun createValidator(let rules: Array<string>): (string) -> boolean = 
    (let input: string) => rules.every((let rule: string) => validateRule(input, rule));

// 매개변수 없는 함수
fun getCurrentTime(): number {
    return Date.now();
}

// 빈 return (void 반환)
fun logMessage(let msg: string): void {
    console.log(msg);
    return;  # 명시적 return
}

// 옵셔널 타입 반환
fun findUser(let id: number): User? {
    let user = database.findById(id);
    return user ?? null;
}

# Named 함수는 함수 이름이 자동으로 레이블 역할
fun add(let a: number, let b: number): number {
    return a + b
}

# 함수 이름을 레이블로 사용하여 early return
fun divide(let a: number, let b: number): number? {
    if (b == 0) return@divide null  # 함수 이름을 레이블로 사용
    return a / b
}
```

## Control Statement

프로그램의 제어 흐름을 변경하는 구문입니다.

### Break Statement

루프나 블록에서 빠져나가는 구문입니다.

#### EBNF

```ebnf
break_statement = "break" [ "@" identifier ] [ ";" ] ;
```

#### Examples

```nugdev
// 기본 break
for (i in 0..10) {
    if (i == 5) break;
    print(i);
}

// 라벨이 있는 break
outer: for (i in 0..5) {
    for (j in 0..5) {
        if (i * j > 10) break outer;
        print(i, j);
    }
}

// when 문에서 break
let result = when (input) {
    "quit": break,
    "exit": break,
    else: process(input)
};
```

### Continue Statement

현재 반복을 건너뛰고 다음 반복을 진행하는 구문입니다.

#### EBNF

```ebnf
continue_statement = "continue" [ "@" identifier ] [ ";" ] ;
```

#### Examples

```nugdev
// 기본 continue
for (i in 0..10) {
    if (i % 2 == 0) continue;
    print(i);  // 홀수만 출력
}

// 라벨이 있는 continue
outer: for (i in 0..5) {
    for (j in 0..5) {
        if (j == 2) continue outer;
        print(i, j);
    }
}

// 조건부 continue
for (item in items) {
    if (item.deleted) continue;
    if (item.archived) continue;
    processItem(item);
}
```

### Return Statement

함수에서 값을 반환하고 실행을 종료하는 구문입니다.

#### EBNF

```ebnf
return_statement = "return" [ "@" identifier ] [ expression ] [ ";" ] ;
```

#### Examples

```nugdev
// 값과 함께 return
fun add(let a: number, let b: number): number {
    return a + b;
}

// 조건부 return
fun divide(let a: number, let b: number): number? {
    if (b == 0) return null;
    return a / b;
}

// 빈 return (void 반환)
fun logMessage(let msg: string): void {
    console.log(msg);
    return;  // 명시적 return
}

// 조기 return 패턴
fun processUser(let user: User?): User? {
    if (!user) return null;
    if (!user.active) return null;
    if (!user.verified) return null;
    
    return {
        id: user.id,
        name: user.name,
        email: user.email
    };
}
```

## Expression Statement

표현식을 구문으로 사용하는 것입니다. 표현식의 값은 무시됩니다.

### EBNF

```ebnf
expression_statement = expression [ ";" ] ;
```

### Examples

```nugdev
// 함수 호출 (부수 효과를 위해)
print("Hello, World!");
console.log("Debug message");
updateDatabase(user);

// 할당 표현식
counter += 1;
array.push(newItem);
object.property = newValue;

// 메서드 체이닝
data
    .filter((let x: any) => x.active)
    .map((let x: any) => x.name)
    .forEach((let name: string) => print(name));

// 복잡한 표현식
calculateAndStore(
    users.filter((let u: User) => u.active)
         .map((let u: User) => processUser(u))
         .reduce((let acc: number, let u: ProcessedUser) => acc + u.score, 0)
);
```

## Statement Blocks

여러 구문을 그룹화하는 블록입니다.

### EBNF

```ebnf
statement_block = "{" { statement } "}" ;
```

### Examples

```nugdev
// if 문의 블록
if (condition) {
    let temp: string = process(data);
    save(temp);
    print("Saved successfully");
}

// for 문의 블록  
for (item in items) {
    let processed: ProcessedItem = transform(item);
    let validated: boolean = validate(processed);
    store.add(validated);
}

// 함수 본문 블록
fun complexFunction(let input: string): Result {
    let step1: string = preprocess(input);
    let step2: ValidatedData = validate(step1);
    let step3: TransformedData = transform(step2);
    return finalize(step3);
}
```

## Statement Composition

구문들의 조합과 중첩 예시입니다.

### Examples

```nugdev
// 변수 선언과 제어 흐름
fun processItems(let items: Array<Item>): ProcessResult {
    mut results: Array<ProcessedItem> = [];
    mut errors: Array<string> = [];
    
    for (item in items) {
        # TODO: try/catch는 향후 확장 기능
        # let processed: ProcessedItem? = try {
        #     return processItem(item);
        # } catch (error) {
        #     errors.push(error.message);
        #     continue;
        # };
        
        let processed: ProcessedItem? = processItem(item)
        if (processed == None) {
            errors.push("Processing failed for " + item.id)
            continue
        }
        
        if (validate(processed)) {
            results.push(processed);
        } else {
            errors.push("Validation failed for " + item.id);
        }
    }
    
    return { results: results, errors: errors };
}

// 복잡한 제어 흐름
fun findOptimalPath(let graph: Graph, let start: Node, let end: Node): Path? {
    let visited: Set<Node> = new Set();
    mut queue: Array<Node> = [start];
    
    while (!queue.isEmpty()) {
        let current: Node = queue.shift();
        
        if (visited.has(current)) continue;
        visited.add(current);
        
        if (current == end) return reconstructPath(current);
        
        for (neighbor in graph.getNeighbors(current)) {
            if (!visited.has(neighbor)) {
                queue.push(neighbor);
            }
        }
    }
    
    return null;  // Path not found
}
```

## Statement vs Expression

Statement와 Expression의 차이점을 보여주는 예시입니다.

### Examples

```nugdev
// ❌ Statement는 값으로 사용할 수 없음
let x: number = (let y: number = 5);  // 컴파일 에러

// ✅ Expression은 값으로 사용 가능
let x: number = (y = 5);      // OK: assignment expression

// ❌ Statement는 표현식 위치에 올 수 없음
let result: number = if (condition) let temp: number = 5; else 10;  // 에러

// ✅ Expression은 어디든 사용 가능
let result: number = if (condition) (temp = 5) else 10;     // OK

// ✅ Statement Block은 표현식으로 사용 가능
let result: number = if (condition) {
    let temp: number = 5;  // statement
    temp * 2               // expression (마지막 값이 반환됨)
} else {
    10
};
``` 