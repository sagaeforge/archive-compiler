# Comprehensive Examples

Nugdev 언어의 expression 기반 특성을 보여주는 종합적인 예시들입니다.

## Simple Calculator

```nugdev
# 계산기 프로그램 - 모든 것이 표현식
let calculator = {
    # 연산자들을 함수로 정의
    .add = (let a: number, let b: number) => a + b,
    .subtract = (let a: number, let b: number) => a - b,
    .multiply = (let a: number, let b: number) => a * b,
    .divide = (let a: number, let b: number) => if (b != 0) a / b else null,
    
    # 복잡한 연산
    .power = fun(let base: number, let exp: number): number = {
        if (exp == 0) return 1
        if (exp == 1) return base
        
        let half: number = power(base, exp / 2)
        if (exp % 2 == 0) {
            half * half
        } else {
            base * half * half
        }
    },
    
    # 표현식을 평가하는 함수
    .evaluate = fun(let operation: string, let a: number, let b: number): number = {
        when (operation) {
            "+": add(a, b),
            "-": subtract(a, b), 
            "*": multiply(a, b),
            "/": divide(a, b) ?? 0,
            "^": power(a, b),
            else: 0
        }
    }
}

# 사용 예시
let result1 = calculator.evaluate("+", 10, 5)  # 15
let result2 = calculator.power(2, 8)           # 256

# 연쇄 계산
let complexResult: number = calculator.add(
    calculator.multiply(3, 4),
    calculator.power(2, 3)
)  # 17
```

## User Management System

```nugdev
# 사용자 관리 시스템
let userManager = {
    .users = [],
    .nextId = 1,
    
    # 사용자 생성 - 표현식이므로 생성된 사용자를 반환
    .createUser = fun(let name: string, let email: string, let role: string): User = {
        let newUser: User = {
            .id = userManager.nextId++,
            .name = name,
            .email = email,
            .role = role,
            .active = true,
            .createdAt = Date.now()
        }
        
        userManager.users = [...userManager.users, newUser]
        newUser  # 생성된 사용자 반환
    },
    
    # 사용자 검색
    .findUser = fun(let predicate: (User) -> bool): User? = {
        userManager.users.find(predicate)
    },
    
    # 사용자 업데이트
    .updateUser = fun(let id: number, let updates: UserUpdate): User? = {
        let userIndex: number = userManager.users.findIndex((let u: User) => u.id == id)
        if (userIndex >= 0) {
            let updatedUser: User = { ...userManager.users[userIndex], ...updates }
            userManager.users = userManager.users.with(userIndex, updatedUser)
            updatedUser
        } else {
            null
        }
    },
    
    # 활성 사용자 필터링
    .getActiveUsers = () => userManager.users.filter((let u: User) => u.active),
    
    # 역할별 사용자 그룹핑
    .getUsersByRole = fun(let role: string): Array<User> = {
        userManager.users.filter((let u: User) => u.role == role)
    },
    
    # 사용자 통계
    .getStats = () => {
        .total = userManager.users.length,
        .active = userManager.getActiveUsers().length,
        .inactive = userManager.users.length - userManager.getActiveUsers().length,
        .byRole = userManager.getUsersByRole()
    }
}

# 사용 예시
let admin: User = userManager.createUser("Alice", "alice@example.com", "admin")
let user1: User = userManager.createUser("Bob", "bob@example.com", "user")
let user2: User = userManager.createUser("Charlie", "charlie@example.com", "guest")

# 검색과 업데이트를 표현식으로 체이닝
let updatedUser: User? = userManager.findUser((let u: User) => u.name == "Bob")
    ?.let((let u: User) => userManager.updateUser(u.id, { active: false }))

let stats = userManager.getStats()
```

## Functional Programming Examples

```nugdev
# 함수형 프로그래밍 유틸리티
let fp = {
    # 커링 함수
    .curry = fun(let fn: (any, any) -> any): (any) -> (any) -> any = {
        (let a: any) => (let b: any) => fn(a, b)
    },
    
    # 함수 합성
    .compose = fun(let ...fns: Array<(any) -> any>): (any) -> any = {
        (let x: any) => fns.reduceRight((let acc: any, let fn: (any) -> any) => fn(acc), x)
    },
    
    .pipe = fun(let ...fns: Array<(any) -> any>): (any) -> any = {
        (let x: any) => fns.reduce((let acc: any, let fn: (any) -> any) => fn(acc), x)
    },
    
    # 부분 적용
    .partial = fun(let fn: (any, any) -> any, let ...partialArgs: Array<any>): (any) -> any = {
        (let ...remainingArgs: Array<any>) => fn(...partialArgs, ...remainingArgs)
    }
}

# 데이터 변환 파이프라인
let processData = fp.pipe(
    (let data: Array<any>) => data.filter((let item: any) => item.active),
    (let data: Array<any>) => data.map((let item: any) => ({ ...item, processed: true })),
    (let data: Array<any>) => data.groupBy((let item: any) => item.category),
    (let data: Array<any>) => Object.entries(data).map((let [key, values]: [string, Array<any>]) => ({
        category: key,
        count: values.length,
        items: values
    }))
)

# 수학 함수 합성
let addThenMultiply = fp.compose(
    (let x: number) => x * 2,
    (let x: number) => x + 10
)

let result = addThenMultiply(5)  # (5 + 10) * 2 = 30

# 커링된 함수 사용
let curriedAdd = fp.curry((let a: number, let b: number, let c: number) => a + b + c)
let add5 = curriedAdd(5)
let add5And10 = add5(10)
let finalResult = add5And10(3)  # 18
```

