#!/bin/bash

# Script to stop all game servers running in tmux sessions
# Usage: ./stop_servers.sh

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Stopping all game servers...${NC}"

# List of server sessions
SERVERS=("masterserver" "authserver" "queryserver" "charserver" "chatserver" "gameserver")

# Function to stop a server session
stop_server() {
    local server_name=$1
    
    if tmux has-session -t "$server_name" 2>/dev/null; then
        echo -e "${GREEN}Stopping $server_name...${NC}"
        # Send Ctrl+C to the session and wait a bit, then kill if still running
        tmux send-keys -t "$server_name" C-c
        sleep 2
        # Kill the session if it's still running
        if tmux has-session -t "$server_name" 2>/dev/null; then
            tmux kill-session -t "$server_name"
        fi
        echo -e "${GREEN}  ✓ $server_name stopped${NC}"
    else
        echo -e "${YELLOW}  - $server_name session not found (already stopped?)${NC}"
    fi
}

# Stop all servers
for server in "${SERVERS[@]}"; do
    stop_server "$server"
done

# Also kill any remaining tmux sessions that match our server names
echo -e "\n${YELLOW}Checking for any remaining sessions...${NC}"
for server in "${SERVERS[@]}"; do
    if tmux has-session -t "$server" 2>/dev/null; then
        echo -e "${RED}Force killing $server...${NC}"
        tmux kill-session -t "$server" 2>/dev/null
    fi
done

echo -e "\n${GREEN}=== All servers stopped ===${NC}"
echo -e "${YELLOW}Remaining tmux sessions:${NC}"
tmux ls 2>/dev/null || echo "  (none)"
