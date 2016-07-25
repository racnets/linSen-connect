#!/bin/bash

echo "linSen data collection" 

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
$(i2cset -y $i2c_dev $servo_i2c_addr $servo_max_addr 1700 w)

# set servo max value
$(i2cset -y $i2c_dev $servo_i2c_addr $servo_max_addr 7700 w)

for run in {1}
#for run in {1..10}
do
	echo "run: $run"

	for speed in "${!speeds[@]}"
	do
		s=${speeds[$speed]}
		t=10

		# set linSen brightness set-point to 2000
		$sensor_prog -w -B2000
		sleep 1

		#set servo initial position and speed
		$(i2cset -y $i2c_dev $servo_i2c_addr $servo_pos_addr 0x00 w)
		$(i2cset -y $i2c_dev $servo_i2c_addr $servo_speed_addr $s w)

		d=$(date +%H%M%S)
		id="speed$s""_$d"

		program1=$sensor_prog" -c$t -f $path/$id.dat -r -E -P -B -R -Q -A -X &"
		program2=$servo_prog" -c$t -f $path/$id""_bright.dat -r -Q -A &"

		echo "$program1"
		echo "$program2"
		eval $program1
		eval $program2
		wait
	done
done

# stop servo
$(i2cset -y $i2c_dev $servo_i2c_addr $servo_speed_addr 0x00 w)