## Async/Await Style (향후 확장)

```nugdev
# 비동기 프로그래밍 예시
let asyncOperations = {
    # Promise 기반 HTTP 요청
    .fetchUser = async function(id: number): Promise<User> {
        let response = await fetch(`/api/users/${id}`)
        if (response.ok) {
            return await response.json()
        } else {
            throw Error(`Failed to fetch user ${id}`)
        }
    },
    
    # 여러 비동기 작업 병렬 처리
    .fetchMultipleUsers = async function(ids: Array<number>): Promise<Array<User>> {
        let promises = for (id in ids) asyncOperations.fetchUser(id)
        return await Promise.all(promises)
    },
    
    # 순차 처리
    .processUsersSequentially = async function(ids: Array<number>): Promise<Array<User>> {
        mut results: Array<User> = []
        for (id in ids) {
            let user = await asyncOperations.fetchUser(id)
            let processed = await asyncOperations.processUser(user)
            results = [...results, processed]
        }
        return results
    },
    
    # 조건부 비동기 처리
    .updateUserIfExists = async function(id: number, updates: UserUpdate): Promise<User?> {
        let user = await asyncOperations.fetchUser(id).catch(() => null)
        if (user != null) {
            return await asyncOperations.updateUser(user.id, updates)
        } else {
            return null
        }
    }
}

# 사용 예시
let userData = await asyncOperations.fetchUser(123)
let allUsers = await asyncOperations.fetchMultipleUsers([1, 2, 3, 4, 5])

# 비동기 체이닝
let processedUser = await asyncOperations.fetchUser(456)
    .then((let user: User) => ({ ...user, lastAccessed: Date.now() }))
    .then((let user: User) => asyncOperations.updateUser(user.id, user))
```

## Pattern Matching and Destructuring

```nugdev
# 복잡한 패턴 매칭과 구조 분해
let dataProcessor = {
    # JSON API 응답 처리
    .processApiResponse = function(response: ApiResponse): Result<ProcessedData, Error> {
        when (response) {
            { status: "success", data: data }: {
                success: true,
                result: dataProcessor.transformData(data)
            },
            { status: "error", message: msg, code: code }: {
                success: false,
                error: { message: msg, code: code }
            },
            { status: "pending" }: {
                success: false,
                pending: true
            },
            else: {
                success: false,
                error: { message: "Unknown response format" }
            }
        }
    },
    
    # 다차원 배열 평탄화
    .flattenArray = function(arr: Array<any>): Array<any> {
        for (let item in arr) {
            when (item) {
                x if Array.isArray(x): ...dataProcessor.flattenArray(x),
                x: x
            }
        }
    },
    
    # 중첩 객체에서 값 추출
    .extractNestedValues = function(obj: any, path: string): any {
        let segments = path.split(".")
        mut current = obj
        
        for (let segment in segments) {
            current = when (current) {
                null: null,
                None: null,
                obj if obj is object: obj[segment] ?? null,
                else: null
            }
            
            if (current == null) break current
        }
        
        return current
    }
}

# 복잡한 데이터 구조 처리
let complexData = {
    users: [
        { id: 1, profile: { name: "Alice", settings: { theme: "dark" } } },
        { id: 2, profile: { name: "Bob", settings: { theme: "light" } } }
    ],
    metadata: { version: "1.0", timestamp: 1234567890 }
}

# 구조 분해와 변환
let {
    users: userList,
    metadata: { version, timestamp }
} = complexData

let processedUsers = for (let user in userList) {
    let { id, profile: { name, settings: { theme } } } = user
    {
        userId: id,
        userName: name,
        preferredTheme: theme,
        isActiveTheme: theme == "dark"
    }
}
```

## Error Handling and Validation

