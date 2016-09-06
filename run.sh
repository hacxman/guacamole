#!/usr/bin/bash
COUNT=70
TOP=$(($COUNT*2))
START_PORT=20000
for a in `seq $START_PORT 2 $(($START_PORT+$TOP))`; do
  echo socat tcp-l:$a,reuseaddr tcp-l:$(($a+1)),reuseaddr
  socat tcp-l:$a,reuseaddr tcp-l:$(($a+1)),reuseaddr &
done

echo you have 6 seconds to change desktop
echo $COUNT QEMU instances will start
sleep 6
/home/mzatko/local/qemu/arm-softmmu/qemu-system-arm -machine netduino2 -kernel usart.elf -d int -serial null -serial tcp:localhost:$START_PORT -serial null -serial null -serial tcp:localhost:$(($START_PORT + 2)) -nographic 2>/dev/null >/dev/null &
sleep 1
for a in `seq $(($START_PORT + 3)) 2 $(($START_PORT+$TOP))`; do
  echo /home/mzatko/local/qemu/arm-softmmu/qemu-system-arm -machine netduino2 -kernel usart.elf -d int -serial null -serial tcp:localhost:$a -serial null -serial null -serial tcp:localhost:$(($a+1))
  echo $(( ($a-$START_PORT) / 2))
  /home/mzatko/local/qemu/arm-softmmu/qemu-system-arm -machine netduino2 -kernel usart.elf -d int -serial null -serial tcp:localhost:$a -serial null -serial null -serial tcp:localhost:$(($a+1)) -nographic 2>/dev/null >/dev/null &
  sleep 0.4
done

echo cluster should be up
echo to connect try:
echo nc localhost $(($START_PORT + 1)) \# first machine
echo nc localhost $(($START_PORT + ($COUNT * 2) + 1)) \# last machine
echo to kill it:
echo killall qemu-system-arm
echo killall socat
#nc localhost 1116
