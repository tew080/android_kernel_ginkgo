#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Modules specific tests cases

# protect against multiple inclusion
if [ $FILE_MODULE ]; then
	return 0
else
	FILE_MODULE=DONE
fi

source cpu.sh
source cpufreq.sh
source governor.sh

# Check basic insmod/rmmod
# $1: module
test_basic_insmod_rmmod()
{
	printf "** Test: Running ${FUNCNAME[0]} **\n\n"

	printf "Inserting $1 module\n"
	# insert module
	insmod $1
	if [ $? != 0 ]; then
		printf "Insmod $1 failed\n"
		exit;
	fi

	printf "Removing $1 module\n"
	# remove module
	rmmod $1
	if [ $? != 0 ]; then
		printf "rmmod $1 failed\n"
		exit;
	fi

	printf "\n"
}

# Insert cpufreq driver module and perform basic tests
# $1: cpufreq-driver module to insert
# $2: If we want to play with CPUs (1) or not (0)
module_driver_test_single()
{
	printf "** Test: Running ${FUNCNAME[0]} for driver $1 and cpus_hotplug=$2 **\n\n"

	if [ $2 -eq 1 ]; then
		# offline all non-boot CPUs
		for_each_non_boot_cpu offline_cpu
		printf "\n"
	fi

	# insert module
	printf "Inserting $1 module\n\n"
	insmod $1
	if [ $? != 0 ]; then
		printf "Insmod $1 failed\n"
		return;
	fi

	if [ $2 -eq 1 ]; then
		# online all non-boot CPUs
		for_each_non_boot_cpu online_cpu
		printf "\n"
	fi

	# run basic tests
	cpufreq_basic_tests

	# remove module
	printf "Removing $1 module\n\n"
	rmmod $1
	if [ $? != 0 ]; then
		printf "rmmod $1 failed\n"
		return;
	fi

	# There shouldn't be any cpufreq directories now.
	for_each_cpu cpu_should_not_have_cpufreq_directory
	printf "\n"
}

# $1: cpufreq-driver module to insert
module_driver_test()
{
	printf "** Test: Running ${FUNCNAME[0]} **\n\n"

	# check if module is present or not
	ls $1 > /dev/null
	if [ $? != 0 ]; then
		printf "$1: not present in `pwd` folder\n"
		return;
	fi

	# test basic module tests
	test_basic_insmod_rmmod $1

	# Do simple module test
	module_driver_test_single $1 0

	# Remove CPUs before inserting module and then bring them back
	module_driver_test_single $1 1
	printf "\n"
}

# test modules: driver and governor
# $1: driver module, $2: governor module
module_test()
{
	printf "** Test: Running ${FUNCNAME[0]} **\n\n"

	# check if modules are present or not
	ls $1 $2 > /dev/null
	if [ $? != 0 ]; then
		printf "$1 or $2: is not present in `pwd` folder\n"
		return;
	fi

	# TEST1: Insert gov after driver
	# insert driver module
	printf "Inserting $1 module\n\n"
	insmod $1
	if [ $? != 0 ]; then
		printf "Insmod $1 failed\n"
		return;
	fi

	# remove driver module
	printf "Removing $1 module\n\n"
	rmmod $1
	if [ $? != 0 ]; then
		printf "rmmod $1 failed\n"
		return;
	fi

	# TEST2: Insert driver after governor
	# insert governor module
	printf "Inserting $2 module\n\n"
	insmod $2
	if [ $? != 0 ]; then
		printf "Insmod $2 failed\n"
		return;
	fi

	# run governor tests
	module_driver_test $1

	# remove driver module
	printf "Removing $2 module\n\n"
	rmmod $2
	if [ $? != 0 ]; then
		printf "rmmod $2 failed\n"
		return;
	fi
}