```nugdev
# 에러 처리와 검증 시스템
let validation = {
    # Result 타입 모방
    .Ok = (let value: any) => ({ type: "Ok", value: value }),
    .Err = (let error: any) => ({ type: "Err", error: error }),
    
    # 검증 함수들
    .isEmail = (let str: string) => /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(str),
    .isPositive = (let num: number) => num is number and num > 0,
    .isNotEmpty = (let str: string) => str is string and str.trim().length > 0,
    
    # 검증 체이닝
    .validate = function(value: any, validators: Array<(any) => Result<any, any>>): Result<any, any> {
        for (let validator in validators) {
            let result = validator(value)
            if (result.type == "Err") {
                return result
            }
            value = result.value
        }
        validation.Ok(value)
    },
    
    # 사용자 입력 검증
    .validateUser = function(userData: UserInput): Result<User, any> {
        let { name, email, age } = userData
        
        let nameResult = if (validation.isNotEmpty(name)) {
            validation.Ok(name.trim())
        } else {
            validation.Err("Name cannot be empty")
        }
        
        let emailResult = if (validation.isEmail(email)) {
            validation.Ok(email.toLowerCase())
        } else {
            validation.Err("Invalid email format")
        }
        
        let ageResult = if (validation.isPositive(age)) {
            validation.Ok(age)
        } else {
            validation.Err("Age must be a positive number")
        }
        
        # 모든 검증 결과 결합
        when ([nameResult, emailResult, ageResult]) {
            [{ type: "Ok", value: validName }, 
             { type: "Ok", value: validEmail }, 
             { type: "Ok", value: validAge }]: 
                validation.Ok({
                    name: validName,
                    email: validEmail,
                    age: validAge
                }),
            else: {
                let errors = [nameResult, emailResult, ageResult]
                    .filter((let r: Result<any, any>) => r.type == "Err")
                    .map((let r: Result<any, any>) => r.error)
                validation.Err(errors)
            }
        }
    }
}

# 사용 예시
let userInput = {
    name: "  Alice  ",
    email: "alice@example.com",
    age: 25
}

let validationResult = validation.validateUser(userInput)

let finalUser = when (validationResult) {
    { type: "Ok", value: user }: {
        ...user,
        id: generateId(),
        createdAt: Date.now()
    },
    { type: "Err", error: errors }: {
        console.error("Validation failed:", errors)
        null
    }
} 
```

## Calculator Application with Default Parameters

```nugdev
# 계산기 프로그램 - 기본값 매개변수 활용
let calculator = {
    # 연산자들을 함수로 정의 (기본값 매개변수 사용)
    .add = (let a: number, let b: number = 0) => a + b,
    .subtract = (let a: number, let b: number = 0) => a - b,
    .multiply = (let a: number, let b: number = 1) => a * b,
    .divide = (let a: number, let b: number = 1) => if (b != 0) a / b else null,
    
    # 고급 연산 with 기본값
    .power = fun(let base: number, let exp: number = 2): number = {
        if (exp == 0) return 1
        if (exp == 1) return base
        
        let half: number = power(base, exp / 2)
        if (exp % 2 == 0) {
            half * half
        } else {
            base * half * half
        }
    },
    
    # 로깅 기능이 있는 계산 함수
    .calculate = fun(
        let operation: string, 
        let a: number, 
        let b: number = 0, 
        let logResult: boolean = true,
        let logger: (string) -> void = (let msg: string) => console.log(msg)
    ): number = {
        let result: number = when (operation) {
            "+" -> add(a, b),
            "-" -> subtract(a, b), 
            "*" -> multiply(a, b),
            "/" -> divide(a, b) ?? 0,
            "^" -> power(a, b),
            else -> 0
        }
        
        if (logResult) {
            logger(`${a} ${operation} ${b} = ${result}`)
        }
        
        return result
    }
}

# 다양한 호출 방식
let result1 = calculator.add(10)        # b=0 기본값: 10 + 0 = 10
let result2 = calculator.multiply(5)    # b=1 기본값: 5 * 1 = 5
let result3 = calculator.power(2)       # exp=2 기본값: 2^2 = 4
let result4 = calculator.power(3, 4)    # 3^4 = 81

# 로깅 옵션 활용
let result5 = calculator.calculate("+", 10, 5)  # 기본 로깅 활성화
let result6 = calculator.calculate("*", 3, 4, false)  # 로깅 비활성화
```

## Enhanced User Management System

