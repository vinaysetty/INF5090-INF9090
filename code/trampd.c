#include "trampd.h"
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
static void daemonize(void) {
	//TODO: FIXME: Remove this return in production
	return;

	pid_t pid, sid;

	// Check if already a daemon
	if(getppid() == 1) {
		return;
	}

	// Fork off the parent process
	pid = fork();
	if(pid < 0) {
		exit(EXIT_FAILURE);
	}

	// If we got a good PID, then we can exit the parent process.
	if(pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// At this point we are executing as the child process

	// Change the file mode mask
	umask(0);

	// Create a new SID for the child process
	sid = setsid();
	if(sid < 0) {
		exit(EXIT_FAILURE);
	}

	// Change the current working directory
	// This prevents the current directory from being locked
	if((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	// Redirect standard files to /dev/null
	freopen( "/dev/null", "r", stdin);
	freopen( "/dev/null", "w", stdout);
	freopen( "/dev/null", "w", stderr);
}

int send_message(int socket, char *message) {
	int i = -1;
	size_t len = -1;
	len = strlen(message);
	int counter = -1;

	if(message == NULL || len <= 0) {
		fprintf(stderr, "No message to send!\n");
		return 0;
	}


	#ifdef DEBUG
		printf("Sending '%s' of '%lu' bytes.\n", message, len);
	#endif

	if(socket == 0) {
		#ifdef DEBUG
			printf("Sending to entire community.\n");
		#endif

		for(i = 0; i <= peers; i++) {
			send(sockets[i], message, len, 0);
			counter++;
		}
	} else {
		#ifdef DEBUG
			printf("Sending to socket '%d'.\n", socket);
		#endif

		send(socket, message, len, 0);
		counter = 1;
	}

	return counter;
}

int send_file(int socket, char* filename) {
	FILE *fd;
	struct stat s;
	fd = open(filename, O_RDONLY);
	fstat(fd, &s); // get the size
	void* adr = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0); // get the address
	write(socket, adr, s.st_size); // send the file from this address directly
	return -1;
}

void *server_listen() {
	printf("Listening for server calls.\n");

	// Setting up server socket
	int sockfd = -1;
	int client_sockfd = -1;
	int optval = 1;
	int ret = -1;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_size = 0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error initializing server socket!\n");
		exit(EXIT_FAILURE);
	}

	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if(ret == -1) {
		fprintf(stderr, "Error setting re-use socket option!\n");
		exit(EXIT_FAILURE);
	}

	ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
	if(ret == -1) {
		fprintf(stderr, "Error setting keep-alive socket option!\n");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	memset(&(server_addr.sin_zero), 0, 8);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if(ret == -1) {
		fprintf(stderr, "Error binding to socket.\n");
		exit(EXIT_FAILURE);
	}

	ret = listen(sockfd, 10);
	if(ret == -1) {
		fprintf(stderr, "Error listening to socket.\n");
		exit(EXIT_FAILURE);
	}

	addr_size = sizeof(struct sockaddr_in);

	// Wait for incoming messages
	#ifdef DEBUG
		printf("%c[%d;%dm", 27, 1, 34);
		printf("Waiting for connections...");
		printf("%c[%dm\n", 27, 0);
	#endif
	for(EVER) {
		client_sockfd = accept(sockfd, (struct sockaddr *) &client_addr, &addr_size);
		if(client_sockfd != -1) {
			printf("Received connection from %s\n", inet_ntoa(client_addr.sin_addr));

			#ifdef DEBUG
				printf("Socket FD '%d'.\n", client_sockfd);
			#endif	

			int ret = pthread_create(&peer_threads[peers++], NULL, connection, &client_sockfd);
			if(ret != 0) {
				fprintf(stderr, "Could not create peer thread!\n");
			}

		} else {
			fprintf(stderr, "Error accepting connection!\n");
		}
	}
}

void *connection(void *socket) {
	int sockfd = * (int *) socket;

	sockets[peers - 1] = sockfd;

	char buffer[1400];
	int buffer_len = 1400;
	int bytecount = -1;
	char *result = NULL;
	int error_count = 0;

	for(EVER) {
		memset(buffer, 0, buffer_len);

		bytecount = recv(sockfd, buffer, buffer_len, 0);
		if(bytecount <= 0) {
			fprintf(stderr, "Error receiving data on socket '%d'. Message lost!\n", sockfd);

			error_count++;
			if(error_count >= 3) {
				fprintf(stderr, "Lost %d messages on socket '%d'. Disconnecting!\n", error_count, sockfd);
				return 0;
			}

			usleep(1000);
			continue;
		}

		#ifdef DEBUG
			printf("Received message '%s' of '%d' bytes on socket '%d'.\n", buffer, bytecount, sockfd);
		#endif

		result = strtok(buffer, ";");

		if(strcmp(result, "PUB") == 0) {
			result = strtok(NULL, ";");
			#ifdef DEBUG
				printf("Incoming PUB on socket '%d'. Store mapping to label '%s'.\n", sockfd, result);
			#endif
			handle_pub_message(sockfd, result);
			continue;
		}

		if(strcmp(result, "GET") == 0) {
			result = strtok(NULL, " ");
			#ifdef DEBUG
				printf("Incoming GET on socket '%d'. Parsing query '%s'.\n", sockfd, result);
			#endif
			handle_get_message(sockfd, result);
			continue;
		}

		if(strcmp(result, "SUB") == 0) {
			result = strtok(NULL, " ");
			#ifdef DEBUG
				printf("Incoming SUB on socket '%d'. Parsing query '%s'.\n", sockfd, result);
			#endif
			//TODO: FIXME: Must SUB be handeled different from GET?
			handle_get_message(sockfd, result);
			continue;
		}

		if(strcmp(result, "YEP") == 0) {
			result = strtok(NULL, " ");
			#ifdef DEBUG
				printf("Incoming YEP on socket '%d'. Result is '%s'.\n", sockfd, result);
			#endif

			handle_yep_message(sockfd, result);
			continue;
		}

		if(strcmp(result, "FET") == 0) {
			result = strtok(NULL, " ");
			#ifdef DEBUG
				printf("Incoming FET on socket '%d'. Result is '%s'.\n", sockfd, result);
			#endif

			handle_fet_message(sockfd, result);
			continue;
		}

		if(strcmp(result, "FETC") == 0) {
			result = strtok(NULL, " ");
			#ifdef DEBUG
				printf("Incoming FETC on socket '%d'. Result is '%s'.\n", sockfd, result);
			#endif

			handle_fetc_message(sockfd, result);
			continue;
		}

		if(strcmp(result, "DAT") == 0) {
			result = strtok(NULL, "");
			#ifdef DEBUG
				printf("Incoming DAT on socket '%d'. Result is '%s'.\n", sockfd, result);
			#endif

			handle_dat_message(sockfd, result);
			continue;
		}

		fprintf(stderr, "Unknown message '%s'!\n", buffer);
	}

	pthread_exit(NULL);
}

void *data(void *message) {
	#ifdef DEBUG
		printf("Message is '%s'.\n", (char *) message);
	#endif

	char *shm = NULL;
	char* label = NULL;
	int socket = -1;
	int shmid = -1;
	int shmflg = 0;
	size_t size = -1;

	label = strdup(strtok(message, ";"));
	size = (size_t) atoi(strtok(NULL, ";"));
	socket = (int) atoi(strtok(NULL, ";"));
	key_t key = strtol(label, NULL, 36);

	// Locate the data segment
	shmid = shmget(key, size, SHM_R);
	if(shmid < 0) {
		fprintf(stderr, "The data segment for label '%s' is not present on this device. It is very strange that this guy request this data from us!\n", label);
		return 0;
	}

	shm = (char *) shmat(shmid, NULL, shmflg);
	if(shm == (char *) -1) {
		fprintf(stderr, "Could not attach data segment!\n");
		return 0;
	}

	#ifdef DEBUG
		printf("This device has contents for '%s' with key '%x' of size '%lu'. Sending the data to socket '%d'.\n", label, key, size, socket);
	#endif

	char *reply = (char *) malloc(MSG_SIZE + size);

	// Find length of header
	// TODO: FIXME: Do this smarter
	sprintf(reply, "DAT;%s;%lu;", label, size);
	int length_of_header = strlen(reply);
	printf("Length of header: '%d'\n", length_of_header);

	for(EVER) {

		if(memcmp(shm, reply + length_of_header, strlen(shm)) != 0) {
			bzero(reply, MSG_SIZE);
			sprintf(reply, "DAT;%s;%lu;%s", label, size, shm);
			send_message(socket, reply);
		}

		usleep(500);
	}

	free(reply);

	return 0;
}

void handle_dat_message(int socket, char *message) {
	char *shm = NULL;
	char* label = NULL;
	void *data = NULL;
	int shmid = -1;
	int shmflg = 0;
	size_t size = -1;

	#ifdef DEBUG
		printf("Message is '%s'.\n", message);
	#endif

	label = strtok(message, ";");
	size = (size_t) atoi(strtok(NULL, ";"));
	key_t key = strtol(label, NULL, 36);
	data = strtok(NULL, ";");

	#ifdef DEBUG
		printf("Label='%s' Size='%lu' Key='%x' Data='%s'.\n", label, size, key, (char *) data);
	#endif

	if(data == NULL) {
		#ifdef DEBUG
			printf("No data. Skipping!\n");
		#endif
		return;
	}

	// Locate the data segment
	shmid = shmget(key, size, SHM_W);
	if(shmid < 0) {
		fprintf(stderr, "The data segment for label '%s' is not present on this device. It is very strange that this guy request this data from us!\n", label);
		return;
	}

	shm = (char *) shmat(shmid, NULL, shmflg);
	if(shm == (char *) -1) {
		fprintf(stderr, "Could not attach data segment!\n");
		return;
	}

	sprintf(shm, "%s", (char *) data);

	#ifdef DEBUG
		printf("Contents for '%s' with key '%x' of size '%lu' is set.\n", label, key, size);
	#endif
}

void handle_fet_message(int socket, char *message) {
	char *shm = NULL;
	char* label = NULL;
	int shmid = -1;
	int shmflg = 0;
	size_t size = -1;

	#ifdef DEBUG
		printf("Message is '%s'.\n", message);
	#endif

	label = strtok(message, ";");
	size = (size_t) atoi(strtok(NULL, ";"));
	key_t key = strtol(label, NULL, 36);

	// Locate the data segment
	shmid = shmget(key, size, SHM_R);
	if(shmid < 0) {
		fprintf(stderr, "The data segment for label '%s' is not present on this device. It is very strange that this guy request this data from us!\n", label);
		return;
	}

	shm = (char *) shmat(shmid, NULL, shmflg);
	if(shm == (char *) -1) {
		fprintf(stderr, "Could not attach data segment!\n");
		return;
	}

	#ifdef DEBUG
		printf("This device has contents for '%s' with key '%x' of size '%lu'. Sending the data to socket '%d'.\n", label, key, size, socket);
	#endif

	char *reply = (char *) malloc(MSG_SIZE + size);
	bzero(reply, MSG_SIZE);
	sprintf(reply, "DAT;%s;%lu;%s", label, size, shm);
	send_message(socket, reply);
	free(reply);
}

void handle_fetc_message(int socket, char *message) {
	sprintf(message, "%s;%d", message, socket);

	int ret = pthread_create(&data_threads[num_data_threads++], NULL, data, message);
	if(ret != 0) {
		fprintf(stderr, "Could not create data thread!\n");
	}

	//TODO: FIXME: If we are too fast here, the message is lost. Not in own thread, so we are in a hurry!
	usleep(1000);
}

void handle_pub_message(int socket, char *message) {
	// Unknown / New label?
	if(label_index(message) == -1) {
		labels[total_labels++] = strdup(message);
	}

	// Debug
	print_labels();
}

void handle_yep_message(int socket, char *message) {
	char* label = NULL;
	int from_origin = 0;

	#ifdef DEBUG
		printf("Message is '%s'.\n", message);
	#endif

	label = strtok(message, ";");
	from_origin = (int) atoi(strtok(NULL, ";"));

	struct timeval timestamp;

	#ifdef DEBUG
		printf("Updaing delay for label '%s' on socket '%d'.\n", label, socket);
		printf("Socket '%d' is '%d' away from origin.\n", socket, from_origin);
	#endif

	gettimeofday(&timestamp, NULL);

	int l_index = label_index(label);
	if(l_index == -1) {
		fprintf(stderr, "Unknown label '%s'. Not expecting delay measurement.\n", label);
		return;
	}

	int p_index = peer_index(socket);
	if(p_index == -1) {
		fprintf(stderr, "Unknown socket '%d'. Not expecting delay measurement.\n", socket);
		return;
	}

	delay[l_index][p_index] = from_origin + (timestamp.tv_usec - delay[l_index][p_index]) / 2;
	print_delays();
}

void handle_get_message(int socket, char *message) {
	char *shm;
	char* label;
	int shmid = -1;
	int shmflg = 0;
	size_t size = -1;
	int from_origin = -1;

	#ifdef DEBUG
		printf("Message is '%s'.\n", message);
	#endif

	label = strtok(message, ";");
	size = (size_t) atoi(strtok(NULL, ";"));
	key_t key = strtol(label, NULL, 36);

	// Locate the data segment if locally available
	shmid = shmget(key, size, SHM_R);
	if(shmid < 0) {
		#ifdef DEBUG
			printf("The data segment for label '%s' is not present on this device. This does not concern me.\n", label);
		#endif
		return;
	}

	shm = (char *) shmat(shmid, NULL, shmflg);
	if(shm == (char *) -1) {
		fprintf(stderr, "Could not attach data segment!\n");
		return;
	}

	#ifdef DEBUG
		printf("This device has contents for '%s' with key '%x' of size '%lu'. Sending YEP (aka. delay reply) to socket '%d'.\n", label, key, size, socket);
	#endif

	if(label_present(label) != -1) {
		// Set distance from origin to zero if label is owned by us
		from_origin = 0;
	} else {
		// Set distance from origin


		int l_index = label_index(label);
		if(l_index == -1) {
			fprintf(stderr, "Unknown label '%s'. How can this be?\n", label);
			return;
		}

		int p_index = peer_index(socket);
		if(p_index == -1) {
			fprintf(stderr, "Unknown socket '%d'. How can this be?\n", socket);
			return;
		}

		from_origin = delay[l_index][p_index];
		#ifdef DEBUG
			printf("We are '%d' away from the origin of '%s'.\n", from_origin, label);
		#endif
	}

	char *reply = (char *) malloc(MSG_SIZE);
	bzero(reply, MSG_SIZE);
	sprintf(reply, "YEP;%s;%d", label, from_origin);
	send_message(socket, reply);
	free(reply);
}

void print_labels() {
	int i = -1;

	printf("%c[%d;%dm", 27, 1, 36);

	for(i = 0; i < total_labels; i++) {
		printf("label[%d]='%s'\n", i, labels[i]);
	}

	printf("%c[%dm\n", 27, 0);
}

void print_delays() {
	int i = -1;
	int j = -1;

	printf("%c[%d;%dm", 27, 1, 35);
	printf("delay[LABEL_INDEX][PEER_INDEX]");
	printf("%c[%dm\n", 27, 0);

	for(i = 0; i < total_labels; i++) {
		for(j = 0; j < peers; j++) {
			if(delay[i][j] != -1) {
				printf("[%d][%d]='%d'\n", i, j, delay[i][j]);
			}
		}
	}
}

void peer_connect(char *line) {
	char identifier[35];
	char locator[15];
	int sockfd = -1;
	int ret = -1;
	int optval = 1;
	struct sockaddr_in peer_addr;

	//TODO: FIXME: The input should be verified for overflows and corrupted data!
	sscanf(line, "%35s %15s", identifier, locator);
	#ifdef DEBUG
		printf("Connecting to '%s' at '%s'.\n", identifier, locator);
	#endif

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error initializing peer socket!\n");
		return;
	}

	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if(ret == -1) {
		fprintf(stderr, "Error setting re-use socket option!\n");
		return;
	}

	ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
	if(ret == -1) {
		fprintf(stderr, "Error setting keep-alive socket option!\n");
		return;
	}

	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(PORT);
	memset(&(peer_addr.sin_zero), 0, 8);
	peer_addr.sin_addr.s_addr = inet_addr(locator);

	ret = connect(sockfd, (struct sockaddr *) &peer_addr, sizeof(peer_addr));
	if(ret == -1) {
		fprintf(stderr, "Error connecting peer socket!\n");
		return;
	}

	#ifdef DEBUG
		printf("Socket FD '%d'.\n", sockfd);
	#endif	

	ret = pthread_create(&peer_threads[peers++], NULL, connection, &sockfd);
	if(ret != 0) {
		fprintf(stderr, "Could not create peer thread!\n");
	}

	//TODO: FIXME: If we are too fast here, the socket FD is lost. Is in own thread anyway, so no hurry...
	sleep(1);
}

void *dbus_listen() {
	DBusMessage* msg;
	DBusConnection* conn;
	DBusError err;
	int ret;

	printf("Listening for RPC calls.\n");

	// Initialise the error
	dbus_error_init(&err);

	// Connect to the bus and check for errors
	// TODO: FIXME:
	// DBUS_BUS_SESSION: The login session bus.
	// DBUS_BUS_SYSTEM: The systemwide bus.
	// DBUS_BUS_STARTER: The bus that started us, if any.
	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Connection Error (%s)\n", err.message);
		dbus_error_free(&err);
		exit(EXIT_FAILURE);
	}

	if(NULL == conn) {
		fprintf(stderr, "Connection Null\n");
		exit(EXIT_FAILURE);
	}

	if(!dbus_connection_get_is_connected(conn)) {
		fprintf(stderr, "DBus not connected\n");
		exit(EXIT_FAILURE);
	}

	// Request our name on the bus and check for errors
	ret = dbus_bus_request_name(conn, TRAMP_DBUS_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
	if(dbus_error_is_set(&err)) {
		fprintf(stderr, "Name Error (%s)\n", err.message);
		dbus_error_free(&err);
	}

	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		fprintf(stderr, "Not Primary Owner (%d)\n", ret);
		exit(EXIT_FAILURE);
	}

	// Loop, testing for new messages
	for(EVER) {
		// Non-blocking read of the next available message
		dbus_connection_read_write(conn, 0);
		msg = dbus_connection_pop_message(conn);

		// Loop again if we haven't got a message
		if(NULL == msg) {
			//TODO: FIXME: How long should we sleep? This only impacts RPC, so no real hurry
			usleep(1000);
			continue;
		}
		
		// Check this is a method call for the right interface & method
		if(dbus_message_is_method_call(msg, "test.method.Type", "PUBLISH")) {
			handle_rpc_publish(msg, conn);
		} else if(dbus_message_is_method_call(msg, "test.method.Type", "GET")) {
			handle_rpc_get(msg, conn);
		} else if(dbus_message_is_method_call(msg, "test.method.Type", "SUBSCRIBE")) {
			handle_rpc_subscribe(msg, conn);
		} else {
			// TODO: FIXME: Unknown RPC request
			fprintf(stderr, "Unknown RPC request!\n");
		}

		// Free the message
		dbus_message_unref(msg);
	}

	// Close the connection
	dbus_connection_close(conn);
}

