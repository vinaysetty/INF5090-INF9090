#include "libtramp.h"

void* tramp_initialize(char *label, size_t size) {
	char *shm = NULL;
	int shmid = -1;
	int shmflg = 0;
	key_t key = strtol(label, NULL, 36);

	#ifdef DEBUG
		printf("Initializing label '%s' of size '%lu'. Key is '%x'.\n", label, size, key);
	#endif

	// Locate the data segment if locally available or create it if it is not
	shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);
/*
        printf("shmid %s \n", strerror(errno));
*/
	if(shmid < 0) {
		shmid = shmget(key, size, 0 | SHM_R);

		//shmflg = SHM_RDONLY;

		#ifdef DEBUG
			printf("The segment is already present on this device with id '%d'.\n", shmid);
		#endif
	} else {
		#ifdef DEBUG
			printf("The segment was not present on this device. Created it with id '%d'.\n", shmid);
		#endif
	}

	if(shmid < 0) {
		fprintf(stderr, "Could not create data segment!\n");
		exit(EXIT_FAILURE);
	}

	// TODO: FIXME: Check if the label is published from other devices and set shmflg = SHM_RDONLY; if so
	// Setting read-only on existing labels should be done to prevent "double writes".. Only meant as protection.

	shm = (char *) shmat(shmid, NULL, shmflg);
	if(shm == (char *) -1) {
		fprintf(stderr, "Could not attach data segment!\n");
		exit(EXIT_FAILURE);
	}

	#ifdef DEBUG
		printf("The segment with label '%s' located at '%p' with id '%d' is attached with flag '%d'.\n", label, shm, shmid, shmflg);
	#endif

	return shm;
}

