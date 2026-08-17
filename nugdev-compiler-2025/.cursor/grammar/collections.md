# Collections

배열과 객체 리터럴, 그리고 컬렉션 조작 표현식들입니다.

## Array Literals

배열 리터럴은 순서가 있는 요소들의 집합을 정의합니다.

### EBNF

```ebnf
array_literal = "[" [ expression_list ] "]" 
              | array_comprehension ;

array_comprehension = "[" expression "for" identifier "in" expression [ "if" expression ] "]" ;

expression_list = expression { "," expression } [ "," ] ;
```

### Examples

```nugdev
# 기본 배열 리터럴
let numbers: Array<number> = [1, 2, 3, 4, 5];
let names: Array<string> = ["Alice", "Bob", "Charlie"];
let mixed: Array<any> = [1, "hello", true, null];

# 빈 배열
let empty: Array<string> = [];

# 타입 명시적 배열
let scores: Array<number> = [95, 87, 92, 78, 85];
let flags: Array<boolean> = [true, false, true];

# 배열 컴프리헨션
let squares: Array<number> = [x * x for x in 1..10];
let evens: Array<number> = [x for x in 1..20 if x % 2 == 0];
let uppercased: Array<string> = [name.toUpperCase() for name in names];

# 중첩 배열
let matrix: Array<Array<number>> = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];

# 함수를 포함한 배열
let operations: Array<(number, number) -> number> = [
    (let a: number, let b: number) => a + b,
    (let a: number, let b: number) => a - b,
    (let a: number, let b: number) => a * b,
    (let a: number, let b: number) => a / b
];
```

## Object Literals

객체 리터럴은 키-값 쌍의 집합을 정의합니다.

### EBNF

```ebnf
object_literal = "{" [ property_list ] "}" ;

property_list = property { "," property } [ "," ] ;

property = property_name ":" expression
         | "[" expression "]" ":" expression    (* computed property *)
         | identifier                           (* shorthand property *)
         | "." identifier "=" expression        (* shorthand assignment *)
         | "..." expression ;                   (* spread operator *)

property_name = identifier | string_literal | number_literal ;
```

### Examples

```nugdev
# 기본 객체 리터럴
let user: User = {
    name: "Alice",
    age: 30,
    email: "alice@example.com",
    active: true
};

# Shorthand 할당 문법
let person: Person = {
    .name = "Bob",
    .age = 25,
    .city = "Seoul"
};

# 혼합 문법
let config: Config = {
    version: "1.0.0",
    .debug = false,
    .port = 3000,
    features: ["auth", "logging"]
};

# 빈 객체
let empty: Record<string, any> = {};

# 계산된 프로퍼티
let dynamicKey: string = "status";
let data: Record<string, any> = {
    name: "Test",
    [dynamicKey]: "active",
    ["computed_" + "key"]: "value"
};

# Shorthand 프로퍼티
let name: string = "Charlie";
let age: number = 35;
let shorthand: Person = { name, age };  # { name: name, age: age }와 동일

# 중첩 객체
let company: Company = {
    .name = "TechCorp",
    .address = {
        .street = "123 Main St",
        .city = "Seoul",
        .zipCode = "12345"
    },
    .employees = [
        { .name = "Alice", .role = "Developer" },
        { .name = "Bob", .role = "Designer" }
    ]
};

# 메서드가 포함된 객체
let calculator: Calculator = {
    .value = 0,
    .add = (let self: Calculator, let n: number) => {
        self.value += n;
        self
    },
    .multiply = (let self: Calculator, let n: number) => {
        self.value *= n;
        self
    },
    .getValue = (let self: Calculator) => self.value
};

# 스프레드 연산자
let baseConfig: BaseConfig = { .timeout = 5000, .retries = 3 };
let extendedConfig: ExtendedConfig = {
    ...baseConfig,
    .cache = true,
    .debug = false
};
```

## Destructuring Assignment

### EBNF

