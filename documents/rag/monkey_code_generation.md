# Monkey 언어 가상 코드 생성 동작 원리 분석

## 목차
1. [코드 표현 (code.go)](#1-코드-표현-codego)
2. [컴파일러 (compiler.go)](#2-컴파일러-compilergo)
3. [심볼 테이블 (symbol_table.go)](#3-심볼-테이블-symbol_tablego)
4. [가상 머신 (vm.go)](#4-가상-머신-vmgo)
5. [프레임 (frame.go)](#5-프레임-framego)
6. [객체 시스템 (object.go)](#6-객체-시스템-objectgo)
7. [REPL (repl.go)](#7-repl-replgo)

---

## 1. 코드 표현 (code.go)

### 1.1 바이트코드 표현

```go
// 명령어 배열을 바이트 슬라이스로 표현
type Instructions []byte

// 연산 코드는 단일 바이트로 표현
type Opcode byte

// 주요 연산 코드 정의
const (
    OpConstant Opcode = iota  // 상수 로드
    OpAdd                     // 더하기 연산
    OpPop                     // 스택에서 값 제거
    // 기타 연산 코드...
)

// 각 연산 코드의 메타데이터 정의
type Definition struct {
    Name          string  // 연산 이름
    OperandWidths []int   // 피연산자 크기 (바이트 단위)
}

// 연산 코드별 정의 매핑
var definitions = map[Opcode]*Definition{
    OpConstant: {"OpConstant", []int{2}},  // 2바이트 피연산자
    OpAdd:      {"OpAdd", []int{}},        // 피연산자 없음
    // 기타 정의...
}
```

### 1.2 명령어 생성 및 디코딩

```go
// 명령어 생성: Opcode와 피연산자를 바이트코드로 인코딩
func Make(op Opcode, operands ...int) []byte {
    def, ok := definitions[op]
    if !ok {
        return []byte{}
    }

    instructionLen := 1  // Opcode 용 1바이트
    for _, w := range def.OperandWidths {
        instructionLen += w  // 피연산자 크기 더하기
    }

    instruction := make([]byte, instructionLen)
    instruction[0] = byte(op)  // 첫 바이트에 Opcode 저장

    offset := 1
    for i, o := range operands {
        width := def.OperandWidths[i]
        switch width {
        case 2:
            // 2바이트 피연산자를 빅 엔디안으로 인코딩
            binary.BigEndian.PutUint16(instruction[offset:], uint16(o))
        }
        offset += width
    }

    return instruction
}

// 명령어 디코딩: 바이트코드에서 피연산자 추출
func ReadOperands(def *Definition, ins Instructions) ([]int, int) {
    operands := make([]int, len(def.OperandWidths))
    offset := 0

    for i, width := range def.OperandWidths {
        switch width {
        case 2:
            // 2바이트 피연산자를 빅 엔디안으로 디코딩
            operands[i] = int(binary.BigEndian.Uint16(ins[offset:]))
        }
        offset += width
    }

    return operands, offset
}
```

### 1.3 바이트코드 구조체

```go
// 컴파일된 바이트코드를 표현하는 구조체
type Bytecode struct {
    Instructions Instructions        // 명령어 배열
    Constants    []object.Object     // 상수 풀
}
```

---

## 2. 컴파일러 (compiler.go)

### 2.1 컴파일러 구조체

```go
// 컴파일러 상태를 유지하는 구조체
type Compiler struct {
    constants []object.Object        // 상수 풀
    symbolTable *SymbolTable         // 심볼 테이블
    
    scopes []CompilationScope        // 컴파일 스코프 스택
    scopeIndex int                   // 현재 스코프 인덱스
}

// 컴파일 스코프 구조체
type CompilationScope struct {
    instructions        code.Instructions  // 현재 스코프의 명령어들
    lastInstruction     EmittedInstruction // 마지막 명령어
    previousInstruction EmittedInstruction // 이전 명령어
}

// 명령어 메타데이터
type EmittedInstruction struct {
    Opcode   code.Opcode  // 연산 코드
    Position int          // 명령어 시작 위치
}
```

### 2.2 컴파일 메서드

```go
// AST 노드를 바이트코드로 컴파일
func (c *Compiler) Compile(node ast.Node) error {
    switch node := node.(type) {
    case *ast.Program:
        // 프로그램 노드는 각 문장을 순서대로 컴파일
        for _, s := range node.Statements {
            err := c.Compile(s)
            if err != nil {
                return err
            }
        }
    
    case *ast.ExpressionStatement:
        // 표현식 문장은 표현식을 컴파일하고 결과 값을 제거
        err := c.Compile(node.Expression)
        if err != nil {
            return err
        }
        c.emit(code.OpPop)
    
    case *ast.InfixExpression:
        // 중위 표현식 처리 (예: a + b)
        // 특수 케이스: < 연산자는 > 연산자로 피연산자 순서 바꿔서 처리
        if node.Operator == "<" {
            err := c.Compile(node.Right)
            if err != nil {
                return err
            }
            err = c.Compile(node.Left)
            if err != nil {
                return err
            }
            c.emit(code.OpGreaterThan)
            return nil
        }
        
        // 일반적인 경우: 왼쪽, 오른쪽 피연산자 컴파일 후 연산자 코드 생성
        err := c.Compile(node.Left)
        if err != nil {
            return err
        }
        err = c.Compile(node.Right)
        if err != nil {
            return err
        }
        
        switch node.Operator {
        case "+":
            c.emit(code.OpAdd)
        case "-":
            c.emit(code.OpSub)
        // 기타 연산자 처리...
        }
    
    case *ast.IntegerLiteral:
        // 정수 리터럴은 상수 풀에 추가하고 OpConstant 명령어 생성
        integer := &object.Integer{Value: node.Value}
        c.emit(code.OpConstant, c.addConstant(integer))
    
    case *ast.LetStatement:
        // let 문장 처리: 값 계산 후 전역/지역 변수에 저장
        err := c.Compile(node.Value)
        if err != nil {
            return err
        }
        
        symbol := c.symbolTable.Define(node.Name.Value)
        if symbol.Scope == GlobalScope {
            c.emit(code.OpSetGlobal, symbol.Index)
        } else {
            c.emit(code.OpSetLocal, symbol.Index)
        }
    
    case *ast.Identifier:
        // 식별자 처리: 변수, 내장 함수, 자유 변수 참조
        symbol, ok := c.symbolTable.Resolve(node.Value)
        if !ok {
            return fmt.Errorf("undefined variable %s", node.Value)
        }
        
        c.loadSymbol(symbol)
    
    case *ast.FunctionLiteral:
        // 함수 리터럴 처리: 함수 본문용 새 컴파일 스코프 생성
        c.enterScope()
        
        // 매개변수 정의
        for _, p := range node.Parameters {
            c.symbolTable.Define(p.Value)
        }
        
        // 함수 본문 컴파일
        err := c.Compile(node.Body)
        if err != nil {
            return err
        }
        
        // 명시적 return 없는 경우 null 반환 추가
        if c.lastInstructionIs(code.OpPop) {
            c.replaceLastPopWithReturn()
        }
        if !c.lastInstructionIs(code.OpReturnValue) {
            c.emit(code.OpReturn)
        }
        
        // 자유 변수 처리
        freeSymbols := c.symbolTable.FreeSymbols
        numLocals := c.symbolTable.numDefinitions
        instructions := c.leaveScope()
        
        // 클로저 생성
        for _, s := range freeSymbols {
            c.loadSymbol(s)
        }
        
        compiledFn := &object.CompiledFunction{
            Instructions:  instructions,
            NumLocals:     numLocals,
            NumParameters: len(node.Parameters),
        }
        
        // 클로저 생성
        fnIndex := c.addConstant(compiledFn)
        c.emit(code.OpClosure, fnIndex, len(freeSymbols))
    
    // 기타 노드 타입 처리...
    }
    
    return nil
}
```

### 2.3 명령어 생성 및 바이트코드 반환

```go
// 상수 풀에 객체 추가
func (c *Compiler) addConstant(obj object.Object) int {
    c.constants = append(c.constants, obj)
    return len(c.constants) - 1  // 상수 인덱스 반환
}

// 명령어 생성 및 현재 스코프에 추가
func (c *Compiler) emit(op code.Opcode, operands ...int) int {
    ins := code.Make(op, operands...)  // 명령어 생성
    pos := c.addInstruction(ins)       // 스코프에 명령어 추가
    
    // 명령어 기록 (최적화, 디버깅용)
    c.setLastInstruction(op, pos)
    
    return pos  // 명령어 위치 반환
}

// 최종 바이트코드 생성
func (c *Compiler) Bytecode() *code.Bytecode {
    return &code.Bytecode{
        Instructions: c.currentInstructions(),
        Constants:    c.constants,
    }
}
```

---

## 3. 심볼 테이블 (symbol_table.go)

### 3.1 심볼 테이블 구조체

```go
// 변수 스코프 상수
const (
    GlobalScope SymbolScope = iota
    LocalScope
    BuiltinScope
    FreeScope
    FunctionScope
)

// 심볼 구조체: 변수 정보 저장
type Symbol struct {
    Name  string       // 변수 이름
    Scope SymbolScope  // 변수 스코프
    Index int          // 변수 인덱스
}

// 심볼 테이블 구조체: 변수 추적
type SymbolTable struct {
    Outer *SymbolTable  // 외부 스코프 테이블
    
    store          map[string]Symbol  // 심볼 저장소
    numDefinitions int                // 정의된 변수 수
    
    FreeSymbols []Symbol  // 자유 변수 목록
}
```

### 3.2 심볼 정의 및 조회

```go
// 새 변수 정의
func (s *SymbolTable) Define(name string) Symbol {
    symbol := Symbol{
        Name:  name,
        Index: s.numDefinitions,
        Scope: LocalScope,
    }
    
    // 루트 테이블이면 전역 변수로 설정
    if s.Outer == nil {
        symbol.Scope = GlobalScope
    }
    
    s.store[name] = symbol
    s.numDefinitions++
    
    return symbol
}

// 변수 조회 (스코프 체인 탐색)
func (s *SymbolTable) Resolve(name string) (Symbol, bool) {
    // 현재 스코프에서 조회
    obj, ok := s.store[name]
    if ok {
        return obj, true
    }
    
    // 외부 스코프가 있으면 탐색
    if s.Outer != nil {
        obj, ok := s.Outer.Resolve(name)
        if ok {
            // 외부 변수면 자유 변수로 등록
            if obj.Scope == GlobalScope || obj.Scope == BuiltinScope {
                return obj, true
            }
            
            // 자유 변수 등록
            free := s.defineFree(obj)
            return free, true
        }
    }
    
    return Symbol{}, false
}

// 자유 변수 등록
func (s *SymbolTable) defineFree(original Symbol) Symbol {
    s.FreeSymbols = append(s.FreeSymbols, original)
    
    symbol := Symbol{
        Name:  original.Name,
        Index: len(s.FreeSymbols) - 1,
        Scope: FreeScope,
    }
    
    s.store[original.Name] = symbol
    return symbol
}
```

---

## 4. 가상 머신 (vm.go)

### 4.1 가상 머신 구조체

```go
const StackSize = 2048           // 스택 최대 크기
const GlobalsSize = 65536        // 전역 변수 저장소 크기
const MaxFrames = 1024           // 최대 호출 프레임 수

// 가상 머신 구조체
type VM struct {
    constants []object.Object    // 상수 풀
    
    stack []object.Object        // 값 스택
    sp    int                    // 스택 포인터
    
    globals []object.Object      // 전역 변수 저장소
    
    frames      []*Frame         // 호출 프레임 스택
    framesIndex int              // 현재 프레임 인덱스
}
```

### 4.2 가상 머신 실행

```go
// 바이트코드 실행
func (vm *VM) Run() error {
    // 현재 프레임 인덱스가 0보다 크거나 같을 때까지 루프
    for vm.currentFrame().ip < len(vm.currentFrame().Instructions())-1 {
        vm.currentFrame().ip++
        
        // 현재 명령어 및 연산 코드 가져오기
        ip := vm.currentFrame().ip
        ins := vm.currentFrame().Instructions()
        op := code.Opcode(ins[ip])
        
        // 연산 코드에 따른 처리
        switch op {
        case code.OpConstant:
            // 상수 로드
            constIndex := code.ReadUint16(ins[ip+1:])
            vm.currentFrame().ip += 2
            
            err := vm.push(vm.constants[constIndex])
            if err != nil {
                return err
            }
        
        case code.OpAdd, code.OpSub, code.OpMul, code.OpDiv:
            // 이항 연산
            err := vm.executeBinaryOperation(op)
            if err != nil {
                return err
            }
        
        case code.OpPop:
            // 스택에서 값 제거
            vm.pop()
        
        case code.OpSetGlobal:
            // 전역 변수 설정
            globalIndex := code.ReadUint16(ins[ip+1:])
            vm.currentFrame().ip += 2
            
            vm.globals[globalIndex] = vm.pop()
        
        case code.OpGetGlobal:
            // 전역 변수 로드
            globalIndex := code.ReadUint16(ins[ip+1:])
            vm.currentFrame().ip += 2
            
            err := vm.push(vm.globals[globalIndex])
            if err != nil {
                return err
            }
        
        case code.OpSetLocal:
            // 지역 변수 설정
            localIndex := code.ReadUint8(ins[ip+1:])
            vm.currentFrame().ip += 1
            
            frame := vm.currentFrame()
            vm.stack[frame.basePointer+int(localIndex)] = vm.pop()
        
        case code.OpGetLocal:
            // 지역 변수 로드
            localIndex := code.ReadUint8(ins[ip+1:])
            vm.currentFrame().ip += 1
            
            frame := vm.currentFrame()
            err := vm.push(vm.stack[frame.basePointer+int(localIndex)])
            if err != nil {
                return err
            }
        
        case code.OpJump:
            // 무조건 점프
            pos := code.ReadUint16(ins[ip+1:])
            vm.currentFrame().ip = int(pos) - 1
        
        case code.OpJumpNotTruthy:
            // 조건부 점프 (false면 점프)
            pos := code.ReadUint16(ins[ip+1:])
            vm.currentFrame().ip += 2
            
            condition := vm.pop()
            if !isTruthy(condition) {
                vm.currentFrame().ip = int(pos) - 1
            }
        
        case code.OpCall:
            // 함수 호출
            numArgs := int(code.ReadUint8(ins[ip+1:]))
            vm.currentFrame().ip += 1
            
            err := vm.executeCall(numArgs)
            if err != nil {
                return err
            }
        
        case code.OpReturnValue:
            // 값 반환
            returnValue := vm.pop()
            
            frame := vm.popFrame()
            vm.sp = frame.basePointer - 1
            
            err := vm.push(returnValue)
            if err != nil {
                return err
            }
        
        case code.OpReturn:
            // 값 없이 반환
            frame := vm.popFrame()
            vm.sp = frame.basePointer - 1
            
            err := vm.push(Null)
            if err != nil {
                return err
            }
        
        case code.OpClosure:
            // 클로저 생성
            constIndex := code.ReadUint16(ins[ip+1:])
            numFree := code.ReadUint8(ins[ip+3:])
            vm.currentFrame().ip += 3
            
            err := vm.pushClosure(int(constIndex), int(numFree))
            if err != nil {
                return err
            }
        
        case code.OpGetFree:
            // 자유 변수 로드
            freeIndex := code.ReadUint8(ins[ip+1:])
            vm.currentFrame().ip += 1
            
            currentClosure := vm.currentFrame().cl
            err := vm.push(currentClosure.Free[freeIndex])
            if err != nil {
                return err
            }
            
        // 기타 연산 코드 처리...
        }
    }
    
    return nil
}
```

### 4.3 스택 연산

```go
// 스택에 값 푸시
func (vm *VM) push(o object.Object) error {
    if vm.sp >= StackSize {
        return fmt.Errorf("stack overflow")
    }
    
    vm.stack[vm.sp] = o
    vm.sp++
    
    return nil
}

// 스택에서 값 팝
func (vm *VM) pop() object.Object {
    o := vm.stack[vm.sp-1]
    vm.sp--
    return o
}

// 이항 연산 실행
func (vm *VM) executeBinaryOperation(op code.Opcode) error {
    right := vm.pop()
    left := vm.pop()
    
    // 정수 연산
    if left.Type() == object.INTEGER_OBJ && right.Type() == object.INTEGER_OBJ {
        return vm.executeBinaryIntegerOperation(op, left, right)
    }
    
    // 문자열 연산 (+ 연산자만)
    if left.Type() == object.STRING_OBJ && right.Type() == object.STRING_OBJ {
        return vm.executeBinaryStringOperation(op, left, right)
    }
    
    return fmt.Errorf("unsupported types for binary operation: %s %s",
        left.Type(), right.Type())
}
```

---

## 5. 프레임 (frame.go)

### 5.1 프레임 구조체

```go
// 호출 프레임 구조체
type Frame struct {
    cl          *object.Closure  // 클로저
    ip          int              // 명령어 포인터
    basePointer int              // 기준 포인터 (지역 변수 시작 위치)
}

// 프레임 생성
func NewFrame(cl *object.Closure, basePointer int) *Frame {
    return &Frame{
        cl:          cl,
        ip:          -1,  // 실행 전이므로 -1로 초기화
        basePointer: basePointer,
    }
}

// 프레임의 명령어 접근
func (f *Frame) Instructions() code.Instructions {
    return f.cl.Fn.Instructions
}
```

---

## 6. 객체 시스템 (object.go)

### 6.1 주요 객체 타입

```go
// 객체 타입 상수
const (
    INTEGER_OBJ      = "INTEGER"
    BOOLEAN_OBJ      = "BOOLEAN"
    NULL_OBJ         = "NULL"
    STRING_OBJ       = "STRING"
    ARRAY_OBJ        = "ARRAY"
    HASH_OBJ         = "HASH"
    FUNCTION_OBJ     = "FUNCTION"
    COMPILED_FUNCTION_OBJ = "COMPILED_FUNCTION"
    CLOSURE_OBJ      = "CLOSURE"
    BUILTIN_OBJ      = "BUILTIN"
)

// 객체 인터페이스
type Object interface {
    Type() ObjectType     // 객체 타입 반환
    Inspect() string      // 객체 문자열 표현
}
```

### 6.2 컴파일된 함수와 클로저

```go
// 컴파일된 함수 객체
type CompiledFunction struct {
    Instructions  code.Instructions  // 함수 명령어
    NumLocals     int                // 지역 변수 개수
    NumParameters int                // 매개변수 개수
}

func (cf *CompiledFunction) Type() ObjectType { return COMPILED_FUNCTION_OBJ }
func (cf *CompiledFunction) Inspect() string {
    return fmt.Sprintf("CompiledFunction[%p]", cf)
}

// 클로저 객체
type Closure struct {
    Fn   *CompiledFunction    // 컴파일된 함수
    Free []Object             // 자유 변수 목록
}

func (c *Closure) Type() ObjectType { return CLOSURE_OBJ }
func (c *Closure) Inspect() string {
    return fmt.Sprintf("Closure[%p]", c)
}
```

---

## 7. REPL (repl.go)

### 7.1 REPL 구현

```go
const PROMPT = ">> "

// REPL 시작
func Start(in io.Reader, out io.Writer) {
    scanner := bufio.NewScanner(in)
    
    // 전역 변수와 상수 풀 초기화
    constants := []object.Object{}
    globals := make([]object.Object, vm.GlobalsSize)
    
    // 심볼 테이블 생성 및 내장 함수 설정
    symbolTable := compiler.NewSymbolTable()
    for i, v := range object.Builtins {
        symbolTable.DefineBuiltin(i, v.Name)
    }
    
    for {
        fmt.Fprintf(out, PROMPT)
        scanned := scanner.Scan()
        if !scanned {
            return
        }
        
        line := scanner.Text()
        
        // 어휘 분석
        l := lexer.New(line)
        // 구문 분석
        p := parser.New(l)
        
        // 프로그램 파싱
        program := p.ParseProgram()
        if len(p.Errors()) != 0 {
            printParserErrors(out, p.Errors())
            continue
        }
        
        // 컴파일
        comp := compiler.NewWithState(symbolTable, constants)
        err := comp.Compile(program)
        if err != nil {
            fmt.Fprintf(out, "Woops! Compilation failed:\n %s\n", err)
            continue
        }
        
        // 바이트코드 얻기
        code := comp.Bytecode()
        constants = code.Constants
        
        // 가상 머신 생성 및 실행
        machine := vm.NewWithGlobalsStore(code, globals)
        err = machine.Run()
        if err != nil {
            fmt.Fprintf(out, "Woops! Executing bytecode failed:\n %s\n", err)
            continue
        }
        
        // 실행 결과 출력
        lastPopped := machine.LastPoppedStackElem()
        io.WriteString(out, lastPopped.Inspect())
        io.WriteString(out, "\n")
    }
}
```

---

## C++ 포팅 고려사항

1. **메모리 관리**: Go의 가비지 컬렉션 대신 C++의 스마트 포인터 사용
2. **에러 처리**: Go의 에러 반환 패턴 대신 예외 또는 Result 타입 사용
3. **다형성**: Go의 인터페이스 대신 C++의 가상 함수와 추상 클래스 사용
4. **자료구조**: Go의 슬라이스와 맵 대신 C++의 vector와 unordered_map 사용
5. **코드 구성**: Go의 패키지 대신 C++의 네임스페이스와 클래스로 구조화 