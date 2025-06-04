# Type System

nugdev의 강화된 타입 시스템으로 덕타이핑의 문제를 해결합니다.

## 문제점: 덕타이핑의 한계

### 기존 방식의 문제

```nugdev
# 덕타이핑 - 타입 안전성 부족
let user: any = {
    id: 1,
    name: "Alice"
    # email 필드 누락 - 런타임에서만 발견됨
}

fun sendEmail(let user: any): void {
    # 컴파일 타임에 user.email 존재 여부를 알 수 없음
    emailService.send(user.email)  # 런타임 에러 가능
}
```

## 해결책: 명시적 타입 시스템

### 1. 구조체 (Struct) 시스템

#### EBNF

```ebnf
struct_declaration = "struct" identifier "{" [ struct_field_list ] "}" [ ";" ] ;
struct_field_list = struct_field { "," struct_field } [ "," ] ;
struct_field = identifier ":" type_literal ;
```

#### 기본 구조체

```nugdev
# 사용자 정의 구조체
struct User {
    id: number,
    name: string,
    email: string,
    active: boolean
}

# 타입 안전한 인스턴스 생성
let user: User = {
    id: 1,
    name: "Alice",
    email: "alice@example.com",
    active: true
}

# 컴파일 에러: 필수 필드 누락
# let invalidUser: User = {
#     id: 1,
#     name: "Bob"  # email, active 누락 - 컴파일 에러!
# }
```

#### 고급 구조체 패턴

```nugdev
# 옵셔널 필드
struct UserProfile {
    user: User,
    avatar: string?,           # 옵셔널
    lastLogin: number?,        # 옵셔널
    preferences: UserSettings
}

# 중첩 구조체
struct Address {
    street: string,
    city: string,
    country: string,
    zipCode: string
}

struct Company {
    name: string,
    address: Address,          # 중첩 구조체
    employees: Array<User>     # 구조체 배열
}

# 함수 타입 필드
struct EventHandlers {
    onUserLogin: (User) -> void,
    onUserLogout: (string) -> boolean,
    onError: (Error) -> void
}
```

### 2. 인터페이스 (Interface) 시스템

#### EBNF

```ebnf
interface_declaration = "interface" identifier "{" [ interface_member_list ] "}" [ ";" ] ;
interface_member_list = interface_member { "," interface_member } [ "," ] ;
interface_member = identifier ":" type_literal
                 | method_signature ;
method_signature = identifier "(" [ parameter_list ] ")" ":" type_literal ;
```

#### 기본 인터페이스

```nugdev
# 계약 정의
interface Drawable {
    x: number,
    y: number,
    visible: boolean,
    
    draw(): void,
    move(let deltaX: number, let deltaY: number): void,
    hide(): void,
    show(): void
}

# 인터페이스 구현
let circle: Drawable = {
    x: 10,
    y: 20,
    visible: true,
    
    draw: (): void => {
        if (circle.visible) {
            console.log(`Drawing circle at (${circle.x}, ${circle.y})`)
        }
    },
    
    move: (let deltaX: number, let deltaY: number): void => {
        circle.x += deltaX
        circle.y += deltaY
    },
    
    hide: (): void => { circle.visible = false },
    show: (): void => { circle.visible = true }
}
```

#### 서비스 인터페이스

```nugdev
# 비즈니스 로직 인터페이스
interface UserRepository {
    findById(let id: number): User?,
    findByEmail(let email: string): User?,
    save(let user: User): boolean,
    delete(let id: number): boolean,
    findAll(): Array<User>
}

# 게임 서비스 인터페이스
interface GameService {
    players: Array<Player>,
    maxPlayers: number,
    
    addPlayer(let player: Player): boolean,
    removePlayer(let playerId: string): Player?,
    broadcast(let message: string): void,
    getPlayerCount(): number,
    isGameFull(): boolean
}
```

### 3. 타입 안전성 활용

#### 함수에서 구조체 사용

```nugdev
# 타입 안전한 함수
fun updateUser(mut user: User, let updates: UserUpdate): User {
    # IDE에서 자동완성 지원
    if (updates.name) user.name = updates.name
    if (updates.email) user.email = updates.email
    if (updates.active != None) user.active = updates.active!
    
    return user
}

# 구조체 유효성 검증
fun validateUser(let user: User): ValidationResult {
    mut errors: Array<string> = []
    
    if (user.name.length == 0) {
        errors.push("Name cannot be empty")
    }
    
    if (!isValidEmail(user.email)) {
        errors.push("Invalid email format")
    }
    
    return {
        valid: errors.length == 0,
        errors: errors
    }
}
```

#### 인터페이스를 통한 다형성

```nugdev
# 다양한 도형 구현
let shapes: Array<Drawable> = [
    circle,  # 위에서 정의한 circle
    {
        x: 0, y: 0, visible: true,
        draw: (): void => console.log("Drawing rectangle"),
        move: (let dx: number, let dy: number): void => {},
        hide: (): void => {},
        show: (): void => {}
    },
    {
        x: 50, y: 50, visible: true,
        draw: (): void => console.log("Drawing triangle"),
        move: (let dx: number, let dy: number): void => {},
        hide: (): void => {},
        show: (): void => {}
    }
]

# 타입 안전한 다형성 활용
fun renderAll(let objects: Array<Drawable>): void {
    for (obj in objects) {
        obj.draw()  # 컴파일 타임에 메서드 존재 보장
    }
}
```

### 4. 혼합 접근: 유연성과 안전성

#### 점진적 타이핑

```nugdev
# 프로토타이핑에서는 any 사용 가능
let prototypeData: any = {
    randomField: "some value",
    dynamicProperty: 42
}

# 안정화 단계에서 구조체로 전환
struct FinalizedData {
    processedValue: string,
    calculatedResult: number,
    timestamp: number
}

let finalData: FinalizedData = {
    processedValue: prototypeData.randomField,
    calculatedResult: prototypeData.dynamicProperty * 2,
    timestamp: Date.now()
}
```

#### 타입 가드와 캐스팅

```nugdev
# 런타임 타입 체크
fun processUserData(let data: any): User? {
    # 타입 가드
    if (data is object and 
        data.id is number and 
        data.name is string and 
        data.email is string) {
        
        # 안전한 캐스팅
        return data as? User  # Safe casting 사용
    }
    
    return None
}

# 옵셔널 체이닝과 결합
fun getDisplayName(let userData: any): string {
    let user: User? = processUserData(userData)
    return user?.name ?? "Unknown User"
}
```

## 장점 요약

### ✅ 타입 안전성
- 컴파일 타임 에러 검출
- 필수 필드 누락 방지
- 타입 불일치 방지

### ✅ 개발자 경험
- IDE 자동완성 지원
- 리팩토링 안전성
- API 계약 명확성

### ✅ 유지보수성
- 코드 가독성 향상
- 런타임 에러 감소
- 문서화 효과

### ✅ 유연성 유지
- 필요시 any 타입 사용 가능
- 점진적 타이핑 지원
- 덕타이핑과 공존 가능

이렇게 **구조체와 인터페이스 시스템**을 도입하면서도 **기존의 유연성을 유지**하여 nugdev가 더욱 실용적이고 안전한 언어가 됩니다! 🚀