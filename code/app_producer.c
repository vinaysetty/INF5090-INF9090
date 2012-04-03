#include <libtramp.h>
#include <string.h>

#define EVER ;;

int main(int argc, char **argv) {
	// Disable output buffering so log will appear
	setbuf(stdout, NULL);

	// Labels are base36 encoded and hence can only contain A-Z, a-z and 0-9
	// Length is also limited.
	char *label_msg = "ChatMSG";
	char *label_usr = "ChatUSR";

	// Data segment sizes are bounded by the OS' shared memory
	// $ cat /proc/sys/kernel/shmmax gives 32 MB on my machine
	size_t size_msg = 80;
	size_t size_usr = 20;

	// Sequence number for outgoing messages
	unsigned char counter = 0;

	// We decide that our main data structure is |0|1|2|...|
	//                                           |S|T|T|...|
	// Where S = sequence number and T is Text.
	// I.e the first byte is a sequence number and the rest is the chat
	char *chat_msg = (char *) tramp_initialize(label_msg, size_msg);

	// We decide that the username data structure is as simple as |0|1|2|3|...|
	//                                                            |N|I|C|K|...|
	// I.e. all the bytes are chatachters of the nickname
	char *chat_usr = (char *) tramp_initialize(label_usr, size_usr);

	// Publish the labels to the community
	tramp_publish(label_msg, size_msg);
	tramp_publish(label_usr, size_usr);

	// Set the nickname from command line argument or Anonymous if not specified
	bzero(chat_usr, size_usr);
	if(argv[1] == NULL) {
		sprintf(chat_usr, "Anonymous");
	} else {
		strncpy(chat_usr, argv[1], strlen(argv[1]));
	}

	// Continue from last time if sequence number is present in data segment
	if((unsigned char) *chat_msg > 0 && (unsigned char) *chat_msg <= 255) {
		counter = (unsigned char) *chat_msg;
	}

	for(EVER) {
		// Prompt user for input
		printf("<%s> ", chat_usr);

		// First byte is sequence number. Rest is chat.
		fgets(chat_msg + 1, size_msg, stdin);

		// First byte is sequence number
		*chat_msg = ++counter;
	}

	return EXIT_SUCCESS;
}
