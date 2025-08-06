#!/bin/bash

# ============================================================================
# File: update_git.sh
# Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
# Description: Git commit and push helper with version management
# Author: Tim Rosener
# Version: 2.0.0 - Enhanced with automatic version detection and updates
# ============================================================================

echo "============================================================================"
echo "SkullStepperV4 Git Update Helper"
echo "Version: 2.0.0 - With Automatic Version Management"
echo "============================================================================"
echo ""

# Check if we're in the project directory
if [ ! -f "SkullStepperV4.ino" ]; then
    echo "Error: Not in SkullStepperV4 project directory!"
    echo "Please run this script from the project root."
    exit 1
fi

# Check if we're in a git repository
if [ ! -d .git ]; then
    echo "Error: Not in a git repository!"
    echo "Initialize with 'git init' first."
    exit 1
fi

# Extract current version from CHANGELOG.md
CURRENT_VERSION=$(grep -E "^## \[[0-9]+\.[0-9]+\.[0-9]+\]" CHANGELOG.md | head -1 | sed -E 's/## \[([0-9]+\.[0-9]+\.[0-9]+)\].*/\1/')
if [ -z "$CURRENT_VERSION" ]; then
    echo "Error: Could not determine current version from CHANGELOG.md"
    exit 1
fi

echo "Current version: $CURRENT_VERSION"
echo ""

# Function to update version in a file
update_version_in_file() {
    local file=$1
    local pattern=$2
    local replacement=$3
    
    if [ -f "$file" ]; then
        # Use sed with backup for safety
        sed -i.bak "$pattern" "$file" && rm "$file.bak"
        echo "  ✓ Updated $file"
    else
        echo "  ⚠ Warning: $file not found"
    fi
}

# Check if version needs updating in files
echo "Checking version consistency across files..."
echo "--------------------------------------------"

# List of files that contain version numbers and their patterns
# Format: filename|search_pattern|replacement_pattern

FILES_TO_CHECK=(
    "SkullStepperV4.ino|Version: [0-9]+\.[0-9]+\.[0-9]+|Version: $CURRENT_VERSION"
    "project_prompt.md|### Version [0-9]+\.[0-9]+\.[0-9]+|### Version $CURRENT_VERSION"
    "README.md|Version [0-9]+\.[0-9]+\.[0-9]+|Version $CURRENT_VERSION"
    "StepperController.cpp|Version: [0-9]+\.[0-9]+\.[0-9]+|Version: $CURRENT_VERSION"
    "SerialInterface.cpp|Version: [0-9]+\.[0-9]+\.[0-9]+|Version: $CURRENT_VERSION"
    "WebInterface.cpp|\"version\": \"[0-9]+\.[0-9]+\.[0-9]+\"|\"version\": \"$CURRENT_VERSION\""
    "SystemConfig.cpp|Version: [0-9]+\.[0-9]+\.[0-9]+|Version: $CURRENT_VERSION"
    "SafetyMonitor.cpp|Version: [0-9]+\.[0-9]+\.[0-9]+|Version: $CURRENT_VERSION"
    "DMXReceiver.cpp|Version: [0-9]+\.[0-9]+\.[0-9]+|Version: $CURRENT_VERSION"
    "GlobalInfrastructure.cpp|Version: [0-9]+\.[0-9]+\.[0-9]+|Version: $CURRENT_VERSION"
)

VERSION_UPDATED=false
for entry in "${FILES_TO_CHECK[@]}"; do
    IFS='|' read -r file search_pattern replacement_pattern <<< "$entry"
    if [ -f "$file" ]; then
        if ! grep -q "$replacement_pattern" "$file"; then
            update_version_in_file "$file" "s/$search_pattern/$replacement_pattern/g"
            VERSION_UPDATED=true
        fi
    fi
done

if [ "$VERSION_UPDATED" = true ]; then
    echo ""
    echo "Version numbers updated to $CURRENT_VERSION in all files."
    echo ""
fi

# Check if commit message provided
if [ -z "$1" ]; then
    echo "Usage: ./update_git.sh \"Your commit message\" [--tag]"
    echo ""
    echo "Options:"
    echo "  --tag    Create and push a git tag for this version"
    echo ""
    echo "Commit message format:"
    echo "  Add: New feature or capability"
    echo "  Fix: Bug fix description"
    echo "  Update: Enhancement or improvement"
    echo "  Refactor: Code restructuring"
    echo "  Docs: Documentation updates"
    echo "  Test: Test additions or changes"
    echo "  Version: Version bump to $CURRENT_VERSION"
    echo ""
    echo "Example:"
    echo "  ./update_git.sh \"Fix: Auto-home on E-stop implementation\""
    echo "  ./update_git.sh \"Version: Bump to $CURRENT_VERSION\" --tag"
    echo ""
    exit 1
