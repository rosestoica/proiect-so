#!/bin/bash
file=$1
chmod 777 $file

l=$(wc -l < "$file")
c=$(wc -c < "$file")
c1=$(wc -w < "$file")

if [ $(wc -l < "$file") -lt 1 ] || [ $(wc -l < "$file") -gt 1000 ]; then #limita pt lini 1-1000
	{
		chmod 000 $file
		echo "$file"
		exit 1
	}

fi

if [ $(wc -w < "$file") -lt 5 ] || [ $(wc -w < "$file") -gt 100000 ]; then #limita pt cuv 5-100000
	{
		chmod 000 $file
		echo "$file"
		exit 1
	}

fi


if [ $(wc -c < "$file") -lt 3 ] || [ $(wc -w < "$file") -gt 1000000 ]; then #limita pt cuv 3-1000000
	{
		chmod 000 $file
		echo "$file"
		exit 1
	}

fi

if [ $(wc -l < "$file") -lt 3 ] && [ $(wc -w < "$file") -gt 1000 ] && [ $(wc -c < "$file") -gt 2000 ]; then #conditia de lini putine, cuv multe,carac multe
	{
		chmod 000 $file
		echo "$file"
		exit 1
	}

fi

if grep -qP "[^\x00-\x7F]" "$file"; then
{
		chmod 000 $file
		echo "$file"
		exit 1
}
fi
if grep -qP 'dangerous|risk|attack|corrupted|malware|malicious' "$file";
then

	{
		chmod 000 $file
		echo "$file"
		exit 1
	}

fi
 chmod 000 $file
 echo "SAFE"
 exit 1












