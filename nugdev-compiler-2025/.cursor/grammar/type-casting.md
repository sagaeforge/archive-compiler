# Type Casting and Type Checking

nugdev는 Kotlin 스타일의 타입 캐스팅과 타입 검사를 지원합니다.

## Type Checking with `is`

### EBNF

```ebnf
type_condition = expression "is" type_literal ;
```

### 기본 사용법

```nugdev
fun processValue(let value: any): string {
    if (value is string) {
        # 이 블록 내에서 value는 자동으로 string으로 취급
        return value.toUpperCase()
    } elif (value is number) {
        # 이 블록 내에서 value는 자동으로 number로 취급
        return value.toString()
    } elif (value is User) {
        # 이 블록 내에서 value는 자동으로 User로 취급
        return value.name
    } else {
        return "Unknown type"
    }
}

# when 표현식에서 사용
let description: string = when (data) {
    is string -> `String: ${data}`,
    is number -> `Number: ${data}`,
    is boolean -> `Boolean: ${data}`,
    is User -> `User: ${data.name}`,
    else -> "Unknown type"
}
```

### 배열과 복합 타입 검사

```nugdev
fun analyzeCollection(let collection: any): string {
    return when (collection) {
        is Array<string> -> "String array",
        is Array<number> -> "Number array", 
        is Array<User> -> "User array",
        is User -> "Single user",
        else -> "Unknown collection type"
    }
}

# 옵셔널 타입 검사
fun handleOptional(let value: any): string {
    if (value is string?) {
        if (value != None) {
            return `Optional string: ${value!}`
        } else {
            return "None value"
        }
    }
    return "Not an optional string"
}
```

## Type Casting

### EBNF

```ebnf
cast_operator = "as" type_literal      (* unsafe cast *)
              | "as?" type_literal ;   (* safe cast *)
```

### 1. Unsafe Casting (`as`)

강제 캐스팅으로, 실패시 런타임 에러가 발생합니다.

```nugdev
# 기본 타입 캐스팅
let value: any = "42"
let num: number = value as number     # 런타임 에러!

let userObj: any = { id: 1, name: "Alice", email: "alice@example.com" }
let user: User = userObj as User      # 성공 (구조가 맞으면)

# 함수에서 사용
fun processUser(let data: any): void {
    let user: User = data as User
    print(`Processing user: ${user.name}`)
}

# 체이닝
let result: string = (getValue() as User).name as string
```

### 2. Safe Casting (`as?`)

안전한 캐스팅으로, 실패시 `None`을 반환합니다.

```nugdev
# 기본 안전 캐스팅
let value: any = "hello"
let num: number? = value as? number   # None 반환

let userObj: any = { id: 1, name: "Alice" }
let user: User? = userObj as? User    # User? 타입 반환

# 옵셔널 체이닝과 함께 사용
let userName: string = (data as? User)?.name ?? "Unknown"

# 안전한 배열 캐스팅
let numbers: Array<number>? = collection as? Array<number>
if (numbers != None) {
    print(`Array has ${numbers!.length} numbers`)
}
```

### 3. 실용적 예시

#### API 응답 처리

```nugdev
interface ApiResponse {
    success: boolean,
    data: any,
    error: string?
}

fun handleApiResponse(let response: any): void {
    let apiResp: ApiResponse? = response as? ApiResponse
    
    if (apiResp == None) {
        print("Invalid API response format")
        return
    }
    
    if (!apiResp!.success) {
        print(`API Error: ${apiResp!.error ?? "Unknown error"}`)
        return
    }
    
    # 데이터 타입별 처리
    when (apiResp!.data) {
        is User -> {
            let user: User = apiResp!.data as User
            print(`User: ${user.name}`)
        },
        is Array<User> -> {
            let users: Array<User> = apiResp!.data as Array<User>
            print(`Users count: ${users.length}`)
        },
        else -> print("Unknown data type")
    }
}
```

