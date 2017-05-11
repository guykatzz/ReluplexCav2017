#!/bin/bash

TIMEOUT=12h

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property5.elf 0 logs/property5_summary.txt 2>&1 | tee logs/property5_stats_0.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property5.elf 1 logs/property5_summary.txt 2>&1 | tee logs/property5_stats_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property5.elf 2 logs/property5_summary.txt 2>&1 | tee logs/property5_stats_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property5.elf 3 logs/property5_summary.txt 2>&1 | tee logs/property5_stats_3.txt
