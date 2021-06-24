N=1 #Number of timestamps (only for timeSlider Choropleth maps, N=1 for ponctual maps)


#Provide correct path to csvs

for (( process_id=0; process_id<$N; process_id++))
do
    awk '(NR==1) || (FNR>1)' ./csv/${process_id}a*.csv > ./csv/combined_$process_id.csv
done

awk '(NR==1) || (FNR>1)' ./csv/combined*.csv > ./csv/merged.csv


# suppression \n une ligne sur 2

#count=1
#touch ./csv/final_merged.csv

#while read line
#do 
    #if [ $((count%2)) = 0 ]
    #then 
        #printf "%s" "$line"
        #echo $line | tr -d "\n" <<< "$line" >> ./csv/final_merged.csv
    #else 
        #echo $line >> ./csv/final_merged.csv
    #fi
#count=`expr $count + 1`
#done < ./csv/merged.csv !
