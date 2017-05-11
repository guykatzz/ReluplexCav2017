#!/bin/bash

TIMEOUT=12h

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property10.elf 1 logs/property10_summary.txt 2>&1 | tee logs/property10_stats_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property10.elf 2 logs/property10_summary.txt 2>&1 | tee logs/property10_stats_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property10.elf 3 logs/property10_summary.txt 2>&1 | tee logs/property10_stats_3.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property10.elf 4 logs/property10_summary.txt 2>&1 | tee logs/property10_stats_4.txt
