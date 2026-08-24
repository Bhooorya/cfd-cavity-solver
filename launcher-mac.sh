#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-8000}"
URL="http://127.0.0.1:${PORT}/project-webpage.html"

echo "Opening CFD app on ${URL}"
open "${URL}"

cd "${PROJECT_DIR}"

if command -v python3 >/dev/null 2>&1; then
  python3 -m http.server "${PORT}"
  exit 0
fi

if command -v python >/dev/null 2>&1; then
  python -m http.server "${PORT}"
  exit 0
fi

echo
echo "[ERROR] Python was not found on this machine."
echo "Install Python from https://www.python.org/downloads/ and run this file again."
exit 1
