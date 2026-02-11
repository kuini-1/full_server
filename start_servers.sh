#!/bin/bash

# Script to start all game servers in tmux sessions
# Usage: ./start_servers.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
CONFIG_DIR="${SCRIPT_DIR}/config"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting all game servers in tmux sessions...${NC}"

# Check if tmux is installed
if ! command -v tmux &> /dev/null; then
    echo -e "${RED}Error: tmux is not installed. Please install it first.${NC}"
    exit 1
fi

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found: $BUILD_DIR${NC}"
    exit 1
fi

# Function to start a server in a tmux session
start_server() {
    local server_name=$1
    local server_binary=$2
    local config_file=$3
    
    # Check if session already exists
    if tmux has-session -t "$server_name" 2>/dev/null; then
        echo -e "${YELLOW}Session '$server_name' already exists. Skipping...${NC}"
        return
    fi
    
    # Check if binary exists
    if [ ! -f "$BUILD_DIR/$server_binary" ]; then
        echo -e "${RED}Error: Binary not found: $BUILD_DIR/$server_binary${NC}"
        return
    fi
    
    # Start server in new tmux session
    echo -e "${GREEN}Starting $server_name...${NC}"
    tmux new-session -d -s "$server_name" -c "$BUILD_DIR" "$BUILD_DIR/$server_binary" "$CONFIG_DIR/$config_file"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}  ✓ $server_name started in tmux session '$server_name'${NC}"
    else
        echo -e "${RED}  ✗ Failed to start $server_name${NC}"
    fi
}

# Start servers in order
echo -e "\n${GREEN}=== Starting Servers ===${NC}"

# Start MasterServer first (other servers connect to it)
start_server "masterserver" "MasterServer" "MasterServer.ini"

# Small delay to let MasterServer start
sleep 2

# Start AuthServer (connects to MasterServer)
start_server "authserver" "AuthServer" "AuthServer.ini"

# Start QueryServer
start_server "queryserver" "QueryServer" "QueryServer.ini"

# Start CharServer
start_server "charserver" "CharServer" "CharServer.ini"

# Start ChatServer
start_server "chatserver" "ChatServer" "ChatServer.ini"

# Start GameServer (can have multiple instances, but we'll start one by default)
start_server "gameserver" "GameServer" "GameServer.ini"

echo -e "\n${GREEN}=== All servers started ===${NC}"
echo -e "\n${YELLOW}Useful commands:${NC}"
echo -e "  View all sessions: ${GREEN}tmux ls${NC}"
echo -e "  Attach to a session: ${GREEN}tmux attach -t <session_name>${NC}"
echo -e "  Detach from session: ${GREEN}Ctrl+B, then D${NC}"
echo -e "  Stop all servers: ${GREEN}./stop_servers.sh${NC}"
echo -e "\n${YELLOW}To view logs, attach to a session:${NC}"
echo -e "  ${GREEN}tmux attach -t authserver${NC}"
