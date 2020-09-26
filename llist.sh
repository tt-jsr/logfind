#!/bin/bash

run=`ls /opt/debesys/**/run`
run=${run%% *}  # May have multiple runs. Pick the first one
if [[ -z $LOGFIND_FILE ]]
then
    echo "LOGFIND_FILE is not defined"
    exit 1
fi
$run ~/logfind list $LOGFIND_FILE "$@"

