#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}Starting servers...${NC}"

if ! command -v tmux &> /dev/null; then
    echo -e "${RED}tmux is not installed.${NC}"
    exit 1
fi

if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Build directory not found: $BUILD_DIR${NC}"
    exit 1
fi

start_server() {
    local NAME=$1
    local BIN=$2
    local CONF=$3

    if tmux has-session -t "$NAME" 2>/dev/null; then
        echo -e "${YELLOW}Session $NAME already exists. Skipping.${NC}"
        return
    fi

    if [ ! -f "$BUILD_DIR/$BIN" ]; then
        echo -e "${RED}Binary not found: $BUILD_DIR/$BIN${NC}"
        return
    fi

    echo -e "${GREEN}Starting $NAME...${NC}"

    tmux new -d -s "$NAME" \
    "cd '$SCRIPT_DIR' && ./build/$BIN config/$CONF"

    sleep 0.3

    if tmux has-session -t "$NAME" 2>/dev/null; then
        echo -e "${GREEN}  ✓ $NAME started${NC}"
    else
        echo -e "${RED}  ✗ Failed to start $NAME${NC}"
    fi
}

echo -e "\n${GREEN}=== Launch Order ===${NC}"

start_server "master" "MasterServer" "MasterServer.ini"
sleep 2

start_server "auth"   "AuthServer"   "AuthServer.ini"
start_server "query"  "QueryServer"  "QueryServer.ini"
#start_server "char"   "CharServer"   "CharServer.ini"
#start_server "chat"   "ChatServer"   "ChatServer.ini"
start_server "game"   "GameServer"   "GameServer.ini"

echo -e "\n${GREEN}Done.${NC}"
echo -e "List sessions: tmux ls"
echo -e "Attach: tmux attach -t <name>"
