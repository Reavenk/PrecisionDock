#!/usr/bin/env bash

VER_MAJOR=0
VER_MINOR=0
VER_PATCH=0

if ! command -v git;
then
	echo "Could not find Git, could not auto-generate VersionInfo.h information"
	echo "Please install Git first before continuing"
	echo $PATH
	exit
fi

COMMIT_SHA=`git rev-parse HEAD`
echo "Detected the repo commit $COMMIT_SHA"

COMMIT_TIME=`git show --format=%cD --no-patch HEAD`
echo "Detected the repo commit time $COMMIT_TIME"

GENERATE_LOCATION="$(dirname "$BASH_SOURCE")/src/VersionInfo.h"
echo "Generating for $GENERATE_LOCATION"

cat<<EOF > $GENERATE_LOCATION
// This file is auto-generated by GenVersionInfo.sh - DO NOT DIRECTLY EDIT
//
const char* GV_COMMIT = "$COMMIT_SHA";
const char* GV_COMMITTIME = "$COMMIT_TIME";
const char* GV_VERSION = "$VER_MAJOR.$VER_MINOR.$VER_PATCH";
EOF