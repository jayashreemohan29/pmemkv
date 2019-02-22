#!/bin/sh

if [ "$#" -ne 1 ]; then
	echo "Provvide the reorder engine type"
	exit 1
fi

reorder_engine=$1
pool_path="/mnt/pmem/kvtree"
store_log="store_log.log"
pmreorder_log="pmreorder.log"

export PMEMOBJ_COW=0
echo "-------------Creating the Pmem pool--------------"
./kvtree k /mnt/pmem/kvtree

echo "-------------Tracing PUT op with Valgrind--------------"
valgrind --tool=pmemcheck -q --log-stores=yes --print-summary=no --log-file=$store_log --log-stores-stacktraces=yes --log-stores-stacktraces-depth=2 --expect-fence-after-clflush=yes ./kvtree p $pool_path

export PMEMOBJ_COW=1
echo "-------------Testing with pmreorder--------------"
python3 ../pmdk-test/src/tools/pmreorder/pmreorder.py -l $store_log -o $pmreorder_log -e debug -r $reorder_engine -p ./kvtree c
