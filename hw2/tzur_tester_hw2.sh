#!/bin/bash
###Author: Alon Tzur

if [ "$#" -ne 1 ]; then
	echo "Please run $0 binary_name"
	exit 1
fi

BIN=$1
n=1

pidof $BIN && { echo "Failed test ${n} part 0" ; exit 1 ; }

cat << EOF > tzur_tester_tmpfile
sleep 1
cat /etc/none-existing-file
EOF

./$BIN < tzur_tester_tmpfile > tzur_tester_test${n}_actual 2>&1 &
PID=$(pidof -s $BIN) 
if [ $? -ne 0 ]; then
	echo "Problem with pidof -s $BIN"
	exit 1	
fi

sleep 1.2

diff tzur_tester_test${n}_{actual,expected} || { echo "Failed test $n part 2" ; exit 1 ; }

ps -fj --pid $PID --ppid $PID --no-headers && { echo "Failed test ${n} part 3" ; exit 1 ; }

echo "Passed test $n"

((n++))

###########

pidof $BIN && { echo "Failed test ${n} part 0" ; exit 1 ; }

cat << EOF > tzur_tester_tmpfile
sleep 3 &
sleep 5 &
sleep 7
EOF

./$BIN < tzur_tester_tmpfile > tzur_tester_test${n}_actual 2>&1 &
PID=$(pidof -s $BIN)

sleep 1

ps -fj --pid $PID --ppid $PID --no-headers | wc -l | grep -q ^4$ || { echo "Failed test ${n} part 1" ; exit 1 ; }
CPID_SLEEP3=$(ps -fj --ppid $PID --no-headers | grep "sleep 3" | awk '{print $2}')
CPID_SLEEP5=$(ps -fj --ppid $PID --no-headers | grep "sleep 5" | awk '{print $2}')
CPID_SLEEP7=$(ps -fj --ppid $PID --no-headers | grep "sleep 7" | awk '{print $2}')

sleep 2.2

