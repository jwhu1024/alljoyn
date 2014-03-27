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

/** Static top level message bus object */
static alljoyn_busattachment g_msgBus = NULL;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.Bus.sample";
static const char* OBJECT_NAME = "org.alljoyn.Bus.sample";
static const char* OBJECT_PATH = "/sample";
static const alljoyn_sessionport SERVICE_PORT = 25;

static QCC_BOOL s_joinComplete = QCC_FALSE;
static alljoyn_sessionid s_sessionId = 0;

/* Static BusListener */
static alljoyn_buslistener g_busListener;

static volatile sig_atomic_t g_interrupt = QCC_FALSE;

#if 1 // lester_hu@bandrich.com
static void SigIntHandler(int sig)
{
    g_interrupt = QCC_TRUE;
}

/* FoundAdvertisedName callback */
void found_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix)
{
	printf("[INFO] found_advertised_name: name=%s\n", name);
	printf("[INFO] found_advertised_name: namePrefix=%s\n", namePrefix);
    if (0 == strcmp(name, OBJECT_NAME)) {
        /* We found a remote bus that is advertising basic service's  well-known name so connect to it */
        alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
        QStatus status;
        /* enable concurrent callbacks so joinsession can be called */
        alljoyn_busattachment_enableconcurrentcallbacks(g_msgBus);
        status = alljoyn_busattachment_joinsession(g_msgBus, name, SERVICE_PORT, NULL, &s_sessionId, opts);

        if (ER_OK != status) {
            printf("alljoyn_busattachment_joinsession failed (status=%s)\n", QCC_StatusText(status));
        }
#ifdef _DEBUG_
		else {
            printf("alljoyn_busattachment_joinsession SUCCESS (Session id=%d)\n", s_sessionId);
        }
#endif
        alljoyn_sessionopts_destroy(opts);
    }
    s_joinComplete = QCC_TRUE;
}

/* NameOwnerChanged callback */
void name_owner_changed(const void* context, const char* busName, const char* previousOwner, const char* newOwner)
{
    if (newOwner && (0 == strcmp(busName, OBJECT_NAME))) {
		printf("[INFO] name_owner_changed: name=%s\n", busName);
		printf("[INFO] name_owner_changed: noldOwner=%s\n", previousOwner ? previousOwner : "<none>");
		printf("[INFO] name_owner_changed: nnewOwner=%s\n", newOwner ? newOwner : "<none>");
    }
}

void int2str(int i, char *s) {
	sprintf(s, "%d", i);
}