```nugdev
# 사용자 관리 시스템 - 기본값 매개변수로 개선
let userManager = {
    .users = [],
    .nextId = 1,
    
    # 유연한 사용자 생성 함수
    .createUser = fun(
        let name: string, 
        let email: string = `${name.toLowerCase()}@example.com`,
        let age: number = 18,
        let role: string = "user", 
        let active: boolean = true,
        let permissions: Array<string> = ["read"]
    ): User = {
        let newUser: User = {
            .id = userManager.nextId++,
            .name = name,
            .email = email,
            .age = age,
            .role = role,
            .active = active,
            .permissions = permissions,
            .createdAt = Date.now()
        }
        
        userManager.users = [...userManager.users, newUser]
        return newUser
    },
    
    # 고급 검색 기능
    .findUsers = fun(
        let criteria: UserCriteria = {},
        let limit: number = 10,
        let offset: number = 0,
        let sortBy: string = "name",
        let sortOrder: string = "asc"
    ): Array<User> = {
        let filtered: Array<User> = userManager.users.filter((let user: User) => {
            if (criteria.active != null && user.active != criteria.active) return false
            if (criteria.role && user.role != criteria.role) return false
            if (criteria.minAge && user.age < criteria.minAge) return false
            if (criteria.maxAge && user.age > criteria.maxAge) return false
            return true
        })
        
        let sorted: Array<User> = filtered.sort((let a: User, let b: User) => {
            let comparison: number = when (sortBy) {
                "name" -> a.name.localeCompare(b.name),
                "age" -> a.age - b.age,
                "createdAt" -> a.createdAt - b.createdAt,
                else -> 0
            }
            return sortOrder == "desc" ? -comparison : comparison
        })
        
        return sorted.slice(offset, offset + limit)
    },
    
    # 배치 업데이트 기능
    .batchUpdate = fun(
        let userIds: Array<number>,
        let updates: UserUpdate,
        let skipInvalid: boolean = true,
        let onError: (number, Error) -> void = (let id: number, let err: Error) => console.error(`Failed to update user ${id}: ${err.message}`)
    ): BatchUpdateResult = {
        mut successful: Array<number> = []
        mut failed: Array<{ id: number, error: string }> = []
        
        for (id in userIds) {
            # TODO: try/catch는 향후 확장 기능
            # try {
            #     let updated: User? = userManager.updateUser(id, updates)
            #     if (updated) {
            #         successful.push(id)
            #     } else if (!skipInvalid) {
            #         failed.push({ id: id, error: "User not found" })
            #     }
            # } catch (error) {
            #     onError(id, error)
            #     failed.push({ id: id, error: error.message })
            # }
            
            let updated: User? = userManager.updateUser(id, updates)
            if (updated) {
                successful.push(id)
            } else if (!skipInvalid) {
                failed.push({ id: id, error: "User not found" })
            }
        }
        
        return { successful: successful, failed: failed }
    }
}

# 다양한 사용자 생성 패턴
let admin: User = userManager.createUser("Alice", "alice@company.com", 30, "admin", true, ["read", "write", "delete"])
let basicUser: User = userManager.createUser("Bob")  # 모든 기본값 사용
let customUser: User = userManager.createUser("Charlie", None, 25)  # 일부 기본값 사용

# 고급 검색 활용
let activeAdmins: Array<User> = userManager.findUsers({ active: true, role: "admin" })
let youngUsers: Array<User> = userManager.findUsers({ minAge: 18, maxAge: 25 }, 5, 0, "age", "asc")
let allUsers: Array<User> = userManager.findUsers()  # 모든 기본값 사용
```

## Configuration Management System

```nugdev
# 설정 관리 시스템 - 기본값으로 견고한 구성
let configManager = {
    # 환경별 기본 설정
    .createConfig = fun(
        let environment: string = "development",
        let host: string = if (environment == "production") "api.example.com" else "localhost",
        let port: number = when (environment) {
            "production": 443,
            "staging": 8443,
            else: 3000
        },
        let ssl: boolean = port == 443 || port == 8443,
        let database: DatabaseConfig = {
            .host = if (environment == "production") "prod-db.example.com" else "localhost",
            .port = 5432,
            .name = `myapp_${environment}`,
            .ssl = ssl
        },
        let logging: LoggingConfig = {
            .level = if (environment == "production") "warn" else "debug",
            .format = "json",
            .output = if (environment == "production") "file" else "console"
        },
        let features: FeatureFlags = {
            .enableAnalytics = environment == "production",
            .enableDebugUI = environment != "production",
            .enableCaching = true
        }
    ): AppConfig = {
        return {
            .environment = environment,
            .host = host,
            .port = port,
            .ssl = ssl,
            .database = database,
            .logging = logging,
            .features = features,
            .createdAt = Date.now()
        }
    },
    
    # 동적 설정 로더
    .loadConfig = fun(
        let configPath: string = "./config.json",
        let fallbackToDefaults: boolean = true,
        let validateConfig: boolean = true,
        let onValidationError: (Array<string>) -> void = (let errors: Array<string>) => {
            console.error("Configuration validation failed:", errors.join(", "))
        }
    ): AppConfig = {
        mut config: AppConfig
        
        # TODO: try/catch는 향후 확장 기능
        # try {
        #     let fileConfig: any = JSON.parse(readFileSync(configPath, "utf8"))
        #     config = mergeWithDefaults(fileConfig)
        # } catch (error) {
        #     if (fallbackToDefaults) {
        #         console.warn(`Failed to load config from ${configPath}, using defaults: ${error.message}`)
        #         config = configManager.createConfig()
        #     } else {
        #         throw error
        #     }
        # }
        
        # 임시 구현 (try/catch 없이)
        config = configManager.createConfig()
        
        if (validateConfig) {
            let validationErrors: Array<string> = validateConfiguration(config)
            if (validationErrors.length > 0) {
                onValidationError(validationErrors)
                if (!fallbackToDefaults) {
                    throw Error("Invalid configuration")
                }
            }
        }
        
        return config
    }
}

# 다양한 환경 설정
let devConfig: AppConfig = configManager.createConfig()  # 모든 기본값
let prodConfig: AppConfig = configManager.createConfig("production")
let stagingConfig: AppConfig = configManager.createConfig("staging", "staging.example.com")

# 설정 로딩 패턴
let config1: AppConfig = configManager.loadConfig()  # 기본 경로, 모든 기본값
let config2: AppConfig = configManager.loadConfig("./prod-config.json", true, false)  # 검증 건너뛰기
```

