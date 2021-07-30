#!/usr/bin/bash

N=10 # number of parallel processes
IP=localhost:8080
dir=arrival
date=03-08-2021
mode="ponctuel" #It can be cloropleth or ponctuel.
                # cloropleth: associates one uniform region to each commune
                # (one only accessibility value for the entire commune)
                # ponctuel: associates an accessibility score per each 
                # transit stop
hour1="09"
ampm1=am

hour2="09"#Use only if timeSlider

ampm2=am
count=0
minutesList=":30" #:10 :20 :30 :40 :50 :00 :10 :20 :30 :40 :50" #Use only if timeSlider

for min in $minutesList
do 
    if [ $count -gt 5 ] #Change if timeslider
    then
        hour=$hour2
        ampm=$ampm2
    
    else
        hour=$hour1
        ampm=$ampm1
    fi

    time=$hour$min

    
    for (( process_id=0; process_id<$N; process_id++))
    do
        ./stop_ponctuels/script_process_newCSV $IP $N $process_id $dir $date $time $ampm $count $mode& #Provide the right path to the executable
    done
    

    count=$(($count + 1))
done
