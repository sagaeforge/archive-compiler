# Control Flow

nugdev 언어의 제어 흐름 구조를 정의합니다. 제어문과 반복문은 조건식이 없는 경우 참을 의미하며, 코틀린 스타일의 `@` 라벨을 지원합니다.

## Labels

함수와 제어문에 라벨을 붙여서 명시적으로 제어할 수 있습니다.

### 레이블 사용 정책

- **Named 함수**: 함수 이름 자체가 레이블 역할 (명시적 레이블 불필요)
- **익명 함수/람다**: 명시적 레이블 필요 시에만 사용
- **제어문 (if/for)**: 익명 구조이므로 레이블 필요 시 명시

### EBNF

```ebnf
label = identifier "@" ;
```

### Examples

```nugdev
# Named 함수 - 함수 이름이 자동으로 레이블 역할
fun compute(let x: number): number {
    if (x < 0) return@compute 0  # 함수 이름을 레이블로 사용
    return x * x
}

# 익명 함수에서만 명시적 레이블 사용
let calculator = calculation@ fun(let a: number, let b: number): number {
    if (a < 0) return@calculation 0
    return a + b
}

# 람다에서 레이블 사용
let validator = validation@ (let input: string) => {
    if (input.length == 0) return@validation false
    return true
}

# 제어문에서 라벨 사용
outer@ for (mut i: number = 0; i < 10; i += 1) {
    inner@ for (mut j: number = 0; j < 10; j += 1) {
        if (i * j > 50) break@outer
        print(`${i} * ${j} = ${i * j}`)
    }
}
```

## If Statement/Expression

조건에 따라 분기하는 구조입니다.

### EBNF

```ebnf
if_statement = [ label ] "if" [ "(" expression ")" ] "{" { statement_or_expression } "}"
               { "elif" [ "(" expression ")" ] "{" { statement_or_expression } "}" }
               [ "else" "{" { statement_or_expression } "}" ] ;

statement_or_expression = statement | expression ;
```

### Examples

```nugdev
# 조건식 없는 if (항상 참)
if {
    print("This always executes");
    let result: number = 42;
}

# 라벨이 있는 if
validation@ if (user.isValid) {
    processUser(user);
} else {
    return@validation null;
}

# 기본 if 문
if (age >= 18) {
    print("You are an adult");
    let status: string = "adult";
}

# if-elif-else 체인
if (score >= 90) {
    print("Excellent!");
    let grade: string = "A";
} elif (score >= 80) {
    print("Good job!");
    let grade: string = "B";
} elif (score >= 70) {
    print("Not bad!");
    let grade: string = "C";
} else {
    print("Try harder!");
    let grade: string = "F";
}

# 조건식 없는 elif (항상 참으로 처리)
if (condition1) {
    handleCondition1();
} elif {
    # 이 블록은 condition1이 false일 때 항상 실행
    handleDefault();
}

# 표현식으로 사용 (if expression)
let status: string = if (age >= 18) "adult" else "minor";
let message: string = if (isLoggedIn) {
    "Welcome back!"
} else {
    "Please log in"
};
```

## When Expression (Pattern Matching)

패턴 매칭을 통한 분기 표현식입니다. 코틀린 스타일의 `->` 문법을 사용합니다.

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
# 기본 when 표현식
let dayType: string = when (dayOfWeek) {
    1, 2, 3, 4, 5 -> "weekday",
    6, 7 -> "weekend", 
    else -> "invalid"
}

# 값에 따른 분기
let statusMessage: string = when (httpStatus) {
    200 -> "OK",
    201 -> "Created",
    400 -> "Bad Request",
    404 -> "Not Found",
    500 -> "Internal Server Error",
    else -> "Unknown Status"
}

# 범위 조건
let ageGroup: string = when (age) {
    in 0..13 -> "child",
    in 13..20 -> "teenager", 
    in 20..65 -> "adult",
    in 65..121 -> "senior",
    else -> "invalid_age"
}

# 타입 검사 (향후 확장)
let description: string = when (value) {
    is string -> "String value: ${value}",
    is number -> "Number value: ${value}",
    is boolean -> "Boolean value: ${value}",
    is Array -> "Array with ${value.length} items",
    else -> "Unknown type"
}

# 가드 조건이 있는 when
let userAccess: string = when (user) {
    u if u.role == "admin" && u.active -> "full_access",
    u if u.role == "moderator" && u.active -> "moderate_access",
    u if u.role == "user" && u.verified -> "limited_access",
    u if u.suspended -> "suspended",
    else -> "no_access"
}