## HTTP Client with Default Options

```nugdev
# HTTP 클라이언트 - 스마트 기본값 활용
let httpClient = {
    # 기본 요청 함수
    .request = fun(
        let url: string,
        let method: string = "GET",
        let headers: Headers = { "Content-Type": "application/json" },
        let body: any = null,
        let timeout: number = 5000,
        let retries: number = 3,
        let retryDelay: number = 1000,
        let validateStatus: (number) -> boolean = (let status: number) => status >= 200 && status < 300
    ): Promise<Response> = {
        async@ (let attempt: number = 0) => {
            try {
                let response: Response = await fetch(url, {
                    method: method,
                    headers: headers,
                    body: body ? JSON.stringify(body) : null,
                    timeout: timeout
                })
                
                if (validateStatus(response.status)) {
                    return@async response
                } else {
                    throw Error(`HTTP ${response.status}: ${response.statusText}`)
                }
            } catch (error) {
                if (attempt < retries) {
                    await delay(retryDelay * (attempt + 1))  # 지수 백오프
                    return@async request(url, method, headers, body, timeout, retries, retryDelay, validateStatus, attempt + 1)
                } else {
                    throw error
                }
            }
        }()
    },
    
    # 편의 메서드들
    .get = (let url: string, let headers: Headers = {}) => 
        httpClient.request(url, "GET", { ...defaultHeaders, ...headers }),
    
    .post = (let url: string, let data: any, let headers: Headers = {}) => 
        httpClient.request(url, "POST", { ...defaultHeaders, ...headers }, data),
    
    .put = (let url: string, let data: any, let headers: Headers = {}) => 
        httpClient.request(url, "PUT", { ...defaultHeaders, ...headers }, data),
    
    .delete = (let url: string, let headers: Headers = {}) => 
        httpClient.request(url, "DELETE", { ...defaultHeaders, ...headers }),
    
    # 특수 용도 메서드
    .upload = fun(
        let url: string,
        let file: File,
        let fieldName: string = "file",
        let additionalFields: any = {},
        let onProgress: (number) -> void = (let progress: number) => {},
        let chunkSize: number = 1024 * 1024  # 1MB
    ): Promise<Response> = {
        let formData: FormData = new FormData()
        formData.append(fieldName, file)
        
        for (let [key, value] in Object.entries(additionalFields)) {
            formData.append(key, value.toString())
        }
        
        return httpClient.request(url, "POST", {}, formData, 30000)  # 30초 타임아웃
    }
}

# 다양한 HTTP 요청 패턴
let response1: Response = await httpClient.get("/api/users")  # 기본 헤더
let response2: Response = await httpClient.post("/api/users", userData)  # 기본 Content-Type
let response3: Response = await httpClient.request("/api/data", "GET", {}, null, 10000)  # 커스텀 타임아웃
```

# 완전한 웹 게임 서버 시스템
# 모든 nugdev 문법 기능을 포함하는 종합 예시

import "http" as HttpModule
import "database"
import "crypto"
export GameServer

# 타입 정의들 (기본 타입, 복합 타입, 옵셔널)
let ServerConfig: any = {
    .host = "localhost",
    .port = 8080,
    .maxPlayers = 100,
    .gameMode = "battle_royale",
    .debug = true
}

