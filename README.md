# archive-compiler

2021년부터 이어온 **컴파일러·언어 설계** 작업 아카이브입니다.
안양공고 시절의 AnyangTechInterpreter 에서 시작해 Quantum(자작 언어 + VM), NCX, nCompiler 를 거쳐
nugdev-compiler 로 이어집니다.

## 계보

```
2021.06  AnyangTechInterpreter    "Quantum Language" — 첫 인터프리터  (54 커밋)
2021.12  Quantum          C 기반 컴파일러 + VM + GC  (103 커밋, 최대 규모)
2022.02      ↓ C++ 전환
2022.03  NCX              프론트엔드 교체형 컴파일러 확장
2022.02  nCompiler        컴파일러 재작성 시도
2022.05  MakeInterpreterOnCSharp   C#으로 인터프리터 만들기
    ⋮
2024.03  nugdev-compiler-2024   FSM 토크나이저 / 타입 파싱  (66 커밋)
2025.05  nugdev-compiler-2025   Tokenizer + AST + gtest    ← 가장 진전된 코드
2025.11  cos                    NASM 백엔드
    ⋮
2026.03  kern-research          순수 함수로 커널을 쓰는 언어 + kernc  ← 현역, 별도 리포
```

> `kern-research` 는 아카이브가 아니라 진행 중인 작업이라
> [`sagaeforge/kern-research`](https://github.com/sagaeforge/kern-research) 로 따로 두었습니다.

## 구성

| 디렉터리 | 내용 | 커밋 | 기간 |
|---|---|---:|---|
| [`anyangtech-interpreter/`](anyangtech-interpreter) | "Quantum Language" — 첫 인터프리터. 『만들면서 배우는 인터프리터』 참고 | 54 | 2021.06 |
| [`quantum/`](quantum) | 자작 언어 Quantum — 컴파일러 · VM · GC | 103 | 2021.12–2026.08 |
| [`ncx/`](ncx) | Nugunga Compiler eXtensions — 프론트엔드 교체형 | 14 | 2022.03 |
| [`ncompiler/`](ncompiler) | 컴파일러 재작성 시도 | 11 | 2022.02–2026.08 |
| [`make-interpreter-csharp/`](make-interpreter-csharp) | C#으로 인터프리터 구현 | 3 | 2022.05–2022.06 |
| [`nugdev-compiler-2024/`](nugdev-compiler-2024) | FSM 토크나이저 / 타입 파싱 (C++, vcpkg) | 66 | 2024.03–2025.03 |
| [`nugdev-compiler-2025/`](nugdev-compiler-2025) | Tokenizer + AST + gtest (C++) — 최신 | 9 | 2025.05–2026.08 |
| [`nugdev-compiler-init/`](nugdev-compiler-init) | 프로젝트 초기 셋업본 | 1 | 2025.05 |
| [`cos/`](cos) | NASM 백엔드 컴파일러 | 4 | 2025.11–2026.08 |
| [`nugdev-language/`](nugdev-language) | nugdev 언어 EBNF 설계 + 뷰어 | 1 | 2025.06 |
| [`docs/language-design/`](docs/language-design) | 언어 설계 노트 | 1 | 2025.01 |

## 서브모듈

`nugdev-compiler-2024`, `nugdev-compiler-2025` 는 vcpkg 를 서브모듈로 씁니다.

```bash
git submodule update --init --recursive
```

## 히스토리 보존 방식

각 프로젝트는 원본 저장소의 커밋 히스토리를 그대로 유지한 채 `git subtree` 방식으로 편입했습니다.
원본의 모든 브랜치 끝점은 `legacy/<프로젝트>/<브랜치>` 태그로 남아 있습니다.

```bash
git tag -l 'legacy/*'                      # 보존된 원본 브랜치 목록
git log  legacy/<프로젝트>/<브랜치>          # 그 프로젝트의 원본 커밋 히스토리
git show legacy/<프로젝트>/<브랜치>          # 편입 직전 최종 상태
```

> ⚠️ `git log -- <디렉터리>/` 로는 편입 커밋 1개만 보입니다.
> 서브트리 편입에 `-s ours` 병합을 써서 main 쪽 경로 히스토리에는 병합 커밋만 남기 때문입니다.
> **원본 커밋은 전부 이 저장소 안에 그대로 있고, 위의 `legacy/*` 태그로 접근합니다.**

보존된 태그 18개:

```
  legacy/anyangtech-interpreter/main
  legacy/cos/master
  legacy/cos/wip/archive-import
  legacy/make-interpreter-csharp/main
  legacy/ncompiler/main
  legacy/ncompiler/wip/archive-import
  legacy/ncx/main
  legacy/nugdev-compiler-2024/develop
  legacy/nugdev-compiler-2024/tag/snapshot/2025-06-21/develop
  legacy/nugdev-compiler-2025/develop
  legacy/nugdev-compiler-2025/wip/archive-import
  legacy/nugdev-compiler-init/develop
  legacy/quantum/main
  legacy/quantum/tag/snapshot/desktop/CleanCoding
  legacy/quantum/tag/snapshot/desktop/main
  legacy/quantum/tag/snapshot/laptop/CleanCoding
  legacy/quantum/tag/snapshot/laptop/main
  legacy/quantum/wip/archive-import
```

---

*이 저장소는 아카이브입니다. 유지보수하지 않습니다.*
