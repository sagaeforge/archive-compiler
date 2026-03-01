# Kern 에이전틱 개발 환경 — 요구사항 명세서

> 작성일: 2026-03-01
> 목적: Kern 컴파일러 프로젝트를 AI 에이전트 기반 개발에 최적화

---

## 1. 사용자 목표

- **역할 분리**: 사용자는 계획(planning)만 담당, 소스 코드 구현은 에이전트에게 위임
- **에이전트 자율성**: 구현 중에는 에이전트가 독립적으로 판단하고 진행
- **안전장치**: 빌드 + 테스트 통과를 기본 가드레일로 적용
- **첫 에이전트 작업**: M3 (부동소수점 + 타입 추론) 구현을 Feature Dev 방식으로 진행

---

## 2. 기능 요구사항

### 2.1. CLAUDE.md 생성 (프로젝트 루트)

에이전트가 세션 시작 시 자동으로 읽는 프로젝트 규칙 파일.

**필수 내용:**
- [ ] 프로젝트 개요 (Kern 컴파일러, C++20, x86-64 macOS)
- [ ] 파이프라인 구조 (Lexer → Parser → TypeChecker → PurityChecker → IRBuilder → CodeGen → NASM → ld)
- [ ] 디렉토리 구조 (`include/`, `lib/`, `tests/`, `tools/`)
- [ ] 빌드 명령어 (`cmake -B build && cmake --build build`)
- [ ] 테스트 실행 (`build/tests/unit/kern_tests` + `bash tests/integration/run_tests.sh`)
- [ ] 핵심 아키텍처 결정 (MEMORY.md의 패턴 그대로)
  - string_view 수명: 소스 텍스트가 모든 소비자보다 오래 살아야 함
  - IR: SSA + block arguments (phi 노드 없음)
  - Merge block params: `[result_val, then_val, else_val, then_block, else_block]`
  - ABI: System V AMD64
  - 머신: arm64 → x86-64 바이너리는 Rosetta 2로 실행
- [ ] 코딩 규칙 (기존 패턴 유지)
  - C++20 표준, `-Wall -Wextra -Wpedantic -Werror`
  - Arena 할당 패턴 (AST/IR 노드)
  - TypeChecker의 `expr_types_` 메모이제이션 패턴
  - PurityChecker의 Kahn 알고리즘 + DFS 패턴
- [ ] 에이전트 작업 범위: `docs/`, `README.md` 제외 전체 수정 가능
- [ ] 금지 사항: 문서 파일 수정, 기존 테스트 삭제, ABI 변경
- [ ] 커밋 규칙: `feat:`, `fix:`, `test:`, `refactor:` 접두사

### 2.2. 플러그인 설치

| 플러그인 | 용도 | 설치 우선순위 |
|---------|------|-------------|
| `ralph-loop` | TDD 자율 반복 루프 | P0 (필수) |
| `feature-dev` | 7단계 구조화 워크플로우 | P0 (필수) |
| `claude-md-management` | 세션 후 CLAUDE.md 자동 갱신 | P0 (필수) |
| `hookify` | 자연어로 가드레일 Hook 생성 | P1 (권장) |
| `pr-review-toolkit` | 에이전트 코드 자동 리뷰 | P1 (권장) |
| `commit-commands` | `/commit`, `/commit-push-pr` | P1 (권장) |

### 2.3. 가드레일 (빌드 + 테스트)

에이전트가 코드를 수정한 후 반드시 통과해야 하는 체크:

- [ ] **빌드 체크**: `cmake --build build` 성공
- [ ] **Unit 테스트**: `build/tests/unit/kern_tests` 전체 통과
- [ ] **Integration 테스트**: `bash tests/integration/run_tests.sh build/tools/kernc/kernc tests/integration` 전체 통과

**구현 방식 (택 1):**
- hookify 플러그인으로 자연어 정의
- 또는 `.claude/settings.local.json`에 직접 Hook 등록

### 2.4. .clang-format 추가

에이전트 출력 코드의 일관성 보장:

- [ ] LLVM 스타일 기반 (프로젝트 기존 코드와 호환)
- [ ] 들여쓰기: 4 spaces
- [ ] 칼럼 제한: 100
- [ ] `#include` 정렬 활성화

### 2.5. .gitignore 보완