ps -fj --pid $PID --ppid $PID --no-headers | wc -l | grep -q ^3$ || { echo "Failed test ${n} part 2" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_SLEEP3 && { echo "Failed test ${n} part 3" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_SLEEP5 || { echo "Failed test ${n} part 4" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_SLEEP7 || { echo "Failed test ${n} part 5" ; exit 1 ; }

sleep 2.2

ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_SLEEP5 && { echo "Failed test ${n} part 6" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_SLEEP7 || { echo "Failed test ${n} part 7" ; exit 1 ; }

sleep 3

ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_SLEEP7 && { echo "Failed test ${n} part 8" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $PID && { echo "Failed test ${n} part 9" ; exit 1 ; }

cat tzur_tester_test${n}_actual | wc -l | grep -q ^0$ || { echo "Failed test ${n} part 10" ; exit 1 ; }

echo "Passed test $n"

((n++))

###########

pidof $BIN && { echo "Failed test ${n} part 0" ; exit 1 ; }

cat << EOF > tzur_tester_tmpfile
sleep 10
sleep 1
EOF

./$BIN < tzur_tester_tmpfile > tzur_tester_test${n}_actual 2>&1 &
PID=$(pidof -s $BIN)

sleep 1

ps -fj --pid $PID --ppid $PID --no-headers | wc -l | grep -q ^2$ || { echo "Failed test ${n} part 1" ; exit 1 ; }
CPID=$(ps -fj --ppid $PID --no-headers | awk '{print $2}')

kill -2 $CPID
ps -fj --pid $PID --ppid $PID --no-headers | wc -l | grep -q ^2$ || { echo "Failed test ${n} part 2" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID && { echo "Failed test ${n} part 3" ; exit 1 ; }

sleep 1

ps -fj --pid $PID --ppid $PID --no-headers | grep -q $PID && { echo "Failed test ${n} part 4" ; exit 1 ; }

cat tzur_tester_test${n}_actual | wc -l | grep -q ^0$ || { echo "Failed test ${n} part 5" ; exit 1 ; }

echo "Passed test $n"

((n++))

##########

pidof $BIN && { echo "Failed test ${n} part 0" ; exit 1 ; }

cat << EOF > tzur_tester_tmpfile
sleep 10
sleep 1
EOF

./$BIN < tzur_tester_tmpfile > tzur_tester_test${n}_actual 2>&1 &

sleep 1

PID=$(pidof -s $BIN)
PGID=$(ps -fj --pid $PID --no-headers | awk '{print $4}')
CPID=$(ps -fj --ppid $PID --no-headers | awk '{print $2}')

ps -fj --pid $PID --ppid $PID --no-headers | wc -l | grep -q ^2$ || { echo "Failed test ${n} part 1" ; exit 1 ; }

pkill -2 -g $PGID
ps -fj --pid $PID --ppid $PID --no-headers | wc -l | grep -q ^2$ || { echo "Failed test ${n} part 2" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID && { echo "Failed test ${n} part 3" ; exit 1 ; }

sleep 1

ps -fj --pid $PID --ppid $PID --no-headers | grep -q $PID && { echo "Failed test ${n} part 4" ; exit 1 ; }

cat tzur_tester_test${n}_actual | wc -l | grep -q ^0$ || { echo "Failed test ${n} part 5" ; exit 1 ; }

echo "Passed test $n"

((n++))

##########

pidof $BIN && { echo "Failed test ${n} part 0" ; exit 1 ; }

cat << EOF > tzur_tester_tmpfile
sleep 2 &
sleep 10
sleep 1
EOF

./$BIN < tzur_tester_tmpfile > tzur_tester_test${n}_actual 2>&1 &

sleep 0.2

PID=$(pidof -s $BIN)
PGID=$(ps -fj --pid $PID --no-headers | awk '{print $4}')
CPID_FOREGROUND=$(ps -fj --ppid $PID --no-headers | grep "sleep 10" | awk '{print $2}')
CPID_BACKGROUND=$(ps -fj --ppid $PID --no-headers | grep "sleep 2" | awk '{print $2}')

ps -fj --pid $PID --ppid $PID --no-headers | wc -l | grep -q ^3$ || { echo "Failed test ${n} part 1" ; exit 1 ; }

pkill -2 -g $PGID
ps -fj --pid $PID --ppid $PID --no-headers | wc -l | grep -q ^3$ || { echo "Failed test ${n} part 2" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_FOREGROUND && { echo "Failed test ${n} part 3" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_BACKGROUND || { echo "Failed test ${n} part 4" ; exit 1 ; }

#kill -2 $CPID_BACKGROUND    #Undefined behaviour in the exercise!
#ps -fj --pid $PID --ppid $PID --no-headers | grep -q $CPID_BACKGROUND && { echo "Failed test ${n} part 5" ; exit 1 ; }

sleep 1

ps -fj --pid $PID --ppid $PID --no-headers | grep -q $PID && { echo "Failed test ${n} part 6" ; exit 1 ; }

sleep 1.5

ps -fj --pid $PID --ppid $PID --no-headers && { echo "Failed test ${n} part 7" ; exit 1 ; }

cat tzur_tester_test${n}_actual | wc -l | grep -q ^0$ || { echo "Failed test ${n} part 8" ; exit 1 ; }

echo "Passed test $n"

((n++))

##########

pidof $BIN && { echo "Failed test ${n} part 0" ; exit 1 ; }

cat << EOF > tzur_tester_tmpfile
sleep 7 &
sleep 10 &
echo test_no_1 | grep test
echo test_no_2 | grep delete
sleep 5 | sleep 6
sleep 8 | sleep 9
EOF

./$BIN < tzur_tester_tmpfile > tzur_tester_test${n}_actual 2>&1 &
sleep 1

PID=$(pidof -s $BIN)
PGID=$(ps -fj --pid $PID --no-headers | awk '{print $4}')
CPID_SLEEP5=$(ps -fj --ppid $PID --no-headers | grep "sleep 5" | awk '{print $2}')
CPID_SLEEP6=$(ps -fj --ppid $PID,$CPID_SLEEP5 --no-headers | grep "sleep 6" | awk '{print $2}')

ps -fj --pid $PID --ppid $PID,$CPID_SLEEP5 --no-headers | wc -l | grep -q ^5$ || { echo "Failed test ${n} part 1" ; exit 1 ; }

kill -2 $CPID_SLEEP5
sleep 1 
CPID_SLEEP8=$(ps -fj --ppid $PID --no-headers | grep "sleep 8" | awk '{print $2}')
CPID_SLEEP9=$(ps -fj --ppid $PID,$CPID_SLEEP8 --no-headers | grep "sleep 9" | awk '{print $2}')

ps -fj --pid $PID --ppid $PID,$CPID_SLEEP5,$CPID_SLEEP8 --no-headers | wc -l | grep -q ^5$ || { echo "Failed test ${n} part 2" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID,$CPID_SLEEP5,$CPID_SLEEP8 --no-headers | grep -q $CPID_SLEEP5 && { echo "Failed test ${n} part 3" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID,$CPID_SLEEP5,$CPID_SLEEP8 --no-headers | grep -q $CPID_SLEEP6 && { echo "Failed test ${n} part 4" ; exit 1 ; }

kill -2 $CPID_SLEEP9
sleep 1
ps -fj --pid $PID --ppid $PID,$CPID_SLEEP5,$CPID_SLEEP8 --no-headers | wc -l | grep -q ^4$ || { echo "Failed test ${n} part 5" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID,$CPID_SLEEP5,$CPID_SLEEP8 --no-headers | grep -q $CPID_SLEEP8 || { echo "Failed test ${n} part 6" ; exit 1 ; }
ps -fj --pid $PID --ppid $PID,$CPID_SLEEP5,$CPID_SLEEP8 --no-headers | grep -q $CPID_SLEEP9 && { echo "Failed test ${n} part 7" ; exit 1 ; }

sleep 7.2

diff tzur_tester_test${n}_{actual,expected} || { echo "Failed test $n part 8" ; exit 1 ; }

ps -fj --pid $PID --ppid $PID,$CPID_SLEEP5,$CPID_SLEEP8 --no-headers && { echo "Failed test ${n} part 9" ; exit 1 ; }

echo "Passed test $n"
