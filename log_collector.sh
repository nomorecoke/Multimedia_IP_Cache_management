clear
cd WORKLOAD_IP$1
cd WORKLOAD$2
grep "ipc" -rn
echo "-------------------------------------------------"
grep "committedInsts" -rn
echo "-------------------------------------------------"
grep "l2.overall_misses::cpu_cluster.cpu" -rn | grep "data"
echo "-------------------------------------------------"
grep "bw_total::total" -rn
echo "-------------------------------------------------"
grep "energy" -rn
echo "-------------------------------------------------"
grep "active_cycles" -rn
grep "true_processing" -rn
echo "-------------------------------------------------"
grep "12" -rn | grep "LRU.out" | grep "2," 
echo "-------------------------------------------------"
grep "12" -rn | grep "DCS.out" | grep "2," 


cd ..
cd ..
