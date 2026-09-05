#!/usr/bin/env bash
sum=0
count=0
for num in "$@"; do
    sum=$((sum + num))
    ((count++))
done

average=$((sum / count))

echo "количество $count"
echo "среднее $average"