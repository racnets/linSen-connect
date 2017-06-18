#!/bin/bash

echo "linSen data collection" 

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
	echo "abort: folder $path already exists"
#	exit
fi

#servo stuff
#speeds=(0x00 0x01 0x02 0x04 0x06 0x08 0x0A 0x0C 0x0E 0x10 0x12 0x14 0x16 0x18 0x1A 0x1C 0x1E 0x20)
speeds=(0x04)

# set servo min value
#i2cset -y $i2c_dev $servo_i2c_addr $servo_min_addr 1700 w
servo_min=$(($(i2cget -y $i2c_dev $servo_i2c_addr $servo_min_addr w)))
echo "servo_min: $servo_min"

# set servo max value
#i2cset -y $i2c_dev $servo_i2c_addr $servo_max_addr 7700 w
servo_max=$(($(i2cget -y $i2c_dev $servo_i2c_addr $servo_max_addr w)))
echo "servo_max: $servo_max"

for run in {1}
#for run in {1..10}
do
	echo "run: $run"

	for speed in "${!speeds[@]}"
	do
		s=${speeds[$speed]}
		t=10

		# set linSen brightness set-point to 2000
		$sensor_prog -w -B2000 >> $log

		# set servo to initial position and speed
		i2cset -y $i2c_dev $servo_i2c_addr $servo_pos_addr $servo_min w
		i2cset -y $i2c_dev $servo_i2c_addr $servo_speed_addr 1000 w
		sleep 1
		
		# prepare for logging
		d=$(date +%H%M%S)
		id="speed$s""_$d"

		program1=$sensor_prog" -c$t -f $path/$id.dat -r -E -P -B -R -Q -A -X &" >> log
		program2=$servo_prog" -c$t -f $path/$id""_bright.dat -r -Q -A -p &" >> log

#		echo "$program1"
#		echo "$program2"
		
		# read result in order to empty output buffer
		$sensor_prog -r -R >> $log
		
		# set servo target position and speed
		i2cset -y $i2c_dev $servo_i2c_addr $servo_speed_addr $s w
		i2cset -y $i2c_dev $servo_i2c_addr $servo_pos_addr $servo_max w
		
		eval $program2 
		eval $program1
		wait
	done
done

# stop servo
i2cset -y $i2c_dev $servo_i2c_addr $servo_speed_addr 0x00 w

