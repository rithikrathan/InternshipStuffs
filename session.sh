#!/bin/bash

SESSION="InternshipStuffs"
ROOT_DIR="$(pwd)"

# dirs to ignore
IGNORE_DIRS=(".git")
PATTERN=$(IFS="|"; echo "${IGNORE_DIRS[*]}")

TARGET_DIR=$(find "$ROOT_DIR" -mindepth 1 -maxdepth 2 -type d \
    | grep -Ev "/($PATTERN)(/|$)" \
    | fzf)

[ -z "$TARGET_DIR" ] && exit 1

tmux kill-session -t $SESSION 2>/dev/null

# editor window
tmux new-session -d -s $SESSION -n "editor" -c "$TARGET_DIR"
tmux send-keys -t $SESSION:editor "nvim" C-m

# shell window
tmux new-window -t $SESSION -n "shell" -c "$TARGET_DIR"
tmux new-window -t $SESSION -n "serialMonitor" -c "$TARGET_DIR"
tmux send-keys -t $SESSION:serialMonitor "make monitor" C-m

# vibecode tool window
# tmux new-window -t $SESSION -n "opencode" -c "$TARGET_DIR"
# tmux send-keys -t $SESSION:opencode "opencode" C-m

tmux select-window -t $SESSION:editor
tmux attach-session -t $SESSION

