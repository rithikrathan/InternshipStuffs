#!/bin/bash

SESSION="InternshipStuffs"

tmux kill-session -t $SESSION 2>/dev/null

# editor window (shell first, then start nvim)
tmux new-session -d -s $SESSION -n "editor" -c "/home/rathan/Desktop/projects/InternshipStuffs"
tmux send-keys -t $SESSION:editor "nvim" C-m

# shell window
tmux new-window -t $SESSION -n "shell" -c "/home/rathan/Desktop/projects/InternshipStuffs"

tmux select-window -t $SESSION:editor
tmux attach-session -t $SESSION
