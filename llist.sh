#!/bin/bash

run=`ls /opt/debesys/**/run`
run=${run%% *}  # May have multiple runs. Pick the first one
$run ~/logfind list $LOGFIND_FILE "$@"

