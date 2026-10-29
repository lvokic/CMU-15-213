# Source this file from the project root: source ./env.sh
if [ -x "$PWD/.local-csapp/bin/gcc32" ]; then
    export CSAPP_GCC32="$PWD/.local-csapp/bin/gcc32"
else
    unset CSAPP_GCC32
fi