# 게임 서버 클래스 (모든 변수 선언 형태)
let gameServer = {
    # 타입만 명시
    mut currentPlayers: Array<Player>
    let config: ServerConfig
    mut isRunning: boolean = false  # 타입 추론
    mut lastUpdate: number = Date.now()  # 값과 타입 모두
    
    # 라벨이 있는 함수 (기본값 매개변수, void 반환)
    fun initialize(
        let configFile: string = "server.config",
        let logLevel: string = "info",
        mut retryCount: number = 3,
        let onReady: (boolean) -> void = (let success: boolean) => console.log(`Server ready: ${success}`)
    ): void {
        console.log("Initializing game server...")
        
        # 모든 리터럴 타입 사용
        let serverName: string = `GameServer_${Date.now()}`  # 템플릿 문자열
        let maxRetries: number = 0xFF  # 16진수
        let binFlags: number = 0b1010_1101  # 이진수
        let octPerms: number = 0o755  # 8진수
        let timeoutSecs: number = 30.5e2  # 부동소수점
        let welcomeChar: string = '🎮'  # 문자 리터럴
        let isEnabled: boolean = true
        let fallbackHost: any = null  # null 리터럴
        let emptyResult: string? = None  # None 리터럴
        
        # 범위 리터럴과 for 루프 (라벨 사용)
        setupLoop@ for (attempt in 1..maxRetries) {
            let connectionResult: ConnectionResult? = tryConnect(configFile)
            
            when (connectionResult) {
                None -> {
                    console.log(`Attempt ${attempt} failed`)
                    if (attempt == maxRetries) {
                        return@initialize  # 함수 이름을 레이블로 사용
                    }
                    continue@setupLoop  # 라벨 continue
                },
                result -> {
                    console.log("Connected successfully!")
                    config = result.config
                    break@setupLoop  # 라벨 break
                }
            }
        }
        
        # 복잡한 표현식들
        let calculatedPort: number = config.port + (isEnabled ? 1000 : 0)  # 삼항 연산자
        let hostAddress: string = config.host ?? "127.0.0.1"  # null coalescing
        
        # 모든 연산자 사용
        mut mathResult: number = 10
        mathResult += 5      # 복합 할당
        mathResult *= 2      # 곱셈 할당
        mathResult >>= 1     # 시프트 할당
        mathResult &= 0xFF   # 비트 AND 할당
        
        # 단항 연산자
        let negated: number = -mathResult
        let incremented: number = ++mathResult
        let bitNot: number = ~mathResult
        let logicalNot: boolean = !isEnabled
        
        # 이진 연산자들
        let sum: number = 10 + 20
        let product: number = 5 * 6
        let quotient: number = 100 / 4
        let remainder: number = 17 % 5
        let leftShift: number = 4 << 2
        let rightShift: number = 16 >> 2
        let bitAnd: number = 0xF0 & 0x0F
        let bitOr: number = 0xF0 | 0x0F
        let bitXor: number = 0xF0 ^ 0x0F
        
        # 논리 연산자
        let andResult: boolean = isEnabled and (mathResult > 0)
        let orResult: boolean = isEnabled or (config.debug == false)
        let comparison: boolean = (mathResult >= 10) and (mathResult <= 100)
        
        # 후위 연산자들
        let arrayAccess: any = config["port"]
        let methodCall: any = serverName.toUpperCase()
        let safeAccess: any = config?.database?.host
        
        onReady(true)
    },
    
    # 복잡한 게임 로직 함수
    fun processGameTick(
        let deltaTime: number,
        let playerInputs: Array<PlayerInput> = [],
        mut worldState: GameWorld = getDefaultWorld(),
        let updateCallback: (GameWorld) -> void = (let world: GameWorld) => {}
    ): GameWorld {
        mut updatedPlayers: Array<Player> = []
        
        # 배열 컴프리헨션과 필터링
        let activePlayers: Array<Player> = [
            player for player in currentPlayers 
            if player.isActive and player.health > 0
        ]
        
        # 복잡한 when 표현식 (모든 패턴 매칭)
        playerProcessing@ for (input in playerInputs) {
            let processedPlayer: Player? = when (input) {
                # 값 매칭
                { type: "move", data: moveData } -> processMovement(input.playerId, moveData),
                { type: "attack", data: attackData } -> processAttack(input.playerId, attackData),
                { type: "chat", data: chatData } -> processChatMessage(input.playerId, chatData),
                
                # 범위 매칭
                i if i.priority in 1..6 -> processHighPriority(i),
                i if i.priority in 6..11 -> processLowPriority(i),
                
                # 타입 매칭 (향후 확장)
                i if i.timestamp is number -> processTimestampedInput(i),
                
                # 가드 조건
                i if i.playerId == "admin" and i.type == "command" -> processAdminCommand(i),
                i if i.banned -> { logBannedAttempt(i); None },
                
                # 다중 값 매칭
                { type: "disconnect", reason: "timeout" }, 
                { type: "disconnect", reason: "quit" } -> processDisconnect(input.playerId),
                
                else -> {
                    console.log(`Unknown input type: ${input.type}`)
                    None
                }
            }
            
            # 옵셔널 처리
            if (processedPlayer != None) {
                updatedPlayers.push(processedPlayer!)
            }
        }
        
        # 고차 함수와 람다들 (기본값 매개변수 포함)
        let gameUpdaters: Array<(GameWorld, number) -> GameWorld> = [
            physics@ (let world: GameWorld, let dt: number = 0.016) => updatePhysics(world, dt),
            ai@ (let world: GameWorld, let difficulty: number = 1.0) => updateAI(world, difficulty),
            effects@ (let world: GameWorld, let quality: number = 1.0) => updateEffects(world, quality)
        ]
        
        # 함수 체이닝과 파이프라인
        let finalWorld: GameWorld = gameUpdaters
            .reduce(
                (let acc: GameWorld, let updater: (GameWorld, number) -> GameWorld) => updater(acc, deltaTime),
                worldState
            )
        
        updateCallback(finalWorld)
        return@processGameTick finalWorld
    },
    
    # 라벨이 있는 람다 표현식들
    .eventHandlers = {
        .onPlayerJoin = playerJoin@ (
            let playerId: string,
            let playerData: PlayerData = getDefaultPlayerData(),
            let notifyOthers: boolean = true
        ) => {
            if (currentPlayers.length >= config.maxPlayers) {
                return@playerJoin { success: false, reason: "Server full" }
            }
            
            let newPlayer: Player = createPlayer(playerId, playerData)
            currentPlayers.push(newPlayer)
            
            if (notifyOthers) {
                broadcastMessage(`Player ${playerId} joined the game`)
            }
            
            return@playerJoin { success: true, player: newPlayer }
        },
        
        .onPlayerLeave = playerLeave@ (
            let playerId: string,
            let reason: string = "quit",
            let saveProgress: boolean = true
        ) => {
            let playerIndex: number = currentPlayers.findIndex(
                (let p: Player) => p.id == playerId
            )
            
            if (playerIndex >= 0) {
                let player: Player = currentPlayers[playerIndex]
                
                if (saveProgress) {
                    database.savePlayerProgress(player)
                }
                
                currentPlayers.removeAt(playerIndex)
                broadcastMessage(`Player ${playerId} left: ${reason}`)
                return@playerLeave true
            }
            
            return@playerLeave false
        }
    },
    
    # 중첩된 제어 흐름과 복잡한 로직
    fun manageServer(): void {
        let serverLoop: boolean = true
        
        mainLoop@ for (serverLoop) {
            let currentTime: number = Date.now()
            let deltaTime: number = currentTime - lastUpdate
            lastUpdate = currentTime
            
            # 모든 제어문 형태 사용
            
            # 조건 없는 if (항상 실행)
            if {
                updateServerMetrics()
            }
            
            # 일반적인 if-elif-else
            if (deltaTime > 100) {
                console.log("Server lagging!")
                optimizePerformance()
            } elif (deltaTime > 50) {
                console.log("Minor lag detected")
            } else {
                # 성능이 좋음
            }
            
            # 다양한 for 루프 형태
            
            # C 스타일 for (init; condition; increment)
            for (mut i: number = 0; i < currentPlayers.length; i += 1) {
                let player: Player = currentPlayers[i]
                
                # when을 statement로 사용 (값 반환 없음)
                when (player.status) {
                    "idle" -> {
                        player.idleTime += deltaTime
                        if (player.idleTime > 300000) {  # 5분
                            kickPlayer(player.id, "idle timeout")
                        }
                    },
                    "playing" -> {
                        updatePlayerState(player, deltaTime)
                    },
                    "disconnected" -> {
                        removePlayer(player.id)
                    }
                }
            }
            
            # while 스타일 for
            networkLoop@ for (hasIncomingData()) {
                let packet: NetworkPacket? = receivePacket()
                
                if (packet == None) break@networkLoop
                
                processNetworkPacket(packet!)
            }
            
            # for-in 루프
            validationLoop@ for (player in currentPlayers) {
                let validationResult: ValidationResult = validatePlayer(player)
                
                when (validationResult.severity) {
                    "critical" -> {
                        banPlayer(player.id, validationResult.reason)
                        continue@validationLoop
                    },
                    "warning" -> {
                        logWarning(player.id, validationResult.reason)
                    },
                    else -> {
                        # 플레이어 정상
                    }
                }
            }
            
            # 초기화만 있는 for
            for (let cleanupTasks: Array<() -> void> = getCleanupTasks(); cleanupTasks.length > 0) {
                let task: () -> void = cleanupTasks.pop()!
                task()
            }
            
            # 조건 없는 for (무한 루프 방지를 위한 break)
            heartbeatLoop@ for {
                sendHeartbeat()
                
                if (shouldShutdown()) {
                    break@heartbeatLoop
                }
                
                sleep(1000)  # 1초 대기
                break@heartbeatLoop  # 한 번만 실행
            }
            
            # 서버 상태 체크
            let serverHealth: ServerHealth = checkServerHealth()
            
            if (!serverHealth.healthy) {
                console.log("Server health issues detected!")
                serverLoop = false
                break@mainLoop
            }
            
            # CPU 사용률 체크
            if (getCurrentCPUUsage() > 0.9) {
                console.log("High CPU usage, throttling...")
                sleep(16)  # 60 FPS 제한
            }
        }
        
        shutdown()
    },
    
    # 객체 프로퍼티들 (shorthand, computed, spread)
    .config = ServerConfig,
    .status = "initializing",
    .["dynamic_prop"] = "computed property",
    .startTime = Date.now(),
    ...getDefaultServerProps(),  # spread operator
    
    # 블록 표현식을 값으로 사용
    .complexCalculation = {
        let base: number = 1000
        let multiplier: number = 1.5
        let bonus: number = when (config.gameMode) {
            "battle_royale" -> 100,
            "team_deathmatch" -> 50,
            "capture_flag" -> 75,
            else -> 25
        }
        
        base * multiplier + bonus  # 마지막 표현식이 반환값
    }
}

