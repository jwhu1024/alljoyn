Alljoyn thin client and service
=====

Setup
=====
Clone from https://lester60@bitbucket.org/lester60/alljoyn_c_application.git

Requirements:
- python 2.6 or 2.7
- scons (sudo apt-get install scons)

To build with scons:

    $ cd alljoyn_c_application
    $ scons VARIANT=(release, debug)

Clean libjson-br
=====
- scons -c