void tramp_publish(char* label, size_t size) {
	DBusMessage* msg;
	DBusMessageIter args;
	DBusConnection* connection;
	DBusError error;
	DBusPendingCall* pending;
	dbus_uint32_t community_members = 0;

	#ifdef DEBUG
		printf("Publishing label '%s' to the community.\n", label);
	#endif

	// Initialize the errors
	dbus_error_init(&error);

	// Connect to the session bus and check for errors
	connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if(dbus_error_is_set(&error)) { 
		fprintf(stderr, "DBus Connection Error (%s)\n", error.message);
		dbus_error_free(&error);
	}

	if (NULL == connection) {
		fprintf(stderr, "DBus has no connection!\n");
		exit(EXIT_FAILURE);
	}

	// Request our name on the bus
	dbus_bus_request_name(connection, "test.method.caller", DBUS_NAME_FLAG_REPLACE_EXISTING , &error);
	if(dbus_error_is_set(&error)) {
		fprintf(stderr, "DBus Name Error (%s)\n", error.message);
		dbus_error_free(&error);
	}

	// Create a new method call and check for errors
	msg = dbus_message_new_method_call(TRAMP_DBUS_NAME, // target for the method call
												  "/test/method/Object", // object to call on
												  "test.method.Type", // interface to call on
												  "PUBLISH");// method name
	if(NULL == msg) { 
		fprintf(stderr, "Message Null\n");
		exit(EXIT_FAILURE);
	}

	// Append arguments
	dbus_message_iter_init_append(msg, &args);
	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &label)) {
		fprintf(stderr, "DBus Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}
	
	// Send message and get a handle for a reply
	if(!dbus_connection_send_with_reply(connection, msg, &pending, -1)) { // -1 is default timeout
		fprintf(stderr, "DBus Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	if(NULL == pending) { 
		fprintf(stderr, "DBus Pending Call is NULL\n");
		exit(EXIT_FAILURE);
	}

	dbus_connection_flush(connection);
	
	#ifdef DEBUG
		printf("RPC request sent to TRAMP daemon.\n");
	#endif
	
	// Free message
	dbus_message_unref(msg);
	
	// Block until we recieve a reply
	dbus_pending_call_block(pending);

	// Get the reply message
	msg = dbus_pending_call_steal_reply(pending);
	if(NULL == msg) {
		fprintf(stderr, "TRAMP daemon sent nothing back from publish request!\n");
		exit(EXIT_FAILURE);
	}

	// Free the pending message handle
	dbus_pending_call_unref(pending);

	// Read the parameters
	if(!dbus_message_iter_init(msg, &args)) {
		fprintf(stderr, "Message has not the number of messages sent as argument!\n");
	}

	if(DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args)) {
		fprintf(stderr, "Message argument is not a number!\n");
	} else {
		dbus_message_iter_get_basic(&args, &community_members);
	}

	#ifdef DEBUG
		printf("Got reply from TRAMP daemon. PUBLISH message sent to '%d' community members.\n", community_members);
	#endif
	
	// Free reply
	dbus_message_unref(msg);	
}


void tramp_get(char *label, size_t size) {
	DBusMessage* msg;
	DBusMessageIter args;
	DBusConnection* connection;
	DBusError error;
	DBusPendingCall* pending;
	dbus_uint32_t community_members = 0;

	#ifdef DEBUG
		printf("Getting data for label '%s' once.\n", label);
	#endif

	// Initialize the errors
	dbus_error_init(&error);

	// Connect to the session bus and check for errors
	connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if(dbus_error_is_set(&error)) { 
		fprintf(stderr, "DBus Connection Error (%s)\n", error.message);
		dbus_error_free(&error);
	}

	if(NULL == connection) { 
		exit(EXIT_FAILURE);
	}

	// Request our name on the bus
	dbus_bus_request_name(connection, "test.method.caller", DBUS_NAME_FLAG_REPLACE_EXISTING , &error);
	if(dbus_error_is_set(&error)) { 
		fprintf(stderr, "DBus Name Error (%s)\n", error.message);
		dbus_error_free(&error);
	}

	// Create a new method call and check for errors
	msg = dbus_message_new_method_call(TRAMP_DBUS_NAME, // target for the method call
												  "/test/method/Object", // object to call on
												  "test.method.Type", // interface to call on
												  "GET");// method name
	if(NULL == msg) { 
		fprintf(stderr, "Message Null\n");
		exit(EXIT_FAILURE);
	}

	// Append arguments
	dbus_message_iter_init_append(msg, &args);
	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &label)) {
		fprintf(stderr, "DBus Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT64, &size)) {
		fprintf(stderr, "DBus Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}
	
	// Send message and get a handle for a reply
	if(!dbus_connection_send_with_reply(connection, msg, &pending, -1)) { // -1 is default timeout
		fprintf(stderr, "DBus Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	if(NULL == pending) { 
		fprintf(stderr, "DBus Pending Call is NULL\n");
		exit(EXIT_FAILURE);
	}

	dbus_connection_flush(connection);
	
	#ifdef DEBUG
		printf("RPC request (GET) sent to TRAMP daemon.\n");
	#endif
	
	// Free message
	dbus_message_unref(msg);
	
	// Block until we recieve a reply
	dbus_pending_call_block(pending);

	// Get the reply message
	msg = dbus_pending_call_steal_reply(pending);
	if (NULL == msg) {
		fprintf(stderr, "TRAMP daemon sent nothing back from get request!\n");
		exit(EXIT_FAILURE);
	}

	// Free the pending message handle
	dbus_pending_call_unref(pending);

	// Read the parameters
	if(!dbus_message_iter_init(msg, &args)) {
		fprintf(stderr, "Message has not the number of messages sent as argument!\n");
	}

	if(DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args)) {
		fprintf(stderr, "Message argument is not a number!\n");
	} else {
		dbus_message_iter_get_basic(&args, &community_members);
	}

	#ifdef DEBUG
		printf("Got reply from TRAMP daemon. GET message sent to '%d' community members.\n", community_members);
	#endif
	
	// Free reply
	dbus_message_unref(msg);
}

void tramp_subscribe(char *label, size_t size) {
	DBusMessage* msg;
	DBusMessageIter args;
	DBusConnection* connection;
	DBusError error;
	DBusPendingCall* pending;
	dbus_uint32_t community_members = 0;

	#ifdef DEBUG
		printf("Subscribing to data for label '%s'.\n", label);
	#endif

	// Initialize the errors
	dbus_error_init(&error);

	// Connect to the session bus and check for errors
	connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if(dbus_error_is_set(&error)) { 
		fprintf(stderr, "DBus Connection Error (%s)\n", error.message);
		dbus_error_free(&error);
	}

	if (NULL == connection) { 
		exit(EXIT_FAILURE);
	}

	// Request our name on the bus
	dbus_bus_request_name(connection, "test.method.caller", DBUS_NAME_FLAG_REPLACE_EXISTING , &error);
	if(dbus_error_is_set(&error)) { 
		fprintf(stderr, "DBus Name Error (%s)\n", error.message);
		dbus_error_free(&error);
	}

	// Create a new method call and check for errors
	msg = dbus_message_new_method_call(TRAMP_DBUS_NAME, // target for the method call
												  "/test/method/Object", // object to call on
												  "test.method.Type", // interface to call on
												  "SUBSCRIBE");// method name
	if(NULL == msg) { 
		fprintf(stderr, "Message Null\n");
		exit(EXIT_FAILURE);
	}

	// Append arguments
	dbus_message_iter_init_append(msg, &args);
	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &label)) {
		fprintf(stderr, "DBus Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT64, &size)) {
		fprintf(stderr, "DBus Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}
	
	// Send message and get a handle for a reply
	if(!dbus_connection_send_with_reply(connection, msg, &pending, -1)) { // -1 is default timeout
		fprintf(stderr, "DBus Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	if(NULL == pending) { 
		fprintf(stderr, "DBus Pending Call is NULL\n");
		exit(EXIT_FAILURE);
	}

	dbus_connection_flush(connection);
	
	#ifdef DEBUG
		printf("RPC request (SUBSCRIBE) sent to TRAMP daemon.\n");
	#endif
	
	// Free message
	dbus_message_unref(msg);
	
	// Block until we recieve a reply
	dbus_pending_call_block(pending);

	// Get the reply message
	msg = dbus_pending_call_steal_reply(pending);
	if(NULL == msg) {
		fprintf(stderr, "TRAMP daemon sent nothing back from subscribe request!\n");
		exit(EXIT_FAILURE);
	}

	// Free the pending message handle
	dbus_pending_call_unref(pending);

	// Read the parameters
	if(!dbus_message_iter_init(msg, &args)) {
		fprintf(stderr, "Message has not the number of messages sent as argument!\n");
	}

	if(DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args)) {
		fprintf(stderr, "Message argument is not a number!\n");
	} else {
		dbus_message_iter_get_basic(&args, &community_members);
	}

	#ifdef DEBUG
		printf("Got reply from TRAMP daemon. SUB message sent to '%d' community members.\n", community_members);
	#endif
	
	// Free reply
	dbus_message_unref(msg);	

	// Publish label, informing community that this node now can provide data for this label
	tramp_publish(label, size);
}