```ebnf
destructuring_assignment = array_destructuring | object_destructuring ;

array_destructuring = "[" destructuring_pattern { "," destructuring_pattern } "]" "=" expression ;
object_destructuring = "{" destructuring_property { "," destructuring_property } "}" "=" expression ;

destructuring_pattern = identifier 
                      | "[" destructuring_pattern { "," destructuring_pattern } "]"
                      | "..." identifier ;

destructuring_property = identifier [ ":" identifier ]
                       | "..." identifier ;
```

### Examples

```nugdev
# 배열 구조 분해
let [first, second, third] = [1, 2, 3];
let [head, ...tail] = [1, 2, 3, 4, 5];  # head = 1, tail = [2, 3, 4, 5]

# 중첩 배열 구조 분해
let [[a, b], [c, d]] = [[1, 2], [3, 4]];

# 객체 구조 분해
let { name, age } = person;
let { x, y, z = 0 } = coordinate;  # 기본값 설정

# 속성 이름 변경
let { name: fullName, age: years } = person;

# 중첩 객체 구조 분해
let { 
    database: { host, port },
    features: { logging }
} = config;

# 나머지 속성
let { name, ...otherProps } = user;

# 함수 매개변수에서 구조 분해
let greetUser = fun(let param: any): string {
    let { name, age = 0 } = param;
    `Hello ${name}, you are ${age} years old.`
};

let processPoint = fun(let point: Array<number>): any {
    let [x, y] = point;
    { x: x, y: y, distance: Math.sqrt(x*x + y*y) }
};
```

## Collection Methods

### Examples

```nugdev
# 배열 메서드 체이닝
let processed: number = numbers
    .filter((let x: number) => x > 0)
    .map((let x: number) => x * 2)
    .reduce((let acc: number, let x: number) => acc + x, 0);

# 객체 메서드
let keys: Array<string> = Object.keys(person);
let values: Array<any> = Object.values(person);
let entries: Array<Array<any>> = Object.entries(person);

# 변환 메서드
let mapped: Array<string> = array.map((let item: string) => item.toUpperCase());
let filtered: Array<string> = array.filter((let item: string) => item.length > 3);
let found: any = array.find((let item: any) => item.id == targetId);

# 집계 메서드
let sum: number = numbers.reduce((let acc: number, let x: number) => acc + x, 0);
let max: number = numbers.reduce((let acc: number, let x: number) => if (x > acc) x else acc);
let concatenated: string = strings.reduce((let acc: string, let s: string) => acc + s, "");

# 불변 업데이트
let updated: Array<any> = array.with(index, newValue);
let appended: Array<any> = array.push(newItem);
let removed: Array<any> = array.filter((let item: any) => item.id != removeId);

# 객체 업데이트
let updatedPerson: Person = { ...person, age: person.age + 1 };
let withNewField: PersonWithStatus = { ...person, status: "active" };
```

## Set and Map Literals (향후 확장)

### Examples

```nugdev
# Set 리터럴
let uniqueNumbers = #{1, 2, 3, 2, 1};  # {1, 2, 3}
let stringSet = #{"apple", "banana", "apple"};  # {"apple", "banana"}

# Map 리터럴
let userMap = %{
    "alice": { age: 30, role: "admin" },
    "bob": { age: 25, role: "user" }
};

# Set 연산
let set1 = #{1, 2, 3};
let set2 = #{2, 3, 4};
let union = set1 | set2;        # {1, 2, 3, 4}
let intersection = set1 & set2;  # {2, 3}
let difference = set1 - set2;    # {1}

# Map 연산
let mergedMap = map1 + map2;     # merge maps
let filteredMap = userMap.filter((let k: string, let v: any) => v.age > 25);
```

## Tuple Literals (향후 확장)

### Examples

```nugdev
# 튜플 리터럴
let point = (10, 20);
let person = ("Alice", 30, "Engineer");
let nested = ((1, 2), (3, 4));

# 튜플 구조 분해
let (x, y) = point;
let (name, age, profession) = person;

# 명명된 튜플
let namedTuple = (x: 10, y: 20, z: 5);
let { x, y } = namedTuple;

# 튜플 메서드
let length = person.length();    # 3
let first = person.first();      # "Alice"
let last = person.last();        # "Engineer"
``` 
 