# Functions

nugdev 언어의 함수 정의와 호출을 다룹니다.

## Function Declaration

함수를 정의하는 구문입니다.

### EBNF

```ebnf
function_declaration = "fun" identifier "(" [ parameter_list ] ")" ":" type_literal function_body ;
function_body = "{" { statement } "}"
              | "=" expression [ ";" ] ;

parameter_list = parameter { "," parameter } ;
parameter = ( "let" | "mut" ) identifier ":" type_literal [ "=" expression ] ;
```

### Examples

```nugdev
# 기본 함수 정의
fun add(let a: number, let b: number): number {
    return a + b;
}

# 기본값 매개변수가 있는 함수
fun greet(let name: string, let greeting: string = "Hello"): string {
    return `${greeting}, ${name}!`;
}

# 표현식 본문 함수
fun multiply(let x: number, let y: number = 1): number = x * y;

# 여러 기본값 매개변수
fun createUser(let name: string, let age: number = 18, let role: string = "user", let active: boolean = true): User {
    return {
        .name = name,
        .age = age,
        .role = role,
        .active = active,
        .createdAt = Date.now()
    };
}

# 복잡한 함수 with 기본값
fun processUser(let user: User, mut config: ProcessConfig = defaultConfig, let logger: (string) -> void = console.log): ProcessResult {
    logger("Processing user: ${user.name}");
    
    if (!user.active) {
        return { success: false, error: "User not active" };
    }
    
    config.lastProcessed = Date.now();
    let result: ValidationResult = validate(user);
    
    if (!result.valid) {
        return { success: false, error: result.message };
    }
    
    return {
        success: true,
        data: {
            id: user.id,
            name: user.name,
            processedAt: config.lastProcessed
        }
    };
}

# 조건부 기본값
fun connect(let host: string, let port: number = if (isProduction) 443 else 8080, let timeout: number = 5000): Connection {
    return createConnection(host, port, timeout);
}
```

## Function Expressions

함수를 표현식으로 정의합니다.

### EBNF

```ebnf
function_expression = [ label ] "fun" "(" [ parameter_list ] ")" ":" type_literal function_expression_body ;
function_expression_body = "{" { statement } [ expression ] "}"
                         | "=" expression ;

parameter_list = parameter { "," parameter } ;
parameter = ( "let" | "mut" ) identifier ":" type_literal [ "=" expression ] ;
label = identifier "@" ;
```

### Examples

```nugdev
# 익명 함수 표현식 with 기본값
let calculator: Calculator = {
    .add = fun(let a: number, let b: number = 0): number = a + b,
    .subtract = fun(let a: number, let b: number = 0): number = a - b,
    .multiply = fun(let a: number, let b: number = 1): number = a * b,
    .divide = fun(let a: number, let b: number = 1): number? {
        if (b == 0) return null;
        return a / b;
    }
};

# 고차 함수 with 기본값
let createFormatter: (string, boolean) -> (any) -> string = fun(let template: string, let addTimestamp: boolean = false): (any) -> string = {
    fun(let value: any): string {
        let formatted: string = template.replace("{}", value.toString());
        if (addTimestamp) {
            return `[${Date.now()}] ${formatted}`;
        }
        return formatted;
    }
};

let numberFormatter: (any) -> string = createFormatter("Number: {}");
let timedFormatter: (any) -> string = createFormatter("Value: {}", true);
let result1: string = numberFormatter(42);  # "Number: 42"
let result2: string = timedFormatter(42);   # "[1234567890] Value: 42"

# 라벨이 있는 함수 표현식
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
```

## Lambda Expressions

간단한 함수를 람다식으로 정의합니다.

### EBNF

```ebnf
lambda_expression = [ label ] "(" [ lambda_parameter_list ] ")" "=>" expression ;
lambda_parameter_list = lambda_parameter { "," lambda_parameter } ;
lambda_parameter = ( "let" | "mut" ) identifier ":" type_literal [ "=" expression ] ;
label = identifier "@" ;
```

### Examples

