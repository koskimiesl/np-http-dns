#!/bin/sh

nclients=$1 # number of clients (i.e. requests)
host=$2 # server hostname
port=$3 # server port
met=$4 # method: GET or PUT
file=$5 # filename prefix for request

mkdir clientfiles # directory for client directories

for i in $(seq 1 $nclients)
do
	mkdir clientfiles/client$i # make client directories beforehand (for directing stdout to output.txt)
done

for i in $(seq 1 $nclients)
do
	# run clients in parallel with '&' operator
	./httpclient -h $host -p $port -m $met -f $file$i.txt -u client$i -d clientfiles/client$i > clientfiles/client$i/output.txt &
done

wait
echo "All $nclients clients completed"

