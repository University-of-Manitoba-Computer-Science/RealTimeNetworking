#!/bin/bash

date=$(date)
entry="$1"

printf "\n# $date\n\n$entry\n" >> LOGBOOK.md
