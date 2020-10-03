#!/bin/bash

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$SCRIPTS_DIR/.."

mkdir -p run

rm -rf run/assets
cp -rf OpenGL-Sandbox/assets run

cd run

../bin/Debug-linux-x86_64/OpenGL-Sandbox/OpenGL-Sandbox