#!/bin/bash

TIMEOUT=12h

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property9.elf 0 logs/property9_summary.txt 2>&1 | tee logs/property9_stats_0.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property9.elf 1 logs/property9_summary.txt 2>&1 | tee logs/property9_stats_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property9.elf 2 logs/property9_summary.txt 2>&1 | tee logs/property9_stats_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property9.elf 4 logs/property9_summary.txt 2>&1 | tee logs/property9_stats_4.txt
