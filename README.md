# Quantum Info
```
1. X Language 를 지원하는 컴파일러
2. 중간코드(.xByte)및 실행(Binary) 파일을 바로 실행시킬 수 있는 프로그램 집합
```

## 프로그램 종류
* Quantum : Compiler, Interpreter
* QuantumVM : VM

## Includes Info
VSCode에서 사용하고 있는 Includes Cache 정보 

    # System Includes
    "${workspaceFolder}/**",

    # ProgramManager Includes
    "${workspaceFolder}/Includes", 
    "${workspaceFolder}/Includes/**",

    # ProgramManager Includes
    "${workspaceFolder}/Modules/ProgramManager/Includes/", 
    "${workspaceFolder}/Modules/ProgramManager/Includes/**", 
    "${workspaceFolder}/Modules/ProgramManager/Module/Exception/Includes/Public", 
    "${workspaceFolder}/Modules/ProgramManager/Module/Exception/Includes/Public/**", 
    "${workspaceFolder}/Modules/ProgramManager/Module/Exception/Includes/Private/", 
    "${workspaceFolder}/Modules/ProgramManager/Module/Exception/Includes/Private/**", 
    "${workspaceFolder}/Modules/ProgramManager/Module/GarbageCollection/Includes/Public", 
    "${workspaceFolder}/Modules/ProgramManager/Module/GarbageCollection/Includes/Public/**", 
    "${workspaceFolder}/Modules/ProgramManager/Module/GarbageCollection/Includes/Private", 
    "${workspaceFolder}/Modules/ProgramManager/Module/GarbageCollection/Includes/Private/**", 
    "${workspaceFolder}/Modules/ProgramManager/Module/InputSystem/Includes/Public", 
    "${workspaceFolder}/Modules/ProgramManager/Module/InputSystem/Includes/Public/**", 
    "${workspaceFolder}/Modules/ProgramManager/Module/InputSystem/Includes/Private/", 
    "${workspaceFolder}/Modules/ProgramManager/Module/InputSystem/Includes/Private/**", 
    "${workspaceFolder}/Modules/ProgramManager/Module/ProcessEvent/Includes/Public/", 
    "${workspaceFolder}/Modules/ProgramManager/Module/ProcessEvent/Includes/Public/**", 
    "${workspaceFolder}/Modules/ProgramManager/Module/ProcessEvent/Includes/Private/", 
    "${workspaceFolder}/Modules/ProgramManager/Module/ProcessEvent/Includes/Private/**", 

    # String Includes
    "${workspaceFolder}/Modules/String/Includes/Public/", 
    "${workspaceFolder}/Modules/String/Includes/Public/**", 
    "${workspaceFolder}/Modules/String/Includes/Private/", 
    "${workspaceFolder}/Modules/String/Includes/Private/**" 

# TODO
1. String 오류 검수
2. 모든 소스코드에 라이센스 파일 및 엔티티 코드 추가
3. InputSystem 설계
4. Exception 설계
5. 헤더 파일 주석 추가


## InputSystem 설계

    stdin: 표준 입력에 관련된 함수들이 존재함.
    stdout: 표준 출력에 관련된 함수들이 존재함.
    stderr: 표준 오류에 관련된 함수들이 존재함.

    bufSize : 각 입출력 시스템의 버퍼 공간.

    Console : Console 기반의 입출력 기능을 담당하는 함수가 있음.
    NetWork : Network 기반의 입출력 기능을 담당하는 함수가 있음. <- 만들지 안 만들지는 모름. 여유가 있으면 개발
    Serial  : Serial Port 기반의 입출력 기능을 담당하는 함수가 있음. 

## Exception 설계

    CallException 함수를 통해 오류를 시스템에 보고
    (ErrCode Code) 
    CallAssert 함수를 통해 어디서 오류가 났는지 시스템에 보고
    (ErrCode code, const_chs File, const_chs FuncName, int Line)
 
    ExceptionAction 특정 오류가 나면 처리되는 함수를 처리함.

    ExceptionDefines 특정 오류가 무엇인지 정의함.
    (const_chs Name)
    ExceptionUnDefines 특정 오류 정의를 삭제함.
    (const_chs Name)

    Try_Catch(ExcuteFunc, Exception<err_name, CatchAction>)
    Try_Catch_Finnaly(ExcuteFunc, Exception<err_name, CatchAction>, FinallyFunc)


