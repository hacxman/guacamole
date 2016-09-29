#!/usr/bin/bash
COUNT=2
TOP=$(($COUNT*2))
START_PORT=20000
BINARY=usart
SOCATTODEBUG=1
for a in `seq $START_PORT 2 $(($START_PORT+$TOP))`; do
  SOCATTODEBUG=$(($SOCATTODEBUG - 1))
  V=-v
  if [ $SOCATTODEBUG -eq 0 ]; then
    V=-v
  fi
  echo socat $V tcp-l:$a,reuseaddr tcp-l:$(($a+1)),reuseaddr
  socat $V tcp-l:$a,reuseaddr tcp-l:$(($a+1)),reuseaddr &
done

echo you have 1 seconds to change desktop
echo $COUNT QEMU instances will start
sleep 1
echo /home/mzatko/local/qemu/arm-softmmu/qemu-system-arm -machine netduino2 -kernel $BINARY -d int -serial null -serial tcp:localhost:$START_PORT -serial null -serial null -serial tcp:localhost:$(($START_PORT + 2)) -nographic '2>/dev/null' '>/dev/null'
/home/mzatko/local/qemu/arm-softmmu/qemu-system-arm -machine netduino2 -kernel $BINARY -d int -serial tcp:localhost:9999 -serial tcp:localhost:$START_PORT -serial null -serial null -serial tcp:localhost:$(($START_PORT + 2)) -nographic 2>/dev/null >/dev/null &
sleep 0.4
for a in `seq $(($START_PORT + 3)) 2 $(($START_PORT+$TOP))`; do
  echo /home/mzatko/local/qemu/arm-softmmu/qemu-system-arm -machine netduino2 -kernel $BINARY -d int -serial tcp:localhost:9999 -serial tcp:localhost:$a -serial null -serial null -serial tcp:localhost:$(($a+1))
  echo $(( ($a-$START_PORT) / 2))
  /home/mzatko/local/qemu/arm-softmmu/qemu-system-arm -machine netduino2 -kernel $BINARY -d int -serial tcp:localhost:9999 -serial tcp:localhost:$a -serial null -serial null -serial tcp:localhost:$(($a+1)) -nographic 2>/dev/null >/dev/null &
  sleep 0.4
done

sleep 3
echo cluster should be up
echo to connect try:
echo nc localhost $(($START_PORT + 1)) \# first machine
echo nc localhost $(($START_PORT + ($COUNT * 2) + 1)) \# last machine
echo to kill it:
echo killall qemu-system-arm
echo killall socat
./utils/echo.py localhost $(($START_PORT + ($COUNT * 2) + 1)) -q
#nc localhost 1116
