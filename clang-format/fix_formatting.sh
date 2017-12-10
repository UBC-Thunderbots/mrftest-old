#!/bin/bash

# The version of the clang executable to use
export CLANG_VERSION=4.0

# The current directory
CURR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Check that we got exactly one argument
if [ "$#" -ne 1 ]; then
    echo "This program takes 1 argument (the name of the branch to compare against)."
    exit 1
fi
TARGET_BRANCH=$1

# Fix formatting on all changes between this branch and the target branch
OUTPUT="$($CURR_DIR/git-clang-format --binary $CURR_DIR/clang-format-$CLANG_VERSION --commit $TARGET_BRANCH)"

# Check the results of clang format
if [[ $OUTPUT == *"no modified files to format"* ]] || [[ $OUTPUT == *"clang-format did not modify any files"* ]] ; then
    # Great, we passed!
    echo "clang-format passed, no files changed :D"
    exit 0
else
    # If we failed, echo the results of clang-format so we can see what we should change
    # We're using printf here so we can replace spaces with newlines to make the output readable
    printf '%s\n' $OUTPUT
    # Since this script is used with CI, if we had to change some files, exit with 1 if we had to
    # change any files
    echo "=================================================="
    echo "Had to change some files :( "
    echo "If this is running in CI, this means the formatting check has failed. To fix things up so they pass, run the 'fix_formatting.sh' script in the 'clang-format' folder"
    exit 1
fi
