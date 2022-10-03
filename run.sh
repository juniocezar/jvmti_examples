#!/bin/bash

# Example:
# ./run.sh -cp java/ HelloWorld
#

if [ $# -lt 1 ]
then
  echo "./run.sh usual-java-options"
else
  ROOT=`dirname $0`
  AGENTPATH="$ROOT/bin/sync_jvmti.so"

  if [ -z ${JAVA_HOME+x} ]; then
    JAVA=java
  else
    JAVA="$JAVA_HOME/bin/java"
  fi

  $JAVA -agentpath:$AGENTPATH $@
fi
