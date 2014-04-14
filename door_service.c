/**
 * @file
 * @brief Sample implementation of an AllJoyn service in C.
 *
 * This sample will show how to set up an AllJoyn service that will registered with the
 * wellknown name 'org.alljoyn.Bus.method_sample'.  The service will register a method call
 * with the name 'cat'  this method will take two input strings and return a
 * Concatenated version of the two strings.
 *
 */

/******************************************************************************
 * Copyright (c) 2010-2013, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/version.h>
#include <alljoyn_c/Status.h>

#include <alljoyn_c/MsgArg.h>

#define APP_NAME "door_app_srv"
#define SIG_NAME "door_signal"
#define SIG_SIGN "i"

/*constants*/
static const char* INTERFACE_NAME = "com.BandRich.signal";
static const char* OBJECT_NAME = "com.BandRich.signal";
static const char* CONNECTSPEC = "unix:abstract=alljoyn";
static const alljoyn_sessionport SERVICE_PORT = 1024;

static volatile sig_atomic_t g_interrupt = QCC_FALSE;
static QCC_BOOL g_found = QCC_FALSE;

static void SigIntHandler(int sig)
{
    g_interrupt = QCC_TRUE;
}

/* AcceptSessionJoiner callback */
QCC_BOOL accept_session_joiner(const void* context, alljoyn_sessionport sessionPort,
                               const char* joiner,  const alljoyn_sessionopts opts)
{
    QCC_BOOL ret = QCC_FALSE;
    if (sessionPort != SERVICE_PORT) {
        printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
    } else {
        printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
               joiner, alljoyn_sessionopts_get_proximity(opts), alljoyn_sessionopts_get_traffic(opts), alljoyn_sessionopts_get_transports(opts));
        ret = QCC_TRUE;
    }
    return ret;
}

/* NameOwnerChanged callback */
void name_owner_changed(const void* context, const char* busName, const char* previousOwner, const char* newOwner)
{
    if (newOwner && (0 == strcmp(busName, OBJECT_NAME))) {
        printf("name_owner_changed: name=%s, oldOwner=%s, newOwner=%s\n",
               busName,
               previousOwner ? previousOwner : "<none>",
               newOwner ? newOwner : "<none>");
    }
}

QStatus bus_create(alljoyn_busattachment *bus)
{
	*bus = alljoyn_busattachment_create(APP_NAME, QCC_TRUE);
	return (*bus != NULL) ? ER_OK : ER_FAIL;
}

QStatus bus_connect(alljoyn_busattachment *bus)
{
	/* Start the msg bus */
    QStatus status = alljoyn_busattachment_start(*bus);
    if (ER_OK == status) {
		status = alljoyn_busattachment_connect(*bus, CONNECTSPEC);
        if (ER_OK != status) {
            printf("alljoyn_busattachment_connect(\"%s\") failed\n", CONNECTSPEC);
        } else {
            printf("alljoyn_busattachment connected to \"%s\"\n", alljoyn_busattachment_getconnectspec(*bus));
        }
    } else {
        printf("alljoyn_busattachment_start (%s) failed\n", QCC_StatusText(status));
    }
	
	return status;
}

QStatus bus_register(alljoyn_busattachment *bus, alljoyn_buslistener *busListener)
{
    /* Create a bus listener */
    alljoyn_buslistener_callbacks callbacks = {
		NULL,
		NULL,
		NULL,
		NULL,
		&name_owner_changed,
		NULL,
		NULL,
		NULL
	};
    
	*busListener = alljoyn_buslistener_create(&callbacks, NULL);
	alljoyn_busattachment_registerbuslistener(*bus, *busListener);
	
	return (*busListener != NULL) ? ER_OK : ER_FAIL;
}

QStatus create_iface(alljoyn_busattachment *bus, alljoyn_interfacedescription *iface)
{
	QStatus status = alljoyn_busattachment_createinterface(*bus, INTERFACE_NAME, iface);
    if (status == ER_OK) {
        alljoyn_interfacedescription_addsignal(*iface,
											   SIG_NAME,
											   SIG_SIGN,
											   "state",
											   0,
											   NULL);

        alljoyn_interfacedescription_activate(*iface);
        printf("Interface Created.\n");
    } else {
        printf("Failed to create interface '%s'\n", INTERFACE_NAME);
	}
	return status;
}

QStatus session_create(alljoyn_busattachment *bus, alljoyn_sessionportlistener *sessionPortListener, alljoyn_sessionopts *opts, alljoyn_sessionport sp)
{
	QStatus status;
	alljoyn_sessionportlistener_callbacks spl_cbs = {
        accept_session_joiner,
        NULL
    };
	
	/* Create session port listener */
    *sessionPortListener = alljoyn_sessionportlistener_create(&spl_cbs, NULL);

    /* Create session */
    *opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    status = alljoyn_busattachment_bindsessionport(*bus, &sp, *opts, *sessionPortListener);
    if (ER_OK != status) {
        printf("alljoyn_busattachment_bindsessionport failed (%s)\n", QCC_StatusText(status));
    }
	return status;
}