```nugdev
# 기본 람다 표현식
let square: (number) -> number = (let x: number) => x * x;
let add: (number, number) -> number = (let a: number, let b: number = 0) => a + b;

# 기본값이 있는 람다
let formatter: (string, string, boolean) -> string = (
    let text: string, 
    let prefix: string = "[INFO]", 
    let uppercase: boolean = false
) => {
    let result: string = `${prefix} ${text}`;
    return uppercase ? result.toUpperCase() : result;
};

# 배열 메서드와 함께 (기본값 활용)
let numbers: Array<number> = [1, 2, 3, 4, 5];
let doubled: Array<number> = numbers.map((let x: number, let multiplier: number = 2) => x * multiplier);
let filtered: Array<number> = numbers.filter((let x: number, let threshold: number = 3) => x > threshold);

# 객체 처리 with 기본값
let users: Array<User> = getUserList();
let processUsers: Array<ProcessedUser> = users
    .filter((let u: User, let activeOnly: boolean = true) => !activeOnly or u.active)
    .map((let u: User, let includeEmail: boolean = false) => {
        let base: ProcessedUser = { id: u.id, name: u.name };
        return includeEmail ? { ...base, email: u.email } : base;
    });

# 복잡한 람다 with 다양한 기본값
let createProcessor: (ProcessOptions) -> (any) -> any = (let options: ProcessOptions = {
    strict: false,
    timeout: 5000,
    retries: 3
}) => {
    (let data: any) => {
        # 프로세싱 로직
        return processWithOptions(data, options);
    };
};
```

## Default Parameter Patterns

기본값 매개변수의 다양한 패턴들입니다.

### Examples

```nugdev
# 1. 단순 기본값
fun log(let message: string, let level: string = "info"): void {
    console.log(`[${level.toUpperCase()}] ${message}`);
}

# 2. 계산된 기본값
fun createId(let prefix: string = "id", let timestamp: number = Date.now()): string {
    return `${prefix}_${timestamp}`;
}

# 3. 조건부 기본값
fun getConfig(let env: string = if (isDevelopment) "dev" else "prod"): Config {
    return loadConfig(env);
}

# 4. 함수 기본값
fun withRetry(let operation: () -> any, let onError: (Error) -> void = (let err: Error) => console.error(err)): any {
    # TODO: try/catch는 향후 확장 기능
    # try {
    #     return operation()
    # } catch (error) {
    #     onError(error)
    #     return None
    # }
    
    let result: any = operation()
    return result
}

# 5. 객체/배열 기본값
fun processItems(
    let items: Array<any>, 
    let options: ProcessOptions = { 
        parallel: true, 
        chunkSize: 100,
        timeout: 30000 
    }
): Array<any> {
    return options.parallel ? 
        processInParallel(items, options) : 
        processSequentially(items, options);
}

# 6. 중첩된 기본값
fun createServer(
    let host: string = "localhost",
    let port: number = if (host == "localhost") 3000 else 80,
    let ssl: boolean = port == 443
): Server {
    return new Server(host, port, ssl);
}

# 7. 가변 매개변수와 기본값
fun logMultiple(let messages: Array<string>, let separator: string = "\n", let timestamp: boolean = true): void {
    let prefix: string = timestamp ? `[${new Date().toISOString()}] ` : "";
    let combined: string = messages.join(separator);
    console.log(`${prefix}${combined}`);
}
```

## Function Call Patterns

기본값 매개변수가 있는 함수 호출 패턴들입니다.

### Examples

```nugdev
# 기본값 사용하는 다양한 호출 방식

# 1. 모든 기본값 사용
log("Application started");  # level="info" 기본값

# 2. 일부 기본값 사용  
let user1: User = createUser("Alice");  # age=18, role="user", active=true 기본값
let user2: User = createUser("Bob", 25);  # role="user", active=true 기본값

# 3. 모든 매개변수 명시
let user3: User = createUser("Charlie", 30, "admin", false);

# 4. 함수 타입 매개변수의 기본값
let result1: any = withRetry(() => riskyOperation());  # 기본 에러 핸들러 사용
let result2: any = withRetry(
    () => riskyOperation(),
    (let err: Error) => logToFile(err)  # 커스텀 에러 핸들러
);

# 5. 체이닝과 기본값
let processed: Array<any> = items
    .filter((let item: any) => validate(item))  # 기본 검증
    .map((let item: any) => transform(item))    # 기본 변환
    .sort((let a: any, let b: any) => compare(a, b));  # 기본 비교

# 6. 조건부 기본값 활용
let connection1: Connection = connect("api.example.com");  # 환경에 따른 포트 기본값
let connection2: Connection = connect("localhost", 8080);  # 개발용 포트 명시
```

## Function Types

함수의 타입을 정의하고 사용합니다.

### Examples

