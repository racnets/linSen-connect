#!/bin/bash

echo "linSen long term data collection" 

# buld log-file name
log=${0##*/}
log=${log%.*}.log

sensor_prog="./linSen-connect -i/dev/i2c-1"
servo_prog="./linSen-connect -i/dev/i2c-1 -a 0x20"
i2c_dev=1
servo_i2c_addr=0x20
servo_pos_addr=0x38
servo_min_addr=0x40
servo_max_addr=0x48
servo_speed_addr=0x50

if [[ -z $1 ]]; then
	echo "abort: no argument supplied - expected path"
	exit
fi

path=$1
if [[ ! -d $path ]]; then
	$(mkdir $path)
else
	echo "info: folder $path already exists"
#	exit
fi


# set servo min value
#i2cset -y $i2c_dev $servo_i2c_addr $servo_min_addr 1700 w
servo_min=$(($(i2cget -y $i2c_dev $servo_i2c_addr $servo_min_addr w)))
echo "servo_min: $servo_min"

# set servo max value
#i2cset -y $i2c_dev $servo_i2c_addr $servo_max_addr 7700 w
servo_max=$(($(i2cget -y $i2c_dev $servo_i2c_addr $servo_max_addr w)))
echo "servo_max: $servo_max"

#set pixel clock to 100kHz
#$sensor_prog -w -P100 >> $log

while :
do
	#for bright in 0 1000 2000 3000 4000
	for bright in 1000 2000 3000
	do
		# set linSen brightness set-point
		$sensor_prog -w -B$bright >> $log
		sleep 2

		#for speed in "${speeds[@]}"
		for ((speed=0; speed<=300; speed+=4))
		#for ((speed=0; speed<=500; speed+=1))
		do
			t=3			
			echo "actual run for brightness: $bright	 speed: $speed"
			
			# set servo to initial position and speed
			i2cset -y $i2c_dev $servo_i2c_addr $servo_pos_addr $servo_min w
			i2cset -y $i2c_dev $servo_i2c_addr $servo_speed_addr 1000 w
			sleep 1;
			
			# prepare for logging
			d=$(date +%H%M%S)
			id="$d""_speed$speed""_bright$bright"

			program1="$sensor_prog -c$t -f $path/$id.dat -r -E -P -B -R -Q -A -X >> $log &"
			program2="$servo_prog -c$t -f $path/$id""_bright.dat -r -Q -A -p >> $log &"
			#echo "$program1"
			#echo "$program2"
			
			# read result in order to flush output buffer
			$sensor_prog -r -R >> $log
		
			# set servo target speed and position
			i2cset -y $i2c_dev $servo_i2c_addr $servo_speed_addr $speed w
			i2cset -y $i2c_dev $servo_i2c_addr $servo_pos_addr $servo_max w

			eval $program1
			eval $program2

			wait
			
			# stop servo
			i2cset -y $i2c_dev $servo_i2c_addr $servo_speed_addr 0 w
		done
	done
	
	echo "wait 30 seconds - hit any key to abort"
	if read -t 30 response; then
		break;
	fi
done
