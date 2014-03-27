#!/bin/sh

# clean first
scons -c

# compile without debug symbol table
scons VARIANT=release

# compile with debug symbol table
# scons VARIANT=debug
