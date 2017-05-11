#!/bin/bash

./check_properties/bin/adversarial.elf logs/adversarial_summary.txt 2>&1 | tee logs/adversarial_stats.txt
