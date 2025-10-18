#!/bin/bash

# DText Release Script
# This script automates the release process for the dtext gem
#
# Usage:
#   ./scripts/release.sh [patch|minor|major] [--dry-run]
#   
# Examples:
#   ./scripts/release.sh                    # Default: patch version bump
#   ./scripts/release.sh minor              # Bump minor version
#   ./scripts/release.sh major --dry-run    # Dry run of major version bump
#   ./scripts/release.sh --dry-run          # Dry run of patch version bump

set -Eeuo pipefail  # Exit on error, unset var, pipeline fail, inherit ERR trap

# Parse command line arguments
BUMP_TYPE="patch"
DRY_RUN=false

for arg in "$@"; do
    case $arg in
        patch|minor|major)
            BUMP_TYPE="$arg"
            ;;
        --dry-run)
            DRY_RUN=true
            ;;
        --help|-h)
            echo "Usage: $0 [patch|minor|major] [--dry-run]"
            echo ""
            echo "Arguments:"
            echo "  patch     Bump patch version (default)"
            echo "  minor     Bump minor version"
            echo "  major     Bump major version"
            echo "  --dry-run Show what would happen without making changes"
            echo "  --help    Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

if [[ "$DRY_RUN" == "true" ]]; then
    echo -e "${PURPLE} DText Release Script (DRY RUN)${NC}"
    echo "=================================="
else
    echo -e "${BLUE} DText Release Script${NC}"
    echo "=========================="
fi

# Function to print colored output
print_step() {
    if [[ "$DRY_RUN" == "true" ]]; then
        echo -e "${PURPLE}[DRY RUN]${NC} ${GREEN}✓${NC} $1"
    else
        echo -e "${GREEN}✓${NC} $1"
    fi
}

print_warning() {
    if [[ "$DRY_RUN" == "true" ]]; then
        echo -e "${PURPLE}[DRY RUN]${NC} ${YELLOW}⚠${NC} $1"
    else
        echo -e "${YELLOW}⚠${NC} $1"
    fi
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_dry_run() {
    echo -e "${PURPLE}[DRY RUN]${NC} Would execute: $1"
}

# Check if we're in the right directory
if [[ ! -f "dtext.gemspec" ]]; then
    print_error "Error: Must be run from the dtext project root directory"
    exit 1
fi

# Check for uncommitted changes
if ! git diff --quiet || ! git diff --cached --quiet; then
    print_error "Error: Working directory has uncommitted changes"
    echo "Please commit or stash your changes before releasing"
    git status --porcelain
    exit 1
fi

print_step "Working directory is clean"

# Get current version
current_version=$(grep 'VERSION = ' lib/dtext/version.rb | sed 's/.*VERSION = "\([^"]*\)".*/\1/')
echo "Current version: $current_version"

# Parse version components
IFS='.' read -r major minor patch <<< "$current_version"

# Calculate new version based on bump type
case $BUMP_TYPE in
    major)
        new_major=$((major + 1))
        new_minor=0
        new_patch=0
        new_version="${new_major}.${new_minor}.${new_patch}"
        ;;
    minor)
        new_major=$major
        new_minor=$((minor + 1))
        new_patch=0
        new_version="${new_major}.${new_minor}.${new_patch}"
        ;;
    patch)
        new_major=$major
        new_minor=$minor
        new_patch=$((patch + 1))
        new_version="${new_major}.${new_minor}.${new_patch}"
        ;;
esac

echo "New version will be: $new_version (${BUMP_TYPE} bump)"

if [[ "$DRY_RUN" == "true" ]]; then
    echo
    echo -e "${PURPLE}DRY RUN MODE - No changes will be made${NC}"
    echo
else
    echo
    read -p "Continue with release $new_version? (y/N): " confirm
    
    if [[ ! $confirm =~ ^[Yy]$ ]]; then
        echo "Release cancelled"
        exit 0
    fi
fi

echo
print_step "Starting release process for $new_version"

