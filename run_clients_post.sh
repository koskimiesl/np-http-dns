#!/bin/sh

nclients=$1 # number of clients (i.e. requests)
host=$2 # server hostname
port=$3 # server port
qname=$4 # name to be queried

mkdir clientfiles # directory for client directories

for i in $(seq 1 $nclients)
do
	mkdir clientfiles/client$i # make client directories beforehand (for directing stdout to output.txt)
done

for i in $(seq 1 $nclients)
do
	# run clients in parallel with '&' operator
	./httpclient -h $host -p $port -m post -q $qname -u client$i -d clientfiles/client$i > clientfiles/client$i/output.txt &
done

wait
echo "All $nclients clients completed"
