#!/bin/bash

SCRIPT_DIR=$(dirname $0)
PROJECT_ROOT=$(realpath "$SCRIPT_DIR/../../../..")
MODULE_ROOT=$(realpath "$SCRIPT_DIR")
DEPENDENCIES_DIR="$PROJECT_ROOT/external_dependencies"
DEPENDENCIES_OBJ_DIR="$DEPENDENCIES_DIR/obj"
REPOSITORIES_DIR="$DEPENDENCIES_DIR/git"

##################### Command Line Interface ##########################
GIT_URL="$1"
if [ "$GIT_URL" == "" ]; then
	echo "Error: unspecified git URL"
	exit 1
fi
##################### Command Line Interface ##########################

RELATIVE_DEPENDENCY_REPOSITORY_DIR=$(echo "$GIT_URL" | sed "s/^.*\///; s/\.git$//")
DEPENDENCY_REPOSITORY_DIR="$REPOSITORIES_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR"

if [ ! -d "$DEPENDENCY_REPOSITORY_DIR" ]; then
	echo "Info: dependency not installed" 1>&2
else
	if [ ! -d "$DEPENDENCY_REPOSITORY_DIR/.git" ]; then
		echo "Error: dependency is not a git repository"
		exit 1
	elif [ ! -d "$DEPENDENCY_REPOSITORY_DIR/.assertions" ]; then
		echo "Error: dependency does not use assertions"
		exit 1
	fi
	echo "Info: deleting '$DEPENDENCY_REPOSITORY_DIR'" 1>&2
	rm -rf "$DEPENDENCY_REPOSITORY_DIR"
fi

if [ ! -L "$DEPENDENCIES_OBJ_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR" ]; then
	echo "Info: dependency source not linked" 1>&2
else
	echo "Info: removing link '$DEPENDENCIES_OBJ_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR'" 1>&2
	rm -rf "$DEPENDENCIES_OBJ_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR"
fi

