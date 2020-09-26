#!/bin/bash

run=`ls /opt/debesys/**/run`
run=${run%% *}  # May have multiple runs. Pick the first one
$run ~/logfind search $LOGFIND_FILE "$@"