# 블록을 포함한 when
let result: ProcessResult = when (command) {
    "start" -> {
        let service: Service = initializeService()
        service.start()
        { success: true, message: "Service started" }
    },
    "stop" -> {
        let service: Service = getRunningService()
        if (service) {
            service.stop()
            { success: true, message: "Service stopped" }
        } else {
            { success: false, message: "No running service" }
        }
    },
    "restart" -> {
        stopService()
        let newService: Service = startService()
        { success: true, message: "Service restarted", data: newService }
    },
    else -> {
        let error: string = "Unknown command: ${command}"
        logError(error)
        { success: false, message: error }
    }
}

# 복잡한 조건 매칭
let fileAction: string = when (file) {
    f if f.extension == "txt" && f.size < 1000 -> "edit_inline",
    f if f.extension == "txt" && f.size >= 1000 -> "open_editor",
    f if f.extension in ["jpg", "png", "gif"] -> "preview_image",
    f if f.extension in ["mp4", "avi", "mov"] -> "play_video", 
    f if f.extension == "pdf" -> "open_pdf_viewer",
    f if f.isDirectory -> "browse_directory",
    else -> "download"
}

# 여러 값에 대한 동일한 처리
let operationType: string = when (httpMethod) {
    "GET", "HEAD", "OPTIONS" -> "read",
    "POST", "PUT", "PATCH" -> "write",
    "DELETE" -> "delete",
    else -> "unknown"
}

# when을 statement로 사용 (값 반환 없음)
when (notification.type) {
    "email" -> {
        sendEmail(notification.recipient, notification.content)
        logNotification("email", notification.id)
    },
    "sms" -> {
        sendSMS(notification.phone, notification.message)
        logNotification("sms", notification.id)
    },
    "push" -> {
        sendPushNotification(notification.deviceId, notification.payload)
        logNotification("push", notification.id)
    },
    else -> {
        logError("Unknown notification type: ${notification.type}")
    }
}
```

## For Loop

다양한 형태의 반복문을 지원합니다.

### EBNF

```ebnf
for_statement = [ label ] "for" for_clause "{" { statement_or_expression } "}" ;

for_clause = (* empty *)                                                    (* infinite loop *)
           | "(" expression ")"                                             (* while-style *)
           | "(" variable_declaration ";" expression ")"                    (* init; condition *)
           | "(" variable_declaration ";" expression ";" expression ")"     (* init; condition; increment *)
           | "(" identifier "in" expression ")"                             (* for-in loop *) ;
```

### Examples

```nugdev
# 무한 루프 (조건식 없음 = 참)
for {
    let data: string = readInput();
    if (data == "quit") break;
    processData(data);
}

# 라벨이 있는 무한 루프
main@ for {
    let command: string = getCommand();
    if (command == "exit") break@main;
    processCommand(command);
}

# while 스타일 (조건만)
for (hasMoreData()) {
    let chunk: DataChunk = getNextChunk();
    processChunk(chunk);
}

# C 스타일 for 루프 (초기화; 조건)
for (mut i: number = 0; i < 10) {
    print(i);
    i += 1;
}

# 완전한 C 스타일 for 루프 (초기화; 조건; 증감)
for (mut i: number = 0; i < 10; i += 1) {
    print(`Number: ${i}`);
    let square: number = i * i;
    print(`Square: ${square}`);
}

# for-in 루프 (컬렉션 반복)
items@ for (item in items) {
    if (item.isDeleted) continue@items;
    if (item.shouldStop) break@items;
    processItem(item);
}

# 복잡한 초기화와 조건
for (mut total: number = 0, mut count: number = 0; count < data.length and total < 1000) {
    total += data[count];
    count += 1;
    print(`Running total: ${total}`);
}

# 중첩 루프
outer@ for (mut i: number = 0; i < rows; i += 1) {
    inner@ for (mut j: number = 0; j < cols; j += 1) {
        let value: number = matrix[i][j];
        if (value == target) {
            print(`Found at (${i}, ${j})`);
            break@outer;
        }
    }
}

# 라벨이 있는 중첩 루프
search@ for (mut i: number = 0; i < 10; i += 1) {
    process@ for (mut j: number = 0; j < 10; j += 1) {
        if (i == j) continue@process;
        if (i * j > 50) break@search;
        print(`${i} * ${j} = ${i * j}`);
    }
}
```

## Loop Control

루프 제어 구문들입니다.

### EBNF

```ebnf
break_statement = "break" [ "@" identifier ] ";" ;
continue_statement = "continue" [ "@" identifier ] ";" ;
return_statement = "return" [ "@" identifier ] [ expression ] ";" ;
```

### Examples

```nugdev
# 기본 break와 continue
for (mut i: number = 0; i < 100; i += 1) {
    if (i % 2 == 0) continue;  # 짝수 건너뛰기
    if (i > 50) break;         # 50 초과시 종료
    print(i);
}

