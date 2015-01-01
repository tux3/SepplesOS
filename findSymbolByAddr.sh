#!/bin/bash

# Normalize our arg (lowercase, no leading 0x, but a leading 0)
needle=$(echo $1 | tr '[:upper:]' '[:lower:]')
if [ "$(echo $needle | head -c 2)" = "0x" ]
then
	needle=$(echo $needle | tail -c +3)
fi
needle=0$needle
firstHaystack=$(readelf kernel.elf -Ws | grep -Eo ": .*" | grep -Eo 0.*)
foundFirst=0
best="Not found"

(
IFS=$'\n'
for fullLine in $firstHaystack
do
	# Take only the raw address, remove the useless leading zeros (but keep at least 1 char)
	line=$(echo $fullLine | grep -Eo "^[0-9a-f]+" | sed "s/0*\([0-9a-f][0-9a-f]*\)/\1/")

	if [ "$foundFirst" = "0" ]
	then
#	echo "Searching symbol at 0x$line: $fullLine..."

	if (( 0x$line == 0x$needle ))
	then
		best=$line
		bestLine=$fullLine
		foundFirst=1
	fi

	if (( 0x$line < 0x$needle ))
	then
		if [ "$best" = "Not found" ]
		then
			best=$line
			bestLine=$fullLine
		else
			if (( 0x$line > 0x$best ))
			then
				best=$line
               	bestLine=$fullLine
			fi
		fi
	fi

	fi
done

if [ "$best" = "Not found" ]
then
	echo "Symbol not found"
else
	echo "Found symbol" $(echo $bestLine | grep -Eo '[^ ]+$') "for address $1"
fi
)
