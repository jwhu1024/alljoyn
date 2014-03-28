/*
Date   : 2014/03/28
Author : lester_hu@bandrich.com
*/

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

/* top level object responsible for connecting to and managing an AllJoyn message bus */
alljoyn_busattachment g_msgBus;

/*
consists of a collection of function pointers implemented by AllJoyn users and called 
by AllJoyn to inform users of bus related events.
*/
alljoyn_buslistener g_busListener;

/*
an object for describing message bus interfaces
*/
alljoyn_interfacedescription g_iface;

static const char* INTERFACE_NAME = "com.bandrich.Bus.sample";
static const char* OBJECT_NAME = "com.bandrich.Bus.sample";
static const char* OBJECT_PATH = "/sample";
static volatile sig_atomic_t g_interrupt = QCC_FALSE;

static QCC_BOOL s_found = QCC_FALSE;

static void SigIntHandler(int sig)
{
    g_interrupt = QCC_TRUE;
}

static char * convert_transport(alljoyn_transportmask transport)
{
	char *transport_str = (char*) malloc (sizeof(char) * 32);
	
	switch (transport & 0xFFFF)
	{
		case 0x0000:
			sprintf(transport_str, "NONE");
			break;
		case 0x0001:
			sprintf(transport_str, "LOCAL");
			break;
		case 0x0002:
			sprintf(transport_str, "BLUETOOTH");
			break;
		case 0xFFFF:
			sprintf(transport_str, "ANY");
			break;
		default:
			break;
	}
	return transport_str;
}

void found_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix)
{
#ifdef DEBUG
	char *transport_cov = convert_transport(transport);
	printf("found_advertised_name - name %s\n", name);
	printf("found_advertised_name - namePrefix %s\n", namePrefix);
	// printf("found_advertised_name - transport 0x%04X", transport);
	printf("found_advertised_name - transport %s\n", transport_cov);
	
	if (transport_cov)	free(transport_cov);
#endif
	s_found = QCC_TRUE;
	return;
}

/* ObjectRegistered callback */
void busobject_object_registered(const void* context)
{
#ifdef DEBUG
    printf("ObjectRegistered has been called\n");
#endif
}

int main(int argc, char** argv, char** envArg)
{
	printf("AllJoyn Library version: %s\n", alljoyn_getversion());
	printf("AllJoyn Library build info: %s\n", alljoyn_getbuildinfo());
	
	char* connectArgs = "unix:abstract=alljoyn";
	QStatus status = ER_FAIL;

	/* Install SIGINT handler */
	signal(SIGINT, SigIntHandler);

	/* Struct containing callbacks used for creation of an alljoyn_buslistener. */
	alljoyn_buslistener_callbacks aj_callback=
	{
		NULL,						// listener_registered
		NULL,						// listener_unregistered
		// NULL,					// found_advertised_name
		&found_advertised_name,		// found_advertised_name
		NULL,						// lost_advertised_name
		NULL,						// name_owner_changed
		NULL,						// bus_stopping
		NULL,						// bus_disconnected
		NULL						// property_changed
	};

	// 1. Create a BusAttachment
	g_msgBus = alljoyn_busattachment_create("myApp", QCC_TRUE);

	// 2. Create and Register Bus Listener
	g_busListener = alljoyn_buslistener_create(&aj_callback, NULL);
	alljoyn_busattachment_registerbuslistener(g_msgBus, g_busListener);

	// 3. Create a BusInterterface
	status = alljoyn_busattachment_createinterface(g_msgBus, INTERFACE_NAME, &g_iface);

	/* Add interface */
	if (status == ER_OK) {
		alljoyn_interfacedescription_addmember(g_iface,
											   ALLJOYN_MESSAGE_METHOD_CALL,
											   "cat",
											   "ss",
											   "s",
											   "inStr1,inStr2,outStr",
											   0);
		alljoyn_interfacedescription_activate(g_iface);
	}
	else
	{
#ifdef DEBUG
		printf("[INFO] Failed to create interface 'org.alljoyn.Bus.method_sample'\n");
#endif
		goto oops;
	}

#ifdef DEBUG
	printf("[INFO] Bus Interface Created\n"); 
#endif

	// Start the bus
	status = alljoyn_busattachment_start(g_msgBus);
	if (ER_OK != status)
	{
		printf("[INFO] Bus Cannot Start\n");
		goto oops;
	}
	printf("[INFO] Bus Started\n");

	// Connect to Bus
	status = alljoyn_busattachment_connect(g_msgBus, connectArgs);
	if (ER_OK != status)
	{
		printf("[INFO] Bus Connect Failed\n");
		goto oops;
	}

	printf("[INFO] Start to find advertised name\n");
	status = alljoyn_busattachment_findadvertisedname(g_msgBus, OBJECT_NAME);
#ifdef DEBUG
	if (status != ER_OK)
	{
    	printf("alljoyn_busattachment_findadvertisedname failed (%s))\n", QCC_StatusText(status));
		goto oops;
	}
#endif

	while (status == ER_OK && g_interrupt == QCC_FALSE)
	{
		usleep(100 * 1000);
	}

oops:
	/* Deallocate bus */
	if (g_msgBus)
	{
		alljoyn_busattachment deleteMe = g_msgBus;
		g_msgBus = NULL;
		alljoyn_busattachment_destroy(deleteMe);
	}
	return 1;
}