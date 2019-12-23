#!/bin/bash

SCRIPT_DIR=$(dirname $0)
PROJECT_ROOT=$(realpath "$SCRIPT_DIR/../../../..")
MODULE_ROOT=$(realpath "$SCRIPT_DIR")
DEPENDENCIES_DIR="$PROJECT_ROOT/external_dependencies"
DEPENDENCIES_LIB_DIR="$DEPENDENCIES_DIR/lib"
DEPENDENCIES_INCLUDE_DIR="$DEPENDENCIES_DIR/include"
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

if [ ! -L "$DEPENDENCIES_LIB_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR" ]; then
	echo "Info: dependency library not linked" 1>&2
else
	echo "Info: removing link '$DEPENDENCIES_LIB_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR'" 1>&2
	rm -rf "$DEPENDENCIES_LIB_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR"
fi

if [ ! -L "$DEPENDENCIES_INCLUDE_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR" ]; then
	echo "Info: dependency headers not linked" 1>&2
else
	echo "Info: removing link '$DEPENDENCIES_INCLUDE_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR'" 1>&2
	rm -rf "$DEPENDENCIES_INCLUDE_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR"
fi

