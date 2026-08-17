#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(git -C "$script_dir" rev-parse --show-toplevel)"
config="$script_dir/copy.bara.sky"

usage() {
  command_name="$(basename -- "$0")"
  echo "Usage: $command_name [--dry-run] [--force] <validate|arduino-esp32|esp-idf|all>"
  echo
  echo "COPYBARA_BIN may name a Copybara executable. Alternatively, set COPYBARA_JAR"
  echo "to a Copybara deploy jar and the script will invoke it with java -jar."
}

copybara_command=()
if [[ -n "${COPYBARA_JAR:-}" ]]; then
  copybara_command=(java -jar "$COPYBARA_JAR")
else
  copybara_command=("${COPYBARA_BIN:-copybara}")
fi

copybara_flags=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run)
      copybara_flags+=(--dry-run)
      shift
      ;;
    --force)
      copybara_flags+=(--force)
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      break
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
    *)
      break
      ;;
  esac
done

action="${1:-}"
if [[ -z "$action" || $# -ne 1 ]]; then
  usage >&2
  exit 2
fi

if [[ "$action" == "validate" ]]; then
  exec "${copybara_command[@]}" validate "$config"
fi

case "$action" in
  arduino-esp32)
    workflows=(arduino_esp32)
    ;;
  esp-idf)
    workflows=(esp_idf)
    ;;
  all)
    workflows=(esp_idf arduino_esp32)
    ;;
  *)
    echo "Unknown action: $action" >&2
    usage >&2
    exit 2
    ;;
esac

if [[ -n "$(git -C "$repo_root" status --porcelain)" ]]; then
  echo "The destination worktree must be clean before Copybara runs." >&2
  exit 1
fi

branch="$(git -C "$repo_root" symbolic-ref --quiet --short HEAD || true)"
if [[ -z "$branch" ]]; then
  echo "Copybara imports require a checked-out destination branch." >&2
  exit 1
fi

for workflow in "${workflows[@]}"; do
  "${copybara_command[@]}" migrate \
    "$config" \
    "$workflow" \
    --git-destination-path="$repo_root" \
    --git-destination-fetch="$branch" \
    --git-destination-push="$branch" \
    "${copybara_flags[@]}"
done
