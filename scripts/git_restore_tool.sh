#!/bin/bash

# Alternative script to view git history and restore from a specific commit
# This lets you choose which version to restore

echo "============================================"
echo "Git History Browser for SerialInterface.cpp"
echo "============================================"
echo ""

# Navigate to the project directory
cd /Users/timrosener/Documents/Arduino/SkullStepperV4

# Check if we're in a git repository
if [ ! -d .git ]; then
    echo "Error: Not in a git repository!"
    exit 1
fi

# Show recent commits that modified SerialInterface.cpp
echo "Recent commits affecting SerialInterface.cpp:"
echo "============================================"
git log --oneline -10 -- SerialInterface.cpp
echo ""

# Option 1: Restore from last commit
echo "Options:"
echo "1) Restore from last commit (discard all changes)"
echo "2) View a specific commit's version"
echo "3) Restore from a specific commit"
echo "4) Show current changes (diff)"
echo "5) Cancel"
echo ""
echo "Enter your choice (1-5):"
read -r choice

case $choice in
    1)
        echo "Restoring from last commit..."
        git checkout HEAD -- SerialInterface.cpp
        echo "✓ Restored from last commit"
        ;;
    2)
        echo "Enter commit hash (or HEAD~n):"
        read -r commit
        echo ""
        echo "Showing SerialInterface.cpp from commit $commit:"
        echo "============================================"
        git show $commit:SerialInterface.cpp | less
        ;;
    3)
        echo "Enter commit hash to restore from:"
        read -r commit
        echo "This will restore SerialInterface.cpp from commit $commit"
        echo "Continue? (y/n)"
        read -r confirm
        if [[ "$confirm" =~ ^([yY][eE][sS]|[yY])$ ]]; then
            git checkout $commit -- SerialInterface.cpp
            echo "✓ Restored from commit $commit"
        else
            echo "Cancelled"
        fi
        ;;
    4)
        echo "Current changes to SerialInterface.cpp:"
        echo "======================================"
        git diff SerialInterface.cpp | less
        ;;
    5)
        echo "Cancelled"
        exit 0
        ;;
    *)
        echo "Invalid choice"
        exit 1
        ;;
esac

echo ""
echo "Done!"
