#!/bin/bash

run=`ls /opt/debesys/**/run`
run=${run%% *}  # May have multiple runs. Pick the first one
if [[ -z $LOGFIND_LOGNAME ]]
then
    echo "LOGFIND_LOGNAME is not defined"
    exit 1
fi
$run ~/logfind list $LOGFIND_LOGNAME "$@"