/** Main entry point */
/** TODO: Make this C89 compatible. */
int main(int argc, char** argv, char** envArg)
{
    QStatus status = ER_OK;
    char* connectArgs = "unix:abstract=alljoyn";
    alljoyn_interfacedescription testIntf = NULL;
    /* Create a bus listener */
    alljoyn_buslistener_callbacks callbacks = {
        NULL,
        NULL,
        &found_advertised_name,
        NULL,
        &name_owner_changed,
        NULL,
        NULL,
        NULL
    };

#if 1 //def _DEBUG_
	printf("------------------------------\n");
#endif

    printf("AllJoyn Library version: %s\n", alljoyn_getversion());
    printf("AllJoyn Library build info: %s\n", alljoyn_getbuildinfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Create message bus */
    g_msgBus = alljoyn_busattachment_create("myApp", QCC_TRUE);

    /* Add org.alljoyn.Bus.method_sample interface */
    status = alljoyn_busattachment_createinterface(g_msgBus, INTERFACE_NAME, &testIntf);
    if (status == ER_OK) {
#ifdef _DEBUG_
        printf("Interface Created.\n");
#endif
        alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "add", "ss",  "s", "inStr1,inStr2,outStr", 0);
        alljoyn_interfacedescription_activate(testIntf);
    } else {
        printf("Failed to create interface 'org.alljoyn.Bus.method_sample'\n");
    }


    /* Start the msg bus */
    if (ER_OK == status) {
        status = alljoyn_busattachment_start(g_msgBus);
        if (ER_OK != status) {
            printf("alljoyn_busattachment_start failed\n");
        }
#ifdef _DEBUG_
		else {
            printf("alljoyn_busattachment started.\n");
        }
#endif
    }

    /* Connect to the bus */
    if (ER_OK == status) {
        status = alljoyn_busattachment_connect(g_msgBus, connectArgs);
        if (ER_OK != status) {
            printf("alljoyn_busattachment_connect(\"%s\") failed\n", connectArgs);
        }
#ifdef _DEBUG_
		else {
            printf("alljoyn_busattachment connected to \"%s\"\n", alljoyn_busattachment_getconnectspec(g_msgBus));
        }
#endif
    }

    g_busListener = alljoyn_buslistener_create(&callbacks, NULL);

    /* Register a bus listener in order to get discovery indications */
    if (ER_OK == status) {
        alljoyn_busattachment_registerbuslistener(g_msgBus, g_busListener);
#ifdef _DEBUG_
        printf("alljoyn_buslistener Registered.\n");
#endif
    }

    /* Begin discovery on the well-known name of the service to be called */
    if (ER_OK == status) {
        status = alljoyn_busattachment_findadvertisedname(g_msgBus, OBJECT_NAME);
        if (status != ER_OK) {
            printf("alljoyn_busattachment_findadvertisedname failed (%s))\n", QCC_StatusText(status));
        }
    }

    /* Wait for join session to complete */
    while (s_joinComplete == QCC_FALSE && g_interrupt == QCC_FALSE) {
#ifdef _WIN32
        Sleep(10);
#else
        usleep(100 * 1000);
#endif
    }

    if (status == ER_OK && g_interrupt == QCC_FALSE) {
		alljoyn_message reply;
		alljoyn_msgarg inputs;
		size_t numArgs;
		int num1=33;
		int num2=67;
		char tmp_str1[5]={0};
		char tmp_str2[5]={0};
		
		alljoyn_proxybusobject remoteObj = alljoyn_proxybusobject_create(g_msgBus, OBJECT_NAME, OBJECT_PATH, s_sessionId);
		const alljoyn_interfacedescription alljoynTestIntf = alljoyn_busattachment_getinterface(g_msgBus, INTERFACE_NAME);
		assert(alljoynTestIntf);
		alljoyn_proxybusobject_addinterface(remoteObj, alljoynTestIntf);

		reply = alljoyn_message_create(g_msgBus);
		inputs = alljoyn_msgarg_array_create(2);
		numArgs = 2;

		int2str(num1, tmp_str1);
		int2str(num2, tmp_str2);
		
#ifdef _DEBUG_
		printf("tmp_str1 = %s\n", tmp_str1);
		printf("tmp_str2 = %s\n", tmp_str2);
#endif
		
		status = alljoyn_msgarg_array_set(inputs, &numArgs, "ss", tmp_str1, tmp_str2);
		if (ER_OK != status) {
			printf("Arg assignment failed: %s\n", QCC_StatusText(status));
		}
		status = alljoyn_proxybusobject_methodcall(remoteObj, INTERFACE_NAME, "add", inputs, 2, reply, 5000, 0);
		if (ER_OK == status) {
			char* ret_str;
			status = alljoyn_msgarg_get(alljoyn_message_getarg(reply, 0), "s", &ret_str);
#if 1 //def _DEBUG_
			printf("------------------------------\n");
#endif
			printf("%s.%s returned \"%s\"\n", INTERFACE_NAME, "add", ret_str);
			printf("[RESULT] %d + %d = %d\n", num1, num2, atoi(ret_str));
#if 1 //def _DEBUG_
			printf("------------------------------\n");
#endif
		} else {
			printf("MethodCall on %s.%s failed\n", INTERFACE_NAME, "add");
		}
		
		alljoyn_proxybusobject_destroy(remoteObj);
		alljoyn_message_destroy(reply);
		alljoyn_msgarg_destroy(inputs);
    }

    /* Deallocate bus */
    if (g_msgBus) {
        alljoyn_busattachment deleteMe = g_msgBus;
        g_msgBus = NULL;
        alljoyn_busattachment_destroy(deleteMe);
    }

    /* Deallocate bus listener */
    alljoyn_buslistener_destroy(g_busListener);
    printf("basic client exiting with status %d (%s)\n", status, QCC_StatusText(status));
	
#ifdef _WIN32
    //system("PAUSE");
#endif

    return (int) status;
}

#else
int main()
{
	printf("Hello, world!\n");
	return 1;
}
#endif
