#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
if [ -x .venv/bin/python ]; then
    exec .venv/bin/python -m desktop.app
fi
exec python3 -m desktop.app