- [ ] `build-cov/` 추가 (현재 untracked으로 노출됨)
- [ ] `.claude/ralph-loop.local.md` 추가 (ralph-loop 상태 파일)

---

## 3. 비기능 요구사항

### 3.1. 에이전트 워크플로우

```
[사용자]                          [에이전트]
   │                                │
   ├─ 계획 작성 (Plan/Design)       │
   │   └─ PROMPT.md 또는 /feature-dev 프롬프트 작성
   │                                │
   ├─ 에이전트 실행 ───────────────→ │
   │                                ├─ 탐색 (Explore)
   │                                ├─ 설계 (Design)
   │   ←── 설계 승인 요청 ──────────┤
   ├─ 승인/수정 ──────────────────→ │
   │                                ├─ 구현 (Implement)
   │                                ├─ 빌드 + 테스트 (Guard Rail)
   │                                ├─ 반복 (Ralph Loop)
   │                                ├─ 완료 시 커밋
   │   ←── 결과 보고 ──────────────┤
   ├─ 리뷰 (pr-review-toolkit)     │
   └─ 다음 작업 계획               │
```

### 3.2. 세션 간 연속성

- CLAUDE.md: 에이전트의 프로젝트 규칙 (정적)
- MEMORY.md: 축적된 패턴/인사이트 (동적, 자동 갱신)
- `claude-md-management` 플러그인: 세션 종료 시 학습 내용 반영

### 3.3. 첫 에이전트 작업 (M3)

환경 구축 후 첫 실전 테스트로 M3 구현 예정:
- f32/f64 타입 추가
- Context-based 타입 추론
- `checkExpr(Expr*, optional<Type> expected)` 패턴
- Feature Dev 워크플로우로 진행 (설계 단계에서 사용자 승인)

---

## 4. 수용 기준 (Acceptance Criteria)

### 환경 구축 완료 조건:
1. `CLAUDE.md`가 프로젝트 루트에 존재하고, 에이전트가 세션 시작 시 규칙을 인식
2. `ralph-loop`, `feature-dev`, `claude-md-management` 플러그인이 설치되어 동작
3. `.clang-format`이 존재하고 기존 코드와 호환
4. 가드레일이 작동하여 빌드/테스트 실패 시 에이전트가 수정 반복
5. `.gitignore`에 `build-cov/` 포함
6. 기존 241 unit + 36 E2E 테스트 모두 통과 유지

### 검증 시나리오:
```bash
# 1. CLAUDE.md 존재 확인
cat CLAUDE.md

# 2. 플러그인 동작 확인
# /ralph-loop "Fix the dead Neg/Not opcode in IRBuilder" --max-iterations 5 --completion-promise "DONE"

# 3. 가드레일 확인 (빌드+테스트)
cmake --build build && build/tests/unit/kern_tests

# 4. Feature Dev 워크플로우 확인
# /feature-dev "Implement f32/f64 floating point types for M3"
```

---

## 5. 미결 사항 (Open Questions)

1. **hookify vs 수동 Hook**: 가드레일을 hookify로 자연어 정의할지, 직접 `hooks.json` 작성할지 → 구현 시 결정
2. **Ralph Loop iteration 상한**: M3 같은 대규모 작업의 기본 `--max-iterations` 값 → 사용 경험 후 조정
3. **PR 워크플로우**: 에이전트 커밋을 별도 브랜치 → PR → 리뷰 방식으로 할지, develop에 직접 커밋할지 → 사용자 선호에 따라 결정
4. **commit-commands 필요성**: ralph-loop 종료 후 자동 커밋을 원하는지, 수동 리뷰 후 커밋할지 → 사용 경험 후 결정

---

## 6. 다음 단계

이 요구사항이 승인되면:
1. `/sc:implement` 또는 `/sc:workflow`로 구현 워크플로우 생성
2. CLAUDE.md 작성 → 플러그인 설치 → .clang-format 추가 → 가드레일 설정
3. M3 계획 수립 후 첫 에이전트 작업 실행

---

*Sources:*
- [SuperClaude Framework](https://github.com/SuperClaude-Org/SuperClaude_Framework)
- [ralph-loop plugin](https://github.com/anthropics/claude-plugins-official/tree/main/plugins/ralph-loop)
- [feature-dev plugin](https://github.com/anthropics/claude-plugins-official/tree/main/plugins/feature-dev)
- [Claude Code Plugins](https://github.com/anthropics/claude-plugins-official)
