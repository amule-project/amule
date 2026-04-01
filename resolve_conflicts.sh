#!/bin/bash

# Resolve conflicts in all files by choosing HEAD version (our fixes)
for file in $(git diff --name-only --diff-filter=U); do
    echo "Resolving conflicts in $file"
    git checkout --ours "$file"
    git add "$file"
done

echo "All conflicts resolved. Now you can commit the changes."