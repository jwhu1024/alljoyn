#!/bin/sh

# clean first
scons -c

# compile without debug symbol table
scons VARIANT=$1

# compile with debug symbol table
# scons VARIANT=debug
