#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<EOF
Usage: $0 [OPTIONS] [TARGET_DIR]
Options:
  -n|--dry-run    show files that would be deleted (default)
  -f|--force      delete without interactive confirmation
  -h|--help       show this help
If TARGET_DIR omitted, uses current directory.
This deletes *all* .xml files under the target, no content check.
EOF
}

DRY_RUN=true
FORCE=false
TARGET="."

# parse args
while (( $# )); do
  case "$1" in
    -n|--dry-run) DRY_RUN=true; FORCE=false; shift ;;
    -f|--force)   FORCE=true;  DRY_RUN=false; shift ;;
    -h|--help)    usage; exit 0 ;;
    --)           shift; break ;;
    *)
      if [[ -d "$1" || -f "$1" ]]; then
        TARGET="$1"
        shift
      else
        echo "Unknown argument: $1" >&2
        usage
        exit 2
      fi
      ;;
  esac
done

# repo-root guard: ensure we're in Hack project root
# (adjust this if your repo has a different sentinel file)
if [[ ! -f "compiler/JackCompiler.cpp" ]]; then
  echo "[error] must run from Hack project root (compiler/JackCompiler.cpp not found)." >&2
  exit 2
fi

# find candidate xml files
mapfile -d '' candidates < <(find "$TARGET" -type f \( -name '*.xml' -o -name '*T.xml' \) -print0)

if [[ ${#candidates[@]} -eq 0 ]]; then
  echo "No XML files found under '$TARGET'."
  exit 0
fi

# ensure every candidate is inside repo (paranoid guard)
real_repo="$(pwd)"
declare -a ok_files=()

for f in "${candidates[@]}"; do
  real_f="$(realpath "$f")"
  case "$real_f" in
    "$real_repo"/*)
      ok_files+=("$f")
      ;;
    *)
      echo "[skip] outside repo (unexpected): $f"
      ;;
  esac
done

if [[ ${#ok_files[@]} -eq 0 ]]; then
  echo "No XML files inside repo (after filtering)."
  exit 0
fi

echo "XML files to delete under '$TARGET':"
for x in "${ok_files[@]}"; do
  printf '  %s\n' "$x"
done

if $DRY_RUN; then
  echo "Dry-run. No files deleted. Rerun with --force to delete."
  exit 0
fi

if ! $FORCE; then
  read -r -p "Delete ${#ok_files[@]} files? Type 'yes' to confirm: " ans
  if [[ "$ans" != "yes" ]]; then
    echo "Aborted."
    exit 0
  fi
fi

for x in "${ok_files[@]}"; do
  rm -v -- "$x"
done

echo "Deleted ${#ok_files[@]} files."
