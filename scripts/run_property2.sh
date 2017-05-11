#!/bin/bash

TIMEOUT=12h

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_1_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_2_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_3_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_3.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_4_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_4.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_5_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_5.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_6_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_6.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_7_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_7.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_8_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_8.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_2_9_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_2_9.txt

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_1_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_2_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_3_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_3.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_4_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_4.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_5_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_5.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_6_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_6.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_7_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_7.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_8_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_8.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_3_9_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_3_9.txt

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_1_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_2_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_3_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_3.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_4_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_4.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_5_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_5.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_6_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_6.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_7_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_7.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_8_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_8.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_4_9_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_4_9.txt

timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_1_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_1.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_2_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_2.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_3_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_3.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_4_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_4.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_5_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_5.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_6_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_6.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_7_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_7.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_8_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_8.txt
timeout --foreground --signal=SIGQUIT $TIMEOUT ./check_properties/bin/property2.elf ./nnet/ACASXU_run2a_5_9_batch_2000.nnet logs/property2_summary.txt 2>&1 | tee logs/property2_stats_5_9.txt
