/**
 * @file
 * @brief  Sample implementation of an AllJoyn client in C.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2013, AllSeen Alliance. All rights reserved.
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
#include <stdlib.h>
#include <string.h>

#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/BusObject.h>
#include <alljoyn_c/MsgArg.h>
#include <alljoyn_c/InterfaceDescription.h>
#include <alljoyn_c/version.h>

#include <alljoyn_c/Status.h>
#include <unistd.h>

#define APP_NAME "door_app_cli"
#define SIG_NAME "door_signal"
#define SIG_SIGN "i"

/*constants*/
static const char* INTERFACE_NAME = "com.BandRich.signal";
static const char* OBJECT_NAME = "com.BandRich.signal";
static const char* OBJECT_PATH = "/door";
static const char* CONNECTSPEC = "unix:abstract=alljoyn";
static const alljoyn_sessionport SERVICE_PORT = 1024;

static QCC_BOOL g_found = QCC_FALSE;
static volatile sig_atomic_t g_interrupt = QCC_FALSE;

static void SigIntHandler(int sig)
{
    g_interrupt = QCC_TRUE;
}

/* FoundAdvertisedName callback */
void found_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix)
{
    printf("found_advertised_name(name=%s, prefix=%s)\n", name, namePrefix);
	
	if (name && strcmp(name, INTERFACE_NAME) == 0) {
        g_found = QCC_TRUE;
    }
}

/* LostAdvertisedName callback */
void lost_advertised_name(const void *context, const char *name, alljoyn_transportmask transport, const char *namePrefix)
{
    printf("lost_advertised_name(name=%s, prefix=%s)\n", name, namePrefix);
	
	if (name && strcmp(name, INTERFACE_NAME) == 0) {
        g_found = QCC_FALSE;
    }
}

/* ObjectRegistered callback */
void busobject_object_registered(const void* context)
{
    printf("ObjectRegistered has been called\n");
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
        printf("alljoyn_busattachment started.\n");
		status = alljoyn_busattachment_connect(*bus, CONNECTSPEC);
        if (ER_OK != status) {
            printf("alljoyn_busattachment_connect(\"%s\") failed, reason %s\n", CONNECTSPEC, QCC_StatusText(status));
        } else {
            printf("alljoyn_busattachment connected to \"%s\"\n", alljoyn_busattachment_getconnectspec(*bus));
        }
    } else {
		printf("alljoyn_busattachment_start Fail reason is %s\n", QCC_StatusText(status));
    }
	
	return status;
}

QStatus bus_register(alljoyn_busattachment *bus, alljoyn_buslistener *busListener)
{
    /* Create a bus listener */
    alljoyn_buslistener_callbacks callbacks = {
		NULL,
		NULL,
		&found_advertised_name,
		&lost_advertised_name,
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
        status = alljoyn_interfacedescription_addsignal(*iface,
													    SIG_NAME,
													    SIG_SIGN,
													    "state",
													    0,
													    NULL);
        alljoyn_interfacedescription_activate(*iface);
        printf("Interface Created.\n");
    } else {
        printf("Failed to create interface, reason is '%s'\n", QCC_StatusText(status));
	}
	return status;
}

QStatus find_advertise_name(alljoyn_busattachment *bus)
{
	QStatus status;
	/* Begin discovery on the well-known name of the service to be called */
	status = alljoyn_busattachment_findadvertisedname(*bus, OBJECT_NAME);
    if (status != ER_OK) {
        printf("alljoyn_busattachment_findadvertisedname failed (%s))\n", QCC_StatusText(status));
    }
	return status;
}

