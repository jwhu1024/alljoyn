Alljoyn thin client and service
=====

Setup
=====
Clone from https://lester60@bitbucket.org/lester60/alljoyn_c_application.git

Requirements
=====
- python 2.6 or 2.7
- scons (sudo apt-get install scons)

Build with scons:
=====
    $ cd alljoyn_c_application
    $ scons VARIANT=(release, debug)

Clean
=====
- scons -c

Run alljoyn
=====
- run ./bin/alljoyn_daemon
- run ./aj_c_client
- run ./aj_c_service
- run ./door_client
- run ./door_service