# 유틸리티 함수들 (모든 타입 시스템 활용)
fun createPlayer(let id: string, let data: PlayerData = getDefaultPlayerData()): Player = {
    .id = id,
    .name = data.name ?? `Player_${id}`,
    .position = { x: 0.0, y: 0.0, z: 0.0 },
    .health = 100,
    .isActive = true,
    .inventory = [],
    .stats = data.stats ?? getDefaultStats(),
    .joinTime = Date.now()
}

# 복합 타입들
let ProcessResult: any = {
    success: boolean,
    data: any?,
    error: string?
}

let PlayerInput: any = {
    playerId: string,
    type: string,
    data: any,
    timestamp: number,
    priority: number,
    banned: boolean?
}

# 함수 타입들
let EventHandler: any = (string, any) -> boolean
let UpdateFunction: any = (GameWorld, number) -> GameWorld
let ValidationFunction: any = (Player) -> ValidationResult

# 배열과 튜플 타입들
let PlayerList: any = Array<Player>
let Coordinates: any = (number, number, number)  # 3D 좌표 tuple
let ServerStats: any = (number, number, boolean)  # (players, uptime, healthy)

# 옵셔널 타입들  
let MaybePlayer: any = Player?
let OptionalConfig: any = ServerConfig?
let PossibleError: any = string?