```nugdev
# 함수 타입 사용
let isEven: (number) -> boolean = (let x: number) => x % 2 == 0;
let toString: (number) -> string = (let x: number) => x.toString();
let sumReducer: (number, number) -> number = (let acc: number, let x: number) => acc + x;

# 고차 함수 정의
fun filter(let arr: Array<any>, let predicate: (any) -> boolean): Array<any> {
    mut result: Array<any> = [];
    for (item in arr) {
        if (predicate(item)) {
            result.push(item);
        }
    }
    return result;
}

fun map(let arr: Array<any>, let mapper: (any) -> any): Array<any> {
    mut result: Array<any> = [];
    for (item in arr) {
        result.push(mapper(item));
    }
    return result;
}
```

## Closures

클로저를 사용한 고급 함수 예시입니다.

### Examples

```nugdev
# 기본 클로저
fun createCounter(): () -> number {
    mut count: number = 0;
    return () => {
        count += 1;
        count
    };
}

let counter: () -> number = createCounter();
let first: number = counter();   # 1
let second: number = counter();  # 2

# 상태를 공유하는 클로저
fun createBankAccount(let initialBalance: number): BankAccount {
    mut balance: number = initialBalance;
    
    return {
        .deposit = (let amount: number) => {
            if (amount > 0) {
                balance += amount;
                return balance;
            }
            return balance;
        },
        .withdraw = (let amount: number) => {
            if (amount > 0 and amount <= balance) {
                balance -= amount;
                return balance;
            }
            return balance;
        },
        .getBalance = () => balance
    };
}

let account: BankAccount = createBankAccount(100);
account.deposit(50);   # balance: 150
account.withdraw(30);  # balance: 120

# 설정을 캡처하는 클로저
fun createValidator(let rules: ValidationRules): (any) -> ValidationResult {
    return (let data: any) => {
        mut errors: Array<string> = [];
        
        if (rules.required and !data) {
            errors.push("Field is required");
        }
        
        if (rules.minLength and data.length < rules.minLength) {
            errors.push(`Minimum length is ${rules.minLength}`);
        }
        
        if (rules.pattern and !rules.pattern.test(data)) {
            errors.push("Pattern does not match");
        }
        
        return {
            valid: errors.length == 0,
            errors: errors
        };
    };
}

let emailValidator: (any) -> ValidationResult = createValidator({
    .required = true,
    .pattern = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
});
```

## Function Composition

함수 합성과 관련된 고급 패턴입니다.

### Examples

```nugdev
# 함수 합성
fun compose(let f: (any) -> any, let g: (any) -> any): (any) -> any {
    return (let x: any) => f(g(x));
}

let addOne: (number) -> number = (let x: number) => x + 1;
let double: (number) -> number = (let x: number) => x * 2;
let addOneThenDouble: (any) -> any = compose(double, addOne);

let result: any = addOneThenDouble(5);  # (5 + 1) * 2 = 12

# 파이프라인 연산자 시뮬레이션
fun pipe(let value: any, let fn: (any) -> any): any {
    return fn(value);
}

let processedValue: any = input
    |> ((let x: any) => x + 1)
    |> ((let x: any) => x * 2)
    |> ((let x: any) => x - 3);

# 커링
fun curry(let fn: (any, any) -> any): (any) -> (any) -> any {
    return (let a: any) => (let b: any) => fn(a, b);
}

let add2: (any, any) -> any = (let a: any, let b: any) => a + b;
let curriedAdd: (any) -> (any) -> any = curry(add2);
let add5: (any) -> any = curriedAdd(5);

let result2: any = add5(10);  # 15
```

# Named 함수 - 함수 이름이 자동으로 레이블 역할
fun add(let a: number, let b: number): number {
    return a + b
}

# 함수 이름을 레이블로 사용
fun divide(let a: number, let b: number): number? {
    if (b == 0) return@divide null  # 함수 이름이 레이블
    return a / b
}

# 조기 리턴이 필요한 복잡한 함수
fun processUser(let user: User, mut config: ProcessConfig = defaultConfig, let logger: (string) -> void = console.log): ProcessResult {
    logger("Processing user: ${user.name}")
    
    if (!user.active) {
        return@processUser { success: false, error: "User not active" }  # 함수 이름 사용
    }
    
    config.lastProcessed = Date.now()
    let result: ValidationResult = validate(user)
    
    if (!result.valid) {
        return@processUser { success: false, error: result.message }
    }
    
    return@processUser {
        success: true,
        data: {
            id: user.id,
            name: user.name,
            processedAt: config.lastProcessed
        }
    }
} 