# 라벨이 있는 break와 continue
outer@ for (mut i: number = 0; i < 5; i += 1) {
    inner@ for (mut j: number = 0; j < 5; j += 1) {
        if (i == j) continue@inner;      # 내부 루프 계속
        if (i + j > 6) continue@outer;   # 외부 루프 계속
        if (i * j > 10) break@outer;     # 외부 루프 종료
        print(`${i}, ${j}`);
    }
}

# 라벨이 있는 return
calculator@ fun divide(let a: number, let b: number): number? {
    if (b == 0) return@calculator null;
    return a / b;
}

# 람다에서 라벨 return
let processor: (Array<number>) -> number = processLoop@ (let numbers: Array<number>) => {
    for (num in numbers) {
        if (num < 0) return@processLoop -1;
        if (num > 100) return@processLoop num;
    }
    return@processLoop 0;
};
```

## Lambda Labels

람다 표현식에도 라벨을 사용할 수 있습니다.

### Examples

```nugdev
# 라벨이 있는 람다
let handler: (string) -> string = errorCheck@ (let input: string) => {
    if (input.length == 0) return@errorCheck "empty";
    if (input.length > 100) return@errorCheck "too_long";
    return@errorCheck "valid";
};

# 고차 함수에서 라벨 사용
let numbers: Array<number> = [1, 2, 3, 4, 5];
let processed: Array<number> = numbers.map(transform@ (let x: number) => {
    if (x < 0) return@transform 0;
    if (x > 10) return@transform 10;
    return@transform x * 2;
});

# 복잡한 람다 체이닝
let result: Array<string> = data
    .filter(validate@ (let item: Item) => {
        if (!item.active) return@validate false;
        if (item.category != "valid") return@validate false;
        return@validate true;
    })
    .map(format@ (let item: Item) => {
        if (item.priority == "high") return@format `[HIGH] ${item.name}`;
        return@format item.name;
    });
```

## Complex Control Flow Examples

복잡한 제어 흐름 조합 예시입니다.

### Examples

```nugdev
# 데이터 처리 파이프라인
dataProcessing@ fun processDataStream(let stream: DataStream): ProcessResult {
    mut processedCount: number = 0;
    mut errorCount: number = 0;
    mut results: Array<ProcessedData> = [];
    
    mainLoop@ for (hasData(stream)) {
        let chunk: DataChunk = readChunk(stream);
        
        itemLoop@ for (item in chunk.items) {
            let validation: ValidationResult = validate(item);
            
            when (validation) {
                { valid: true, data: validData }: {
                    let processed: ProcessedData = transformData(validData);
                    results.push(processed);
                    processedCount += 1;
                },
                { valid: false, error: error }: {
                    logError(error);
                    errorCount += 1;
                    
                    if (errorCount > MAX_ERRORS) {
                        return@dataProcessing { success: false, error: "Too many errors" };
                    }
                    continue@itemLoop;
                }
            }
        }
        
        # 진행률 체크
        if (processedCount % 1000 == 0) {
            print(`Processed ${processedCount} items`);
        }
        
        if (shouldStop()) break@mainLoop;
    }
    
    return@dataProcessing {
        success: true,
        processed: processedCount,
        errors: errorCount,
        results: results
    };
}

# 게임 루프
gameMain@ fun gameLoop(): void {
    mut running: boolean = true;
    mut lastFrameTime: number = Date.now();
    
    mainLoop@ for (running) {
        let currentTime: number = Date.now();
        let deltaTime: number = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        
        # 입력 처리
        inputLoop@ for (input in getInputEvents()) {
            when (input.type) {
                "keydown": handleKeyDown(input),
                "keyup": handleKeyUp(input),
                "quit": {
                    running = false;
                    break@inputLoop;
                }
            }
        }
        
        if (!running) break@mainLoop;
        
        # 게임 로직 업데이트
        updateGame(deltaTime);
        
        # 렌더링
        render();
        
        # FPS 제한
        let frameTime: number = Date.now() - currentTime;
        if (frameTime < TARGET_FRAME_TIME) {
            sleep(TARGET_FRAME_TIME - frameTime);
        }
    }
    
    cleanup();
} 