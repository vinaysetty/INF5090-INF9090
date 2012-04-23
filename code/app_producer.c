#include <libtramp.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define EVER ;;
typedef unsigned char byte;
typedef unsigned int uint32;

void intToBytes( uint32 val, byte *pBytes )
{
    pBytes[0] = (byte)val;
    pBytes[1] = (byte)(val >> 8);
    pBytes[2] = (byte)(val >> 16);
    pBytes[3] = (byte)(val >> 24);
}

uint32 GetInt32( byte *pBytes )
{
    return (uint32)(*(pBytes + 3) << 24 | *(pBytes + 2) << 16 | *(pBytes + 1) << 8 | *pBytes);
}


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
        char *message = (char *) malloc(size_msg*pageSize);
        
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
    int numPacketsSent = 0;
	for(EVER) {
		// Prompt user for input
		printf("<%s> ", chat_usr);
//		strcpy(chat_msg+1,"vinay\n");
//		size_msg = 8;
//		++count;
		// First byte is sequence number. Rest is chat.
       
		fgets(message, size_msg*100, stdin);
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
           
                    if(strrchr(filename, '/')!= NULL)
                        sprintf(chat_msg+1, "file:%d|%s", fileLen, strrchr(filename, '/')+1);
                    else
                        sprintf(chat_msg+1, "file:%d|%s", fileLen, filename);
                    *chat_msg = ++counter;
                    numPacketsSent++;
                    if(strrchr(filename, '/')!= NULL)
                        printf("seq: %d file_size: %d file_name: %s\n", counter, fileLen, strrchr(filename, '/')+1);
                    else
                        printf("seq: %d file_size: %d file_name: %s\n", counter, fileLen, filename);
                    int bytes_sent = 0;
                    sleep(2);
//                    usleep(100000);
                    while(1)
                    {
                        printf("size of chat_msg: %d\n", sizeof(chat_msg));
                        uint32 bytes_read = fread(message,1,size_msg*pageSize-5,fd);
                        memcpy(chat_msg+5,message,bytes_read);
                        intToBytes(bytes_read, chat_msg+1);
                        printf("num bytes from the chat_msg: %u\n", GetInt32(chat_msg+1));
                        bytes_sent += bytes_read;
                        *chat_msg = ++counter;
                        numPacketsSent++;
                        printf("seq: %d  Bytes read : %u\n", counter, bytes_read);
                        if(bytes_sent >= fileLen){
//                            numPacketsSent = 0;
                            usleep(100000);
                            printf("all bytes sent\n");
                            printf("num packets sent: %d, bytes: %d ", numPacketsSent, bytes_sent);
//                            *chat_msg = 255;
                            break;
                        }
                        //sleep for 500ms
                        usleep(100000);

                         if (bytes_read == 0) // We're done reading from the file
                         {
                             printf("0 bytes read");
                             break;
                         }
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
                    numPacketsSent++;
                }
		printf("%d size: ", size_msg);
		// First byte is sequence number
	}

	return EXIT_SUCCESS;
}

