# Program Structure

nugdev 언어의 프로그램과 모듈 구조를 정의합니다.

## Program

프로그램은 여러 모듈의 집합입니다.

### EBNF

```ebnf
program = { module } ;
```

### Examples

```nugdev
# main.nug (메인 모듈)
import "std/io" as io
import "utils/math" 
import "config/settings" as config

export let VERSION: string = "1.0.0"

fun main(): void {
    io.print("nugdev Compiler ${VERSION}")
    let result: number = math.fibonacci(10)
    io.print("Fibonacci(10) = ${result}")
}

# math.nug (수학 유틸리티 모듈)
export fun fibonacci(let n: number): number {
    if (n <= 1) return n
    return fibonacci(n - 1) + fibonacci(n - 2)
}

export fun factorial(let n: number): number {
    if (n <= 1) return 1
    return n * factorial(n - 1)
}

export let PI: number = 3.14159
export let E: number = 2.71828

# 내부 전용 유틸리티 (export 없음)
fun validateInput(let input: number): boolean {
    return input >= 0 && input <= 1000
}
```

## Module

모듈은 하나의 소스 파일을 의미하며, 구문들의 연속입니다.

### EBNF

```ebnf
module = { import_statement | export_statement | statement } ;

import_statement = "import" string_literal [ "as" identifier ] [ ";" ] ;
export_statement = "export" statement ;
```

### Examples

```nugdev
# 모듈 가져오기
import "std/io"
import "utils/string" as str
import "config/database" as db

# 모듈 내보내기  
export let MAX_CONNECTIONS: number = 100

export fun connect(let url: string): Connection {
    # 연결 로직
    return createConnection(url)
}

# 내부 전용 함수 (export 없음)
fun validateUrl(let url: string): boolean {
    return url.startsWith("http")
}
```

## Module System Features

### Re-exports

```nugdev
# utils/index.nug
export import "utils/string"
export import "utils/array" 
export import "utils/object"

# 선택적 re-export
export { map, filter, reduce } from "utils/array"

## Conditional Imports

```nugdev
# 플랫폼별 조건부 import
import when (platform) {
    "web": "platform/web",
    "node": "platform/node", 
    "native": "platform/native",
    else: "platform/generic"
} as platform

# 동적 import (비동기)
let module: Promise<Module> = import("runtime/heavy-module")
let math: MathModule = await module

# 조건부 동적 로딩
if (config.enableAdvancedFeatures) {
    let advanced: AdvancedModule = await import("features/advanced")
    registerAdvancedFeatures(advanced)
}
```

### Dynamic Imports (향후 확장)

```nugdev
# 동적 import (비동기)
let loadModule = async function(moduleName) {
    let module = await import(moduleName);
    return module;
};

# 조건부 동적 로딩
let math = if (needAdvancedMath) {
    await import("./advanced-math")
} else {
    await import("./basic-math")
};
```

## Module Resolution

### Relative vs Absolute Paths

```nugdev
# utils/index.nug
export import "utils/string"
export import "utils/array" 
export import "utils/object"

# 선택적 re-export
export { map, filter, reduce } from "utils/array"

## Conditional Imports

```nugdev
# 플랫폼별 조건부 import
import when (platform) {
    "web": "platform/web",
    "node": "platform/node", 
    "native": "platform/native",
    else: "platform/generic"
} as platform

# 동적 import (비동기)
let module: Promise<Module> = import("runtime/heavy-module")
let math: MathModule = await module

# 조건부 동적 로딩
if (config.enableAdvancedFeatures) {
    let advanced: AdvancedModule = await import("features/advanced")
    registerAdvancedFeatures(advanced)
}
```

### Module Search Paths

```nugdev
# 절대 경로 (표준 라이브러리)
import "std/io";
import "std/collections/array";

# 상대 경로
import "./utils";           # utils.nug 또는 utils/index.nug
import "../shared/config";  # 상위 디렉토리
import "./math/advanced";   # 하위 디렉토리

# 패키지 경로
import "lodash";           # node_modules/lodash
import "@company/utils";   # 스코프 패키지
```

## Module Metadata

### package.json

```json
{
    "name": "my-nugdev-project",
    "version": "1.0.0",
    "main": "src/index.nug",
    "dependencies": {
        "@nugdev/std": "^1.0.0",
        "http-client": "^2.1.0"
    },
    "nugdev": {
        "target": "native",
        "optimization": "release"
    }
}
```

### Module Attributes

```nugdev
# 모듈 레벨 속성
#[version("1.0.0")]
#[author("nugdev team")]
#[license("MIT")]
module;

# 조건부 컴파일
#[target("web")]
export let browserUtils = { /* ... */ };

#[target("native")]
export let systemUtils = { /* ... */ };
```

## Circular Dependencies

```nugdev
# a.nug
import "./b" as B;
export let aValue = "from A";
export let useB = function() B.bValue;

# b.nug  
import "./a" as A;
export let bValue = "from B";
export let useA = function() A.aValue;

# 순환 참조는 허용되지만 초기화 시점 주의
```

## Module Visibility

```nugdev
# public export (기본)
export let publicFunction = function() "public";

# private (export 없음)
let privateFunction = function() "private";

# internal export (같은 패키지 내에서만)
internal export let internalFunction = function() "internal";

# protected export (상속 관계에서만)
protected export let protectedFunction = function() "protected";
``` 