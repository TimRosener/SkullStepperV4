#!/bin/bash

echo "============================================================================"
echo "Git Status Check and Push Helper"
echo "============================================================================"
echo ""

# Show current branch
echo "Current branch: $(git branch --show-current)"
echo ""

# Show unpushed commits
UNPUSHED=$(git log origin/main..HEAD --oneline 2>/dev/null | wc -l)
if [ $UNPUSHED -gt 0 ]; then
    echo "You have $UNPUSHED unpushed commit(s):"
    git log origin/main..HEAD --oneline
    echo ""
    
    echo "Do you want to push these commits to GitHub? (y/n)"
    read -r response
    if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
        echo "Pushing to origin/main..."
        git push origin main
        if [ $? -eq 0 ]; then
            echo "✓ Push successful!"
            echo ""
            echo "Your changes should now be visible at:"
            echo "https://github.com/timrosener/SkullStepperV4"
        else
            echo "✗ Push failed. Please check your connection and credentials."
        fi
    else
        echo "Push cancelled. Run 'git push origin main' when ready."
    fi
else
    echo "All commits are already pushed to GitHub."
    echo ""
    echo "Your repository is up to date at:"
    echo "https://github.com/timrosener/SkullStepperV4"
fi

echo ""
echo "============================================================================"
