#!/bin/bash

# Request a file to write results to
echo logfile name? \(should be a .h\): 
read logFile

###
##   Query and identify available MSRs
###

RANGELOW=0
RANGEHIGH=8192 # Covers entire range of possible x86 MSRs
ALLOWEDMSRS="allowlist.txt"

echo Generating MSRS for range ${RANGELOW} to ${RANGEHIGH} and printing results to ${logFile}
#Insert a header into the output file for readability
echo // FORMAT: MSR Value, If it exists, contained within allowlist, allowlist mask >> ${logFile}


for ((i=RANGELOW;i<=RANGEHIGH;i++)) do 
    addr=$(printf "%#x\n" $i) # Convert to hex
    regresult=`rdmsr ${addr} 2> /dev/null` # Filter unadressable MSRs
   if [ -z "$regresult" ]; then
       echo \{NULL, false, false, NULL\}, >> ${logFile}
   else # Register is available
       addr=$(printf "%#010x" $i) # Allowlist standard is 10 digit adresses.
       # Search first Column of allowlist for matchs to addr then extract 
       # mask from second column. Current Implementation is error resistant 
       # but inefficent.
       # TODO: Refactor to regex using BASH_REMATCH
       if cat ${ALLOWEDMSRS} | awk '{ print $1 }' | grep -q ${addr}; then
          mask=$( cat ${ALLOWEDMSRS} | grep ${addr} | awk '{ print $2 }' )
          echo \{0x${regresult}ULL, true, true, ${mask}\}, >> ${logFile}
       else
          echo \{0x${regresult}ULL, true, false, NULL\}, >> ${logFile}
       fi
    fi
done



