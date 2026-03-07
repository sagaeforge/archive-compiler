#!/usr/bin/env bash
# doc-check.sh — Detect stale documentation in Kern project
# Run: bash scripts/doc-check.sh
# Returns non-zero if errors detected. Warnings are informational.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ERRORS=0
WARNS=0

red()    { printf '\033[31m%s\033[0m\n' "$*"; }
yellow() { printf '\033[33m%s\033[0m\n' "$*"; }
green()  { printf '\033[32m%s\033[0m\n' "$*"; }

error() { red "  ERROR: $*"; ERRORS=$((ERRORS + 1)); }
warn()  { yellow "  WARN:  $*"; WARNS=$((WARNS + 1)); }

# Active docs to check (exclude: whitepaper=historical, REQUIREMENTS.md=protected)
ACTIVE_DOCS=()
[[ -f "$ROOT/CLAUDE.md" ]] && ACTIVE_DOCS+=("$ROOT/CLAUDE.md")
for f in "$ROOT"/.claude/rules/*.md; do [[ -f "$f" ]] && ACTIVE_DOCS+=("$f"); done
for f in "$ROOT"/docs/KERN_LANGUAGE_GUIDE.md "$ROOT"/docs/SYNTAX.md; do
    [[ -f "$f" ]] && ACTIVE_DOCS+=("$f")
done

# ── 1. v1 ghost references ──────────────────────────────────────────
echo "=== 1. v1 ghost references ==="

V1_PATHS=("include/kern/sema" "include/kern/ir/" "include/kern/codegen" "lib/Sema/" "lib/IR/" "lib/CodeGen/")
for pat in "${V1_PATHS[@]}"; do
    for f in "${ACTIVE_DOCS[@]}"; do
        if grep -qF "$pat" "$f" 2>/dev/null; then
            error "$(basename "$f") references deleted v1 path '$pat'"
        fi
    done
done

# v1 pipeline stages (word-boundary to avoid HIRBuilder matching IRBuilder)
for f in "${ACTIVE_DOCS[@]}"; do
    if grep -qP '\bTypeChecker\b|\bPurityChecker\b|\b(?<!HIR)IRBuilder\b|\b(?<!Instruction)CodeGen\b' "$f" 2>/dev/null; then
        error "$(basename "$f") references v1 pipeline stages"
    fi
done

# ── 2. Hardcoded test counts ────────────────────────────────────────
echo "=== 2. Hardcoded test counts ==="

for f in "${ACTIVE_DOCS[@]}"; do
    if grep -qP '\b\d{2,4}\s+(unit|E2E)\b' "$f" 2>/dev/null; then
        warn "$(basename "$f") contains hardcoded test counts"
    fi
done

# ── 3. Directory structure vs reality ────────────────────────────────
echo "=== 3. Directory structure ==="

actual_dirs=$(ls -d "$ROOT"/include/kern/*/ 2>/dev/null | \
    xargs -I{} basename {} | sort)

# Check CLAUDE.md lists all actual dirs
if [[ -f "$ROOT/CLAUDE.md" ]]; then
    for d in $actual_dirs; do
        if ! grep -qF "$d" "$ROOT/CLAUDE.md" 2>/dev/null; then
            warn "CLAUDE.md doesn't mention include/kern/$d/"
        fi
    done
fi

# ── 4. Referenced rule files exist ───────────────────────────────────
echo "=== 4. File references ==="

if [[ -f "$ROOT/CLAUDE.md" ]]; then
    for ref in $(grep -oP '@\.claude/rules/\S+\.md' "$ROOT/CLAUDE.md" 2>/dev/null); do
        path="$ROOT/${ref#@}"
        if [[ ! -f "$path" ]]; then
            error "CLAUDE.md references $ref but file doesn't exist"
        fi
    done
fi

# ── 5. Layer boundaries vs actual libs ───────────────────────────────
echo "=== 5. Layer boundaries ==="

if [[ -f "$ROOT/.claude/rules/layer-boundaries.md" ]]; then
    actual_libs=$(ls -d "$ROOT"/lib/*/ 2>/dev/null | xargs -I{} basename {} | sort)
    for lib in $actual_libs; do
        if ! grep -qi "$lib" "$ROOT/.claude/rules/layer-boundaries.md" 2>/dev/null; then
            warn "layer-boundaries.md missing layer: $lib"
        fi
    done
fi

# ── 6. Deleted files still referenced ────────────────────────────────
echo "=== 6. Deleted file references ==="

DELETED_BASENAMES=(
    "ARCHITECTURE.md" "WORKFLOW_M1.md" "AGENTIC_REQUIREMENTS.md"
    "ralph-loop-handoff.md" "structural-refactoring-requirements.md"
    "m5-requirements.md"
)

for bname in "${DELETED_BASENAMES[@]}"; do
    for f in "${ACTIVE_DOCS[@]}"; do
        if grep -qF "$bname" "$f" 2>/dev/null; then
            error "$(basename "$f") references deleted file '$bname'"
        fi
    done
done

# ── Summary ──────────────────────────────────────────────────────────
echo ""
echo "=== Summary ==="
if [[ $ERRORS -eq 0 ]] && [[ $WARNS -eq 0 ]]; then
    green "All checks passed."
    exit 0
elif [[ $ERRORS -eq 0 ]]; then
    yellow "$WARNS warning(s), 0 errors."
    exit 0
else
    red "$ERRORS error(s), $WARNS warning(s)."
    exit 1
fi
