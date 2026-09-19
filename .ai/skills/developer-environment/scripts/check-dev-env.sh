#!/usr/bin/env bash
set -u

# Read-only inspection for a token-efficient Codex + Claude Code environment.
# Does not install/start/stop/modify anything. Always exits 0 so agents can consume the full report.

have() { command -v "$1" >/dev/null 2>&1; }
section() { printf '\n[%s]\n' "$1"; }
kv() { printf '%-28s %s\n' "$1" "$2"; }
first_line() { "$@" 2>/dev/null | head -n 1 || true; }

section "Host"
kv "OS" "$(uname -s 2>/dev/null || echo unknown)"
kv "Architecture" "$(uname -m 2>/dev/null || echo unknown)"

section "Core tools"
for tool in git gh curl jq python3 uv uvx graphify sqlite3 pandoc pdftotext rg ast-grep; do
  if have "$tool"; then
    kv "$tool" "installed: $(command -v "$tool")"
  else
    kv "$tool" "missing/not on PATH"
  fi
done

section "Docker (optional)"
# Docker neni podminka niceho. Hlasi se jen jako informace - kdyz chybi, NENI to nalez
# a nic se nedoporucuje instalovat. Ma smysl teprve tehdy, kdyz projekt sam provozuje sluzbu.
if have docker; then
  if docker info >/dev/null 2>&1; then
    kv "docker" "available (optional, not required)"
  else
    kv "docker" "installed, daemon stopped (optional - fix only if a project service needs it)"
  fi
else
  kv "docker" "not installed (optional - no action needed)"
fi

section "Python / SQLite FTS5"
if have python3; then
  kv "python3" "$(first_line python3 --version)"
  if python3 - <<'PY' >/dev/null 2>&1
import sqlite3
c = sqlite3.connect(':memory:')
c.execute('create virtual table t using fts5(x)')
PY
  then
    kv "Python sqlite FTS5" "available"
  else
    kv "Python sqlite FTS5" "unavailable"
  fi
fi
have uv && kv "uv" "$(first_line uv --version)"
have uvx && kv "uvx" "$(first_line uvx --version)"

section "Project-local retrieval"
[ -x "tools/fts.sh" ] && kv "tools/fts.sh" "present/executable" || kv "tools/fts.sh" "absent/not executable"
[ -x "tools/extract-text.sh" ] && kv "tools/extract-text.sh" "present/executable" || kv "tools/extract-text.sh" "absent/not executable"
[ -f ".fts/fts.db" ] && kv ".fts/fts.db" "present ($(du -h .fts/fts.db 2>/dev/null | awk '{print $1}'))" || kv ".fts/fts.db" "absent (created by fts.sh on first index/query)"

# Nejcastejsi tichá past: index existuje, ale nezna jazyk projektu (jen .md).
# Porovna pripony v repu s priponami v indexu a vypise, co chybi.
if [ -f ".fts/fts.db" ]; then
  python3 - <<'PYEOF' 2>/dev/null || true
import sqlite3, subprocess, collections, os
try:
    files = subprocess.run(["git","ls-files"], capture_output=True, text=True, timeout=20).stdout.split()
except Exception:
    files = []
CODE = {".java",".py",".c",".h",".cpp",".hpp",".cc",".ino",".cs",".go",".rs",".swift",
        ".ts",".tsx",".js",".jsx",".vue",".svelte",".kt",".rb",".php",".scala",".dart",".lua",".sh"}
repo = collections.Counter(os.path.splitext(f)[1].lower() for f in files)
repo = {e: n for e, n in repo.items() if e in CODE and n >= 3}
if repo:
    db = sqlite3.connect(".fts/fts.db")
    idx = {e for e in repo if db.execute(
        "select 1 from files where path like ? limit 1", ("%" + e,)).fetchone()}
    missing = sorted(set(repo) - idx, key=lambda e: -repo[e])
    if missing:
        print("  %-28s %s" % ("Index language gap",
              "NOT indexed: " + ", ".join(f"{e} ({repo[e]} files)" for e in missing[:6])
              + "  -> widen EXTS in tools/fts.sh and reindex"))
    else:
        print("  %-28s %s" % ("Index language coverage", "covers all code extensions found in repo"))
PYEOF
fi
[ -d "files/_text" ] && kv "files/_text" "present" || kv "files/_text" "absent"

section "Code intelligence"
if have graphify; then
  kv "graphify" "installed/on PATH"
else
  kv "graphify" "no direct CLI on PATH"
fi

# Serena may be configured as an MCP without a standalone `serena` executable.
if have serena; then
  kv "serena" "installed/on PATH: $(command -v serena)"
else
  kv "serena" "no direct CLI on PATH; inspect MCP config before calling it missing"
fi

section "Project instructions"
[ -f "AGENTS.md" ] && kv "AGENTS.md" "present" || kv "AGENTS.md" "absent"
[ -f "CLAUDE.md" ] && kv "CLAUDE.md" "present" || kv "CLAUDE.md" "absent"
[ -d ".agents/skills" ] && kv ".agents/skills" "present" || kv ".agents/skills" "absent"
[ -d ".claude/skills" ] && kv ".claude/skills" "present" || kv ".claude/skills" "absent"

section "Known configs"
[ -f "$HOME/.codex/config.toml" ] && kv "Codex config" "$HOME/.codex/config.toml" || kv "Codex config" "not found"
[ -f "$HOME/.claude.json" ] && kv "Claude config" "$HOME/.claude.json" || true
[ -d "$HOME/.claude" ] && kv "Claude dir" "$HOME/.claude" || true

section "Policy"
kv "State model" "missing != stopped != configured-but-unreachable"
kv "Retrieval order" "Graphify/Serena/FTS -> targeted reads -> edit/test/diff"
kv "Install policy" "verify first; install only after confirming the actual gap"

exit 0