void program_uninitialize(alljoyn_busattachment 		*bus,
						  alljoyn_buslistener 			*busListener,
						  alljoyn_busobject 			*bus_object,
						  alljoyn_interfacedescription 	*iface)
						  // alljoyn_sessionopts 			*opts,
						  // alljoyn_sessionportlistener 	*spl
{
	/* Deallocate sessionopts */
    //if (*opts) {
    //    alljoyn_sessionopts_destroy(*opts);
    //}
	
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

#if 0
    /* Deallocate session port listener */
    if (*spl) {
        alljoyn_sessionportlistener_destroy(*spl);
    }
#endif

#if 1
	/* Deallocate the bus object */
    if (*bus_object) {
        alljoyn_busobject_destroy(*bus_object);
    }
#endif
}

QStatus bus_object_init(alljoyn_busattachment *bus, alljoyn_busobject *busObject, alljoyn_interfacedescription *interface)
{
	QStatus status = ER_FAIL;
	alljoyn_busobject_callbacks busObjCbs = {
        NULL,
        NULL,
        &busobject_object_registered,
        NULL
    };
	
	/* Set up bus object */
    *busObject = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
    *interface = alljoyn_busattachment_getinterface(*bus, INTERFACE_NAME);

	status = alljoyn_busobject_addinterface(*busObject, *interface);
	if (ER_OK != status) {
		printf("alljoyn_busobject_addinterface Fail reason is %s\n", QCC_StatusText(status));
	}
	
	status = alljoyn_busattachment_registerbusobject(*bus, *busObject);	
	if (ER_OK != status) {
		printf("alljoyn_busattachment_registerbusobject Fail reason is %s\n", QCC_StatusText(status));
	}
	return status;
}

QStatus emit_signal(alljoyn_busattachment *bus, alljoyn_busobject *bus_object, const int state)
{
	QStatus status;
	size_t sz = 1;
	alljoyn_msgarg args;
	
	alljoyn_interfacedescription_member member;
    alljoyn_interfacedescription interface;
	interface = alljoyn_busattachment_getinterface(*bus, INTERFACE_NAME);
    alljoyn_interfacedescription_getmember(interface, SIG_NAME, &member);
	
	args = alljoyn_msgarg_array_create(sz);
    status = alljoyn_msgarg_array_set(args, &sz, SIG_SIGN, state);

    if (ER_OK == status) {
		status = alljoyn_busobject_signal(*bus_object,
										  NULL,
										  0,
										  member,
										  args,
										  sz,
										  0,
										  ALLJOYN_MESSAGE_FLAG_SESSIONLESS,
										  NULL);
		printf("alljoyn_busobject_signal Fail reason is %s\n", QCC_StatusText(status));
    }
    alljoyn_msgarg_destroy(args);
	return status;
}

/** Main entry point */
/** TODO: Make this C89 compatible. */
int main(int argc, char** argv, char** envArg)
{
	int dummy_state=0;
	QStatus status = ER_OK;
	alljoyn_busattachment aj_bus = NULL;
    alljoyn_interfacedescription interface = NULL;
    alljoyn_buslistener busListener = NULL;
	alljoyn_busobject bus_object = NULL;
	
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
	
	// register bus
	status = bus_register(&aj_bus, &busListener);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Bus Register Failed\n");
		goto oops;
	}
	
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

	// bus object init
	status = bus_object_init(&aj_bus, &bus_object, &interface);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Object Init Failed\n");
		goto oops;
	}

	status = find_advertise_name(&aj_bus);
	if ( ER_FAIL == status ) {
		printf("[ERROR] Find Advertise Failed\n");
		goto oops;
	}
	
	/* Wait for join session to complete */
    while (g_interrupt == QCC_FALSE) {
		if (g_found == QCC_TRUE) {
			printf("emit_signal\n");
			status = emit_signal(&aj_bus, &bus_object, dummy_state);
			
			// the state should be read from arduino (TBD)
			dummy_state = (dummy_state==0) ? 1 : 0;
		}
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif		
    }
	
oops:
	program_uninitialize(&aj_bus,
						 &busListener,
						 &bus_object,
						 &interface
						 );

    return (int) status;
}