# 메인 서버 실행 함수
fun main(): void {
    console.log("Starting nugdev Game Server...")
    
    # TODO: try/catch는 향후 확장 기능
    # let serverInstance: GameServer? = try {
    #     let server: GameServer = createGameServer()
    #     server.initialize()
    #     server
    # } catch (error) {
    #     console.error(`Failed to start server: ${error.message}`)
    #     None
    # }
    
    # 임시 구현 (try/catch 없이)
    let server: GameServer = createGameServer()
    server.initialize()
    let serverInstance: GameServer? = server
    
    if (serverInstance != None) {
        console.log("Server started successfully!")
        serverInstance!.manageServer()
    } else {
        console.error("Server startup failed!")
        exit(1)
    }
}

# 모든 리터럴과 표현식을 테스트하는 함수
fun testAllFeatures(): void {
    # 모든 수치 리터럴
    let numbers: Array<number> = [
        42,           # 십진수
        0xFF,         # 16진수  
        0b1010,       # 이진수
        0o755,        # 8진수
        3.14159,      # 부동소수점
        6.022e23,     # 과학적 표기법
        1_000_000     # 언더스코어 구분
    ]
    
    # 모든 문자열 리터럴  
    let strings: Array<string> = [
        "basic string",
        'single char',
        r"raw string \n no escape",
        `template ${42} string`,
        "escaped \"quotes\" and \n newlines"
    ]
    
    # 모든 불린과 특수 값들
    let values: Array<any> = [
        true,
        false, 
        null,
        None
    ]
    
    # 범위들
    let ranges: Array<any> = [
        1..10,        # exclusive range (1부터 9까지)
        10..          # unbounded range
    ]
    
    # 복잡한 표현식 조합
    let complexExpr: any = (numbers[0] + numbers[1]) * 
                          (strings.length > 0 ? 1 : 0) +
                          (values[0] and values[1] ? 10 : 5) ??
                          (ranges[0] != None ? 100 : 0)
    
    console.log(`Complex expression result: ${complexExpr}`)
}

# 프로그램 시작점
main()
testAllFeatures()