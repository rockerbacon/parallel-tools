#!/bin/bash

SCRIPT_DIR=$(dirname $0)
PROJECT_ROOT=$(realpath "$SCRIPT_DIR/../../../..")
MODULE_ROOT=$(realpath "$SCRIPT_DIR")
DEPENDENCIES_DIR="$PROJECT_ROOT/external_dependencies"
DEPENDENCIES_OBJ_DIR="$DEPENDENCIES_DIR/objs"
REPOSITORIES_DIR="$DEPENDENCIES_DIR/git"

mkdir -p "$REPOSITORIES_DIR"

##################### Command Line Interface ##########################
GIT_URL="$1"
if [ "$GIT_URL" == "" ]; then
	echo "Error: unspecified git URL"
	exit 1
fi
GIT_COMMIT="$2"
##################### Command Line Interface ##########################

GIT_URL_IS_HTTP=$(echo "$GIT_URL" | grep -oe "^http")
if [ "$GIT_URL_IS_HTTP" == "" ]; then
	echo "Error: not an HTTP git URL"
	exit 1
fi

RELATIVE_DEPENDENCY_REPOSITORY_DIR=$(echo "$GIT_URL" | sed "s/^.*\///; s/\.git$//")
DEPENDENCY_REPOSITORY_DIR="$REPOSITORIES_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR"

rollback_installation () {
	if [ -d "$DEPENDENCY_REPOSITORY_DIR/.git" ]; then
		echo "Rolling back: deleting '$DEPENDENCY_REPOSITORY_DIR'"
		rm -rf "$DEPENDENCY_REPOSITORY_DIR"
	fi
}

if [ -d "$DEPENDENCY_REPOSITORY_DIR" ]; then
	echo "Info: Dependency '$DEPENDENCY_REPOSITORY_DIR' already cloned" 1>&2
else
	cd "$REPOSITORIES_DIR"
	git clone "$GIT_URL"
	GIT_EXECUTION_STATUS=$?
	if [ "$GIT_EXECUTION_STATUS" != "0" ]; then
		exit 1
	fi
fi

cd "$DEPENDENCY_REPOSITORY_DIR"
if [ "$GIT_COMMIT" != "" ]; then
	git checkout $GIT_COMMIT
	CHECKOUT_STATUS=$?
	if [ "$CHECKOUT_STATUS" != "0" ]; then
		echo "Error: not a valid commit: '$GIT_COMMIT'"
		rollback_installation
		exit 1
	fi
else
	LASTEST_COMMIT=$(git log | grep -m 1 "^commit" | sed "s/commit //")
	echo "Info: commit not specified, using latest ($LASTEST_COMMIT)" 1>&2
	GIT_COMMIT=$LASTEST_COMMIT
fi

if [ -f "$DEPENDENCY_REPOSITORY_DIR/.assertions/language" ]; then
	DEPENDENCY_LANGUAGE=$(cat "$DEPENDENCY_REPOSITORY_DIR/.assertions/language")
	if [ "$DEPENDENCY_LANGUAGE" == "cpp" ]; then
		ln -s "$DEPENDENCY_REPOSITORY_DIR/src/objs" "$DEPENDENCIES_OBJ_DIR/$RELATIVE_DEPENDENCY_REPOSITORY_DIR"
	else
		echo "Error: project uses Assertions, but it's not meant for this languague. Expected language: 'cpp', language reported by dependency: '${DEPENDENCY_LANGUAGE}'"
		rollback_installation
		exit 1
	fi
	echo "Info: dependency configured: $GIT_URL $GIT_COMMIT"
else
	echo "Error: dependencies can only be from projects using the Assertions C++ Framework"
	rollback_installation
	exit 1
fi

