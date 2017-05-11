#!/bin/bash

TIMEOUT=12h

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property6a.elf 1 logs/property6_summary.txt 2>&1 | tee logs/property6_lower_stats_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property6a.elf 2 logs/property6_summary.txt 2>&1 | tee logs/property6_lower_stats_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property6a.elf 3 logs/property6_summary.txt 2>&1 | tee logs/property6_lower_stats_3.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property6a.elf 4 logs/property6_summary.txt 2>&1 | tee logs/property6_lower_stats_4.txt

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property6b.elf 1 logs/property6_summary.txt 2>&1 | tee logs/property6_upper_stats_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property6b.elf 2 logs/property6_summary.txt 2>&1 | tee logs/property6_upper_stats_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property6b.elf 3 logs/property6_summary.txt 2>&1 | tee logs/property6_upper_stats_3.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property6b.elf 4 logs/property6_summary.txt 2>&1 | tee logs/property6_upper_stats_4.txt
