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
	// $ cat /proc/sys/kernel/shmmax gives 33554432, i.e 32 MB on my machine
	size_t size_msg = 80;
	size_t size_usr = 20;

	// Sequence number for incoming messages
	unsigned char counter = 0;

	// We decide that our main data structure is |0|1|2|...|
	//                                           |S|T|T|...|
	// Where S = sequence number and T is Text.
	// I.e the first byte is a sequence number and the rest is the chat
	char *chat_msg = (char *) tramp_initialize(label_msg, size_msg);

	// We decide that the username data structure is as simple as |0|1|2|3|...|
	//                                                            |N|I|C|K|...|
	// I.e. all the bytes are chatachter of the nickname
	char *chat_usr = (char *) tramp_initialize(label_usr, size_usr);

	// Get the nickname once
	tramp_get(label_usr, size_usr);

	// Get the messages continous
	tramp_subscribe(label_msg, size_msg);

	// Print waiting message to screen
	printf("%c[%d;%dm", 27, 1, 34);
	printf("Waiting for messages from %s...", chat_usr);
	printf("%c[%dm\n", 27, 0);

	for(EVER) {
		// Check for new messages. This is either because of higher sequence number or because unsinged char has wrapped
		if(((unsigned char) *chat_msg > counter) || (counter == 255 && (unsigned char) *chat_msg == 1)) {

			// Update sequence number
			counter = (unsigned char) *chat_msg;

			// Remove enter chatachter
			if(chat_msg[strlen(chat_msg) - 1] == '\n') {
				chat_msg[strlen(chat_msg) - 1] = '\0';
			}

			// Print the message to screen witch fancy smancy colors
			printf("%c[%d;%dm", 27, 1, 32);
			printf("[%u] ", counter);
			printf("%c[%d;%dm", 27, 1, 35);
			printf("<%s> ", chat_usr);
			printf("%c[%d;%dm", 27, 1, 37);
			printf("%s", chat_msg + 1);
			printf("%c[%dm\n", 27, 0);
		}

	}

	return EXIT_SUCCESS;
}