void handle_rpc_publish(DBusMessage *msg, DBusConnection *conn) {
	#ifdef DEBUG
		printf("PUBLISH!\n");
	#endif

	DBusMessage *reply;
	DBusMessageIter args;
	dbus_uint32_t serial = 0;
	char *label = NULL;
	dbus_uint32_t community_members = 0;

	// Read the arguments
	if(!dbus_message_iter_init(msg, &args)) {
		fprintf(stderr, "Message has no attached label to publish!\n");
		return;
	} else if(DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
		fprintf(stderr, "The label is not a string!\n");
		return;
	} else {
		dbus_message_iter_get_basic(&args, &label);
	}

	#ifdef DEBUG
		printf("Publishing label '%s'.\n", label);
	#endif

	// Storing this label so we know we have it locally
	own_labels[total_own_labels++] = strdup(label);

	// Send publish message to the other TRAMP daemons in the community
	char *message = (char *) malloc(MSG_SIZE);
	bzero(message, MSG_SIZE);
	sprintf(message, "PUB;%s", label);

	community_members = send_message(0, message);

	// Create a reply from the message
	reply = dbus_message_new_method_return(msg);

	// Add the number of community members that got the publish message to the reply
	dbus_message_iter_init_append(reply, &args);

	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &community_members)) { 
		fprintf(stderr, "Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	// Send the reply and flush the connection
	if(!dbus_connection_send(conn, reply, &serial)) {
		fprintf(stderr, "Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}
	dbus_connection_flush(conn);

	// Free
	dbus_message_unref(reply);
	free(message);
}

void handle_rpc_subscribe(DBusMessage *msg, DBusConnection *conn) {
	#ifdef DEBUG
		printf("SUBSCRIBE!\n");
	#endif

	DBusMessage *reply;
	DBusMessageIter args;
	dbus_uint32_t serial = 0;
	char *label = NULL;
	dbus_uint32_t community_members = 0;
	size_t size = -1;
	struct timeval timestamp;

	// Read the arguments
	if(!dbus_message_iter_init(msg, &args)) {
		fprintf(stderr, "Message has no attached label to fetch!\n");
		return;
	} else if(DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
		fprintf(stderr, "The label is not a string!\n");
		return;
	} else {
		dbus_message_iter_get_basic(&args, &label);
	}

	if(!dbus_message_iter_next(&args)) {
		fprintf(stderr, "Message has no attached size of data to fetch!\n");
		return;
	} else if(DBUS_TYPE_INT64 != dbus_message_iter_get_arg_type(&args)) {
		fprintf(stderr, "The size is not an int!\n");
		return;
	} else {
		dbus_message_iter_get_basic(&args, &size);
	}

	if(label_present(label) != -1) {
		#ifdef DEBUG
			printf("This device already has data for label '%s'. Do not send out queries!\n", label);
		#endif

		goto REPLY;
	}

	#ifdef DEBUG
		printf("Fetching data for label '%s' of size '%lu'.\n", label, size);
	#endif

	// Send SUB message to the other TRAMP daemons in the community
	char *message = (char *) malloc(MSG_SIZE);
	bzero(message, MSG_SIZE);
	sprintf(message, "SUB;%s;%lu", label, size);

	int index = label_index(label);
	if(index == -1) {
		labels[total_labels++] = label;
		print_labels();
		index = total_labels - 1;
	}

	// Set timestamp for calculating delay
	gettimeofday(&timestamp, NULL);
	int counter = -1;
	for(counter = 0; counter < peers; counter++) {
		delay[index][counter] = timestamp.tv_usec;
		#ifdef DEBUG
			printf("Setting delay of label index '%d' and peer index '%d' to current timestamp '%lu'.\n", index, counter, timestamp.tv_usec);
		#endif
	}

	community_members = send_message(0, message);

	gettimeofday(&timestamp, NULL);
	#ifdef DEBUG
		printf("Done sending GET messages to the community. Time is now: '%lu'.\n", timestamp.tv_usec);
	#endif

	//TODO: FIXME: Wait here for some time to allow incoming messages to appear
	usleep(900000);

	// Find optimal provider for the data based on delay
	int optimal_peer_index = -1;
	int lowest_delay = INT_MAX;
	for(counter = 0; counter < peers; counter++) {
		if(delay[index][counter] < lowest_delay) {
			lowest_delay = delay[index][counter];
			optimal_peer_index = counter;

			#ifdef DEBUG
				printf("So far the best socket FD is '%d' with '%d' delay.\n", sockets[optimal_peer_index], lowest_delay);
			#endif
		}
	}

	#ifdef DEBUG
		printf("Optimal socket FD is '%d' with '%d' delay. Sending FETC message io it.\n", sockets[optimal_peer_index], lowest_delay);
	#endif

	bzero(message, MSG_SIZE);
	sprintf(message, "FETC;%s;%lu", label, size);
	send_message(sockets[optimal_peer_index], message);

	free(message);

	REPLY:

	// Create a reply from the message
	reply = dbus_message_new_method_return(msg);

	// Add the number of community members that got the publish message to the reply
	dbus_message_iter_init_append(reply, &args);

	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &community_members)) { 
		fprintf(stderr, "Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	// Send the reply and flush the connection
	if(!dbus_connection_send(conn, reply, &serial)) {
		fprintf(stderr, "Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	dbus_connection_flush(conn);

	// Free the reply
	dbus_message_unref(reply);
}

void handle_rpc_get(DBusMessage *msg, DBusConnection *conn) {
	#ifdef DEBUG
		printf("GET!\n");
	#endif

	DBusMessage *reply;
	DBusMessageIter args;
	dbus_uint32_t serial = 0;
	char *label = NULL;
	dbus_uint32_t community_members = 0;
	size_t size = -1;
	struct timeval timestamp;

	// Read the arguments
	if(!dbus_message_iter_init(msg, &args)) {
		fprintf(stderr, "Message has no attached label to fetch!\n");
		return;
	} else if(DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
		fprintf(stderr, "The label is not a string!\n");
		return;
	} else {
		dbus_message_iter_get_basic(&args, &label);
	}

	if(!dbus_message_iter_next(&args)) {
		fprintf(stderr, "Message has no attached size of data to fetch!\n");
		return;
	} else if(DBUS_TYPE_INT64 != dbus_message_iter_get_arg_type(&args)) {
		fprintf(stderr, "The size is not an int!\n");
		return;
	} else {
		dbus_message_iter_get_basic(&args, &size);
	}

	if(label_present(label) != -1) {
		#ifdef DEBUG
			printf("This device already has data for label '%s'. Do not send out queries!\n", label);
		#endif

		goto REPLY;
	}

	#ifdef DEBUG
		printf("Fetching data for label '%s' of size '%lu'.\n", label, size);
	#endif

	// Send GET message to the other TRAMP daemons in the community
	char *message = (char *) malloc(MSG_SIZE);
	bzero(message, MSG_SIZE);
	sprintf(message, "GET;%s;%lu", label, size);

	int index = label_index(label);
	if(index == -1) {
		labels[total_labels++] = label;
		print_labels();
		index = total_labels - 1;
	}

	// Set timestamp for calculating delay
	gettimeofday(&timestamp, NULL);
	int counter = -1;
	for(counter = 0; counter < peers; counter++) {
		delay[index][counter] = timestamp.tv_usec;
		#ifdef DEBUG
			printf("Setting delay of label index '%d' and peer index '%d' to current timestamp '%lu'.\n", index, counter, timestamp.tv_usec);
		#endif
	}

	community_members = send_message(0, message);

	gettimeofday(&timestamp, NULL);
	#ifdef DEBUG
		printf("Done sending GET messages to the community. Time is now: '%lu'.\n", timestamp.tv_usec);
	#endif

	//TODO: FIXME: Wait here for some time to allow incoming messages to appear
	usleep(900000);

	// Find optimal provider for the data based on delay
	int optimal_peer_index = -1;
	int lowest_delay = INT_MAX;
	for(counter = 0; counter < peers; counter++) {
		if(delay[index][counter] < lowest_delay) {
			lowest_delay = delay[index][counter];
			optimal_peer_index = counter;

			#ifdef DEBUG
				printf("So far the best socket FD is '%d' with '%d' delay.\n", sockets[optimal_peer_index], lowest_delay);
			#endif
		}
	}

	#ifdef DEBUG
		printf("Optimal socket FD is '%d' with '%d' delay. Sending FET message io it.\n", sockets[optimal_peer_index], lowest_delay);
	#endif

	bzero(message, MSG_SIZE);
	sprintf(message, "FET;%s;%lu", label, size);
	send_message(sockets[optimal_peer_index], message);

	free(message);

	REPLY:

	// Create a reply from the message
	reply = dbus_message_new_method_return(msg);

	// Add the number of community members that got the publish message to the reply
	dbus_message_iter_init_append(reply, &args);

	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &community_members)) { 
		fprintf(stderr, "Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	// Send the reply and flush the connection
	if(!dbus_connection_send(conn, reply, &serial)) {
		fprintf(stderr, "Out Of Memory!\n");
		exit(EXIT_FAILURE);
	}

	dbus_connection_flush(conn);

	// Free the reply
	dbus_message_unref(reply);
}

void bootstrap() {
	#ifdef DEBUG
		printf("Bootstrapping community.\n");
	#endif

  char line[52];
	FILE *fp = NULL;

	fp = fopen("my-devices.cache", "r");
	if(!fp) {
		fprintf(stderr, "No cache file found for previous community members!\n");
	}

	while(fgets(line, sizeof(line), fp) != NULL) {
		peer_connect(line);
	}

	fclose(fp);
}

int label_index(char *label) {
	int i = -1;

	for(i = 0; i < total_labels; i++) {
		if(strcmp(label, labels[i]) == 0 ) {
			#ifdef DEBUG
				printf("Found label '%s' at '%d'.\n", labels[i], i);
			#endif
			return i;
		}
	}

	return -1;
}

int peer_index(int socket) {
	int i = -1;

	for(i = 0; i < peers; i++) {
		if(socket == sockets[i]) {
			#ifdef DEBUG
				printf("Found socket '%d' at '%d'.\n", sockets[i], i);
			#endif
			return i;
		}
	}

	return -1;
}

int label_present(char *label) {
	int i = -1;

	for(i = 0; i < total_own_labels; i++) {
		if(strcmp(label, own_labels[i]) == 0 ) {
			#ifdef DEBUG
				printf("The label '%s' is available locally.\n", own_labels[i]);
			#endif
			return i;
		}
	}

	return -1;
}

int main(int argc, char *argv[]) {
	// Make myself a daemon
	daemonize();

	// Disable output buffering so log will appear
	setbuf(stdout, NULL);

	int ret = -1;

	// Setup dbus thread
	pthread_t dbus_thread;
	ret = pthread_create(&dbus_thread, NULL, dbus_listen, NULL);
	if(ret != 0) {
		fprintf(stderr, "Could not create RPC thread!\n");
		exit(EXIT_FAILURE);
	}

	// Connect to other TRAMP daemons
	bootstrap();

	// Setup server thread
	pthread_t server_thread;
	ret = pthread_create(&server_thread, NULL, server_listen, NULL);
	if(ret != 0) {
		fprintf(stderr, "Could not create TRAMP daemon server thread!\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "READY!\n");

	// Waiting for threads to finish. This will never happen.
	pthread_join(dbus_thread, NULL);
	pthread_join(server_thread, NULL);

	return EXIT_SUCCESS;
}