fi

# Check for --tag option
CREATE_TAG=false
if [ "$2" == "--tag" ]; then
    CREATE_TAG=true
fi

# Run pre-flight check if available
if [ -x "./preflight_check.sh" ]; then
    echo "Running pre-flight check..."
    echo "----------------------------"
    ./preflight_check.sh
    echo ""
    echo "Continue with commit? (y/n)"
    read -r response
    if [[ ! "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
        echo "Commit cancelled."
        exit 0
    fi
    echo ""
fi

# Show current status
echo "Current git status:"
echo "-------------------"
git status --short
echo ""

# Count changes
CHANGES=$(git status --porcelain | wc -l)
if [ $CHANGES -eq 0 ]; then
    echo "No changes to commit."
    exit 0
fi

echo "Found $CHANGES changed file(s)."
echo ""

# Check for uncommitted version changes in CHANGELOG
if grep -q "## \[Unreleased\]" CHANGELOG.md; then
    echo "⚠ Warning: CHANGELOG.md has an [Unreleased] section."
    echo "Consider updating it with a proper version number and date."
    echo ""
fi

# Add all changes
echo "Adding all changes..."
git add -A

# Create commit
echo "Creating commit..."
git commit -m "$1"

if [ $? -ne 0 ]; then
    echo "Commit failed!"
    exit 1
fi

echo ""
echo "Commit created successfully:"
git log --oneline -1
echo ""

# Create tag if requested
if [ "$CREATE_TAG" = true ]; then
    TAG_NAME="v$CURRENT_VERSION"
    echo "Creating tag: $TAG_NAME"
    
    # Check if tag already exists
    if git rev-parse "$TAG_NAME" >/dev/null 2>&1; then
        echo "Error: Tag $TAG_NAME already exists!"
        echo "To delete and recreate: git tag -d $TAG_NAME && git push origin :refs/tags/$TAG_NAME"
        exit 1
    fi
    
    # Create annotated tag
    git tag -a "$TAG_NAME" -m "Release version $CURRENT_VERSION"
    echo "✓ Tag $TAG_NAME created"
    echo ""
fi

# Show unpushed commits
UNPUSHED=$(git log origin/main..HEAD --oneline 2>/dev/null | wc -l)
if [ $UNPUSHED -gt 0 ]; then
    echo "You have $UNPUSHED unpushed commit(s):"
    git log origin/main..HEAD --oneline
    echo ""
    
    echo "Push to remote now? (y/n)"
    read -r response
    if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
        echo "Pushing to origin/main..."
        git push origin main
        if [ $? -eq 0 ]; then
            echo "✓ Push successful!"
            
            # Push tag if it was created
            if [ "$CREATE_TAG" = true ]; then
                echo "Pushing tag $TAG_NAME..."
                git push origin "$TAG_NAME"
                if [ $? -eq 0 ]; then
                    echo "✓ Tag pushed successfully!"
                else
                    echo "✗ Tag push failed. Push manually with:"
                    echo "  git push origin $TAG_NAME"
                fi
            fi
        else
            echo "✗ Push failed. Check your connection and try again with:"
            echo "  git push origin main"
            if [ "$CREATE_TAG" = true ]; then
                echo "  git push origin $TAG_NAME"
            fi
        fi
    else
        echo "Commit saved locally. Push later with:"
        echo "  git push origin main"
        if [ "$CREATE_TAG" = true ]; then
            echo "  git push origin $TAG_NAME"
        fi
    fi
else
    echo "All commits are already pushed."
    
    # Check for unpushed tags
    if [ "$CREATE_TAG" = true ]; then
        echo ""
        echo "Pushing tag $TAG_NAME..."
        git push origin "$TAG_NAME"
        if [ $? -eq 0 ]; then
            echo "✓ Tag pushed successfully!"
        else
            echo "✗ Tag push failed. Push manually with:"
            echo "  git push origin $TAG_NAME"
        fi
    fi
fi

echo ""
echo "============================================================================"
echo "Git update complete!"
if [ "$CREATE_TAG" = true ]; then
    echo "Version $CURRENT_VERSION has been tagged as $TAG_NAME"
fi
echo "============================================================================"

# Show recent history
echo ""
echo "Recent commit history:"
echo "----------------------"
git log --oneline -5