QStatus advertise_name(alljoyn_busattachment *bus, alljoyn_sessionopts *opts)
{
	/* Request name */
    uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
    QStatus status = alljoyn_busattachment_requestname(*bus, OBJECT_NAME, flags);
    if (ER_OK == status) {
		/* Advertise name */
		status = alljoyn_busattachment_advertisename(*bus, OBJECT_NAME, alljoyn_sessionopts_get_transports(*opts));
		if (status != ER_OK) {
			printf("Failed to advertise name %s (%s)\n", OBJECT_NAME, QCC_StatusText(status));
		}
    }
	return status;
}

void program_uninitialize(alljoyn_busattachment 		*bus,
						  alljoyn_buslistener 			*busListener,
						  alljoyn_busobject 			*bus_object,
						  alljoyn_interfacedescription 	*iface,
						  alljoyn_sessionopts 			*opts,
						  alljoyn_sessionportlistener 	*spl)
{
	/* Deallocate sessionopts */
    if (*opts) {
        alljoyn_sessionopts_destroy(*opts);
    }
	
    /* Deallocate bus */
    if (*bus) {
        alljoyn_busattachment deleteMe = *bus;
        *bus = NULL;
        alljoyn_busattachment_destroy(deleteMe);
    }

    /* Deallocate bus listener */
    if (*busListener) {
        alljoyn_buslistener_destroy(*busListener);
    }

    /* Deallocate session port listener */
    if (*spl) {
        alljoyn_sessionportlistener_destroy(*spl);
    }

    /* Deallocate the bus object */
    if (*bus_object) {
        alljoyn_busobject_destroy(*bus_object);
    }
}

void signalHandler(const alljoyn_interfacedescription_member* member, const char* srcPath, alljoyn_message message)
{
	printf("*********Received signal*********\n");
	
	const char *interface = alljoyn_message_getinterface(message);
    const char *member_name = alljoyn_message_getmembername(message);

    if (!interface || strcmp(interface, INTERFACE_NAME) ||
        !member_name || strcmp(member_name, SIG_NAME)) {
        printf("Unknown interface and member");
        return;
    }

    alljoyn_msgarg inputs;
    size_t inputs_size = 0;

    alljoyn_message_getargs(message, &inputs_size, &inputs);

    if (inputs_size != 1) {
        printf("Invalid number of arguments.\n");
        return;
    }

    int state = -1;
    alljoyn_msgarg_get(alljoyn_msgarg_array_element(inputs, 0), "i", &state);

    printf("DOOR : %d\n", state);	
	g_found = QCC_TRUE;
}

QStatus register_signal_handler(alljoyn_busattachment *bus, alljoyn_interfacedescription *iface)
{
	QStatus status;
	QCC_BOOL get_member = QCC_FALSE;
	alljoyn_interfacedescription_member member;
    get_member = alljoyn_interfacedescription_getmember(*iface, SIG_NAME, &member);
	
	if (QCC_FALSE == get_member) {
		printf("alljoyn_interfacedescription_getmember FAIL\n");
	}

    status = alljoyn_busattachment_registersignalhandler(*bus, signalHandler, member, NULL);
	
    if (ER_OK == status) {
		char szRule[128] = {0};
		sprintf(szRule, "type='signal',interface='%s',member='%s'", INTERFACE_NAME, SIG_NAME);
        status = alljoyn_busattachment_addmatch(*bus, szRule);
    } else {
		printf("alljoyn_busattachment_addmatch Fail reason is %s\n", QCC_StatusText(status));
	}
	
	return status;
}

/** Main entry point */
int main(int argc, char** argv, char** envArg)
{
	QStatus status = ER_OK;
	
	alljoyn_busattachment aj_bus = NULL;
    alljoyn_interfacedescription interface = NULL;
	alljoyn_buslistener busListener = NULL;
    alljoyn_busobject bus_object = NULL;
	alljoyn_sessionopts opts;
    alljoyn_sessionport session_port = SERVICE_PORT;
    alljoyn_sessionportlistener session_port_listener = NULL;
	
	/* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);
	
	printf("AllJoyn Library version: %s\n", alljoyn_getversion());
    printf("AllJoyn Library build info: %s\n", alljoyn_getbuildinfo());
	
	// create bus
	status = bus_create(&aj_bus);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Bus Create Failed\n");
		goto oops;
	}
	
#if 1
	// register bus
	status = bus_register(&aj_bus, &busListener);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Bus Register Failed\n");
		goto oops;
	}
#endif
	
	// connect bus
	status = bus_connect(&aj_bus);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Bus Connect Failed\n");
		goto oops;
	}
	
	// create interface
	status = create_iface(&aj_bus, &interface);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Interface Create Failed\n");
		goto oops;
	}
	
	// register signal handler
	status = register_signal_handler(&aj_bus, &interface);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Register Handler Failed\n");
		goto oops;
	}
	
	// session initialize
	status = session_create(&aj_bus, &session_port_listener, &opts, session_port);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Session Initialize Failed\n");
		goto oops;
	}
	
	// advertise name
	status = advertise_name(&aj_bus, &opts);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Advertise Name Failed\n");
		goto oops;
	}
	
	if ( ER_OK == status ) {
        while (g_interrupt == QCC_FALSE) {
			printf(".\n");
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(10);
#endif
        }
    }
	
oops:
	program_uninitialize(&aj_bus,
						 &busListener,
						 &bus_object,
						 &interface,
						 &opts,
						 &session_port_listener);

    return (int) status;
}
