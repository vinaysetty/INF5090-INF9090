#include <libtramp.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define EVER ;;

char *trim(char *s) {
    char *ptr;
    if (!s)
        return NULL;   // handle NULL string
    if (!*s)
        return s;      // handle empty string
    for (ptr = s + strlen(s) - 1; (ptr >= s) && isspace(*ptr); --ptr);
    ptr[1] = '\0';
    return s;
}

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
        
        //Page Size
        int pageSize = 4096;
	// We decide that our main data structure is |0|1|2|...|
	//                                           |S|T|T|...|
	// Where S = sequence number and T is Text.
	// I.e the first byte is a sequence number and the rest is the chat
	char *chat_msg = (char *) tramp_initialize(label_msg, size_msg);

	// We decide that the username data structure is as simple as |0|1|2|3|...|
	//                                                            |N|I|C|K|...|
	// I.e. all the bytes are chatachters of the nickname
	char *chat_usr = (char *) tramp_initialize(label_usr, size_usr);
        
         printf("finished tramp init\n");
        //Before writing to shared memory get the chat message in this string
        char *message = (char *) malloc(size_msg);
        
        //File pointer to be read
        FILE *fd;
	// Publish the labels to the community
        
        char *filename = (char *) malloc(size_msg);
        //file size
        int fileLen = 0;
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
//		strcpy(chat_msg+1,"vinay\n");
//		size_msg = 8;
//		++count;
		// First byte is sequence number. Rest is chat.
		fgets(message, size_msg, stdin);
                //If the chat message starts with send start sending the file
                if(strncmp(message, "send", strlen("send")) == 0)
                {
                    printf("inside send\n");
                    strcpy(filename, message+5);
                    fd = fopen(trim(filename), "r");
                    if (!fd)
                     {
                                fprintf(stderr, "can't open file %s", filename);
                                continue;
                       }
                    	//Get file length
                    fseek(fd, 0, SEEK_END);
                    fileLen=ftell(fd);
                    fseek(fd, 0, SEEK_SET);
                    //First send the file size
                    printf("file open complete\n");
                    printf("file_size: %d file_name: %s\n", fileLen, filename);
                    sprintf(chat_msg+1, "file:%d|%s", fileLen, filename);
                    *chat_msg = ++counter;
                    int bytes_sent = 0;
                    while(1)
                    {
                        printf("size of chat_msg: %d\n", sizeof(chat_msg));
                        int bytes_read = fread(chat_msg+1,1,(2*pageSize),fd);
                        chat_msg[1] = bytes_read;
                        printf("Bytes read : %d\n", bytes_read);
                        bytes_sent += bytes_read;
                        chat_msg[pageSize-1]='\0';
                        *chat_msg = ++counter;
                        if(bytes_sent >= fileLen)
                            break;
                        sleep(2);

                         if (bytes_read == 0) // We're done reading from the file
                                break;
                        if(bytes_read < 0)
                        {
                            printf("Error reading file %s, error code: %d \n", filename, bytes_read);
                            break;
                        }
                    }
                }
                else
                {
                    strcpy(chat_msg+1, message);
                        *chat_msg = ++counter;
                }
		printf("%d size: ", size_msg);
		// First byte is sequence number
	}

	return EXIT_SUCCESS;
}