# Step 1: Regenerate dtext.cpp
echo
echo -e "${BLUE}Step 1: Regenerating parser${NC}"
if [[ "$DRY_RUN" == "true" ]]; then
    print_dry_run "docker compose run --rm rake compile:force"
else
    docker compose run --rm rake compile:force
fi
print_step "Parser regenerated successfully"

# Step 2: Run tests
echo
echo -e "${BLUE}Step 2: Running tests${NC}"
if [[ "$DRY_RUN" == "true" ]]; then
    print_dry_run "docker compose run --rm rake test"
    print_step "Tests would be run"
else
    if docker compose run --rm rake test; then
        print_step "All tests passed"
    else
        print_error "Tests failed - aborting release"
        exit 1
    fi
fi

# Step 3: Update version
echo
echo -e "${BLUE}Step 3: Updating version${NC}"
if [[ "$DRY_RUN" == "true" ]]; then
    print_dry_run "sed -i 's/VERSION = \"$current_version\"/VERSION = \"$new_version\"/' lib/dtext/version.rb"
    echo -e "${PURPLE}[DRY RUN]${NC} Version file would change from:"
    echo "    VERSION = \"$current_version\""
    echo "  to:"
    echo "    VERSION = \"$new_version\""
else
    sed -i "s/VERSION = \"$current_version\"/VERSION = \"$new_version\"/" lib/dtext/version.rb
fi
print_step "Version updated to $new_version"

# Step 4: Commit changes
echo
echo -e "${BLUE}Step 4: Committing changes${NC}"
if [[ "$DRY_RUN" == "true" ]]; then
    print_dry_run "git add ext/dtext/dtext.cpp lib/dtext/version.rb"
    print_dry_run "git commit -m \"[Release] Version $new_version\""
    echo -e "${PURPLE}[DRY RUN]${NC} Commit message would be:"
    echo "    Release $new_version"
    echo "    "
    echo "    - Regenerate dtext.cpp"
    echo "    - Bump version to $new_version"
else
    git add ext/dtext/dtext.cpp lib/dtext/version.rb
    git commit -m "[Release] Version $new_version

- Regenerate dtext.cpp
- Bump version to $new_version"
fi
print_step "Changes committed"

# Step 5: Create tag
echo
echo -e "${BLUE}Step 5: Creating git tag${NC}"
if [[ "$DRY_RUN" == "true" ]]; then
    print_dry_run "git tag -a \"$new_version\""
else
    git tag -a "$new_version"
fi
print_step "Tag $new_version created"

# Step 6: Push changes
echo
echo -e "${BLUE}Step 6: Pushing to remote${NC}"
if [[ "$DRY_RUN" == "true" ]]; then
    print_dry_run "git push origin master"
    print_dry_run "git push origin \"$new_version\""
    print_step "Changes would be pushed to remote"
else
    echo "This will push the commit and tag to origin/master"
    read -p "Push to remote? (y/N): " push_confirm
    
    if [[ $push_confirm =~ ^[Yy]$ ]]; then
        git push origin master
        git push origin "$new_version"
        print_step "Changes pushed to remote"
    else
        print_warning "Changes committed locally but NOT pushed to remote"
    fi
fi

echo
if [[ "$DRY_RUN" == "true" ]]; then
    echo -e "${PURPLE} Dry run completed for $new_version!${NC}"
    echo
    echo "Summary of changes that would be made:"
    echo "• Parser would be regenerated from Ragel source"
    echo "• Version would be bumped from $current_version to $new_version ($BUMP_TYPE)"
    echo "• Git commit would be created with version bump and regenerated files"
    echo "• Git tag $new_version would be created"
    echo "• Changes would be pushed to origin/master"
    echo
    echo "To execute this release for real, run without --dry-run:"
    echo -e "${BLUE}./scripts/release.sh $BUMP_TYPE${NC}"
else
    echo -e "${GREEN} Release $new_version completed successfully!${NC}"
fi
