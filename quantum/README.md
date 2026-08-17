## 프로그램 종류
* Quantum : Compiler, Interpreter
* QuantumVM : VM

<br>
<br>
<br>

# TODO
1. JSON 2022-02-07까지 완성
2. 기능 테스트 2022-02-08까지 완수
3. GC 성능 테스트 2022-02-09까지 완수

<br>
<br>
<br>

# .vscode 파일 구성
현재 사용중인 .vscode 파일 구성
<br>
<br>

## c_cpp_properties.json
    {
        "configurations": [
            {
                "name": "Linux",
                "includePath": [
                    "${workspaceFolder}/**",
                    "${workspaceFolder}/Includes/",
                    "${workspaceFolder}/Includes/**",
                    "${workspaceFolder}/Modules/Application/Includes/Private/",
                    "${workspaceFolder}/Modules/Application/Includes/Private/**",
                    "${workspaceFolder}/Modules/Application/Includes/Public/",
                    "${workspaceFolder}/Modules/Application/Includes/Public/**",
                    "${workspaceFolder}/Modules/String/Includes/Private/",
                    "${workspaceFolder}/Modules/String/Includes/Private/**",
                    "${workspaceFolder}/Modules/String/Includes/Public/",
                    "${workspaceFolder}/Modules/String/Includes/Public/**",
                    "${workspaceFolder}/Modules/JSON/Includes/Public/",
                    "${workspaceFolder}/Modules/JSON/Includes/Public/**",
                    "${workspaceFolder}/Modules/JSON/Includes/Private",
                    "${workspaceFolder}/Modules/JSON/Includes/Private/**"
                ],
                "defines": [],
                "compilerPath": "/usr/bin/clang",
                "cStandard": "c11",
                "cppStandard": "c++14",
                "configurationProvider": "ms-vscode.cmake-tools"
            }
        ],
        "version": 4
    }

## launch.json
    {
        // Use IntelliSense to learn about possible attributes.
        // Hover to view descriptions of existing attributes.
        // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
        "version": "0.2.0",
        "configurations": [
            {
            "type": "lldb",
            "request": "launch",
            "name": "Debug",
            "program": "${workspaceFolder}/build/QuantumCompiler",
            "args": [],
            "cwd": "${workspaceFolder}/build/"
            }
        ]
    }

## settings.json
    {
        "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    }

## tasks.json
    {
        // See https://go.microzsoft.com/fwlink/?LinkId=733558
        // for the documentation about the tasks.json format

        "version": "2.0.0",
        "tasks": [
            {
                "label": "Valgrind",
                "type": "shell",
                "command": "valgrind",
                "args": [
                    "--leak-check=full",
                    "--show-leak-kinds=all",
                    "-s",
                    "${workspaceFolder}/build/QuantumCompiler"
                ],
                "problemMatcher": [],
                "group": {
                    "kind": "build",
                    "isDefault": true
                },
        "options": {
            "cwd": "${workspaceFolder}/build/"
        }
            }
        ]
    }

<br>
<br>
<br>

# 계획
1. 2월달 안으로 파서 끝
2. 3월 중순 안으로 AST 개발 끝
3. 3월 말 안으로 중간코드 생성기 개발 끝
4. 4월 초 최적화기 개발
5. 5월 초 전체 프로젝트 최적화
6. 5월 말 목적 프로그램 생성기 개발
7. 6월 라이브러리 개발