#### 다형성 처리

```nugdev
interface Shape {
    area(): number
}

struct Circle {
    radius: number,
    area: () -> number
}

struct Rectangle {
    width: number,
    height: number,
    area: () -> number
}

fun processShapes(let shapes: Array<any>): void {
    for (shape in shapes) {
        # 타입 검사 후 안전한 캐스팅
        if (shape is Circle) {
            let circle: Circle = shape as Circle
            print(`Circle area: ${circle.area()}`)
        } elif (shape is Rectangle) {
            let rect: Rectangle = shape as Rectangle
            print(`Rectangle area: ${rect.area()}`)
        } else {
            # 안전한 캐스팅으로 Shape 인터페이스 시도
            let shapeObj: Shape? = shape as? Shape
            if (shapeObj != None) {
                print(`Unknown shape area: ${shapeObj!.area()}`)
            } else {
                print("Not a valid shape")
            }
        }
    }
}
```

#### JSON 파싱

```nugdev
fun parseUserFromJson(let jsonData: any): User? {
    # 먼저 기본 구조 검증
    if (!(jsonData is object)) {
        return None
    }
    
    let obj: object = jsonData as object
    
    # 안전한 필드 추출
    let id: number? = obj.id as? number
    let name: string? = obj.name as? string
    let email: string? = obj.email as? string
    let active: boolean? = obj.active as? boolean
    
    # 모든 필수 필드가 있는지 확인
    if (id == None or name == None or email == None or active == None) {
        return None
    }
    
    # 안전하게 User 객체 생성
    return {
        id: id!,
        name: name!,
        email: email!,
        active: active!
    }
}
```

## 타입 가드와 스마트 캐스팅

### Smart Casting

`is` 검사 후 자동으로 타입이 좁혀집니다.

```nugdev
fun smartCastExample(let value: any): string {
    if (value is string) {
        # 여기서 value는 자동으로 string 타입
        return value.length.toString()  # 캐스팅 불필요
    }
    
    if (value is User and value.active) {
        # 여기서 value는 자동으로 User 타입이고 active가 true
        return `Active user: ${value.name}`
    }
    
    return "Unknown"
}
```

### 커스텀 타입 가드

```nugdev
fun isValidUser(let obj: any): boolean {
    return obj is object and
           obj.id is number and
           obj.name is string and
           obj.email is string and
           obj.active is boolean
}

fun processUserData(let data: any): void {
    if (isValidUser(data)) {
        # isValidUser 함수가 true를 반환하면 User로 안전하게 캐스팅 가능
        let user: User = data as User
        print(`Processing: ${user.name}`)
    } else {
        print("Invalid user data")
    }
}
```

## 성능 고려사항

### 🚀 권장사항

1. **Safe casting 우선 사용**: `as?`를 사용하여 런타임 에러 방지
2. **타입 검사 후 unsafe casting**: `is` 검사 후 `as` 사용
3. **Smart casting 활용**: `is` 검사 후 자동 타입 좁혀짐 활용

### ⚠️ 주의사항

1. **Unsafe casting은 신중히**: `as`는 런타임 에러 가능성
2. **성능 고려**: 빈번한 타입 검사/캐스팅은 성능 영향
3. **타입 설계**: 가능하면 명확한 타입 구조 설계로 캐스팅 최소화

```nugdev
# ❌ 비추천: 빈번한 unsafe casting
for (item in items) {
    let user: User = item as User  # 매번 런타임 에러 위험
    processUser(user)
}

# ✅ 추천: Safe casting과 검증
for (item in items) {
    let user: User? = item as? User
    if (user != None) {
        processUser(user!)
    }
}

# ✅ 더 좋음: 타입 안전한 설계
for (user in typedUsers) {  # Array<User> 타입
    processUser(user)  # 캐스팅 불필요
}
```

이제 nugdev에서 안전하고 효율적인 타입 캐스팅을 할 수 있습니다! 🎯 