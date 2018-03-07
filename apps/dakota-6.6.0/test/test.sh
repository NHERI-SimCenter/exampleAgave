#!/bin/bash

DIR=$( cd "$( dirname "$0" )" && pwd )

# set test variables
export AGAVE_JOB_MEMORY_PER_NODE=1
export AGAVE_JOB_NAME=some-test-job-form-my-research
export AGAVE_JOB_CALLBACK_FAILURE=
export AGAVE_JOB_CALLBACK_RUNNING=

export inputFile=dakota.in
export driverFile=fem_driver
export modules=petsc,dakota

# stage file to root as it would be during a run
cp -r $DIR/* $DIR/../

# call wrapper script as if the values had been injected by the API
sh -c ../wrapper.sh

# clean up after the run completes
rm $DIR/../dakota.*
rm -fr $DIR/../templatedir
rm -fr $DIR/../workdir.*


