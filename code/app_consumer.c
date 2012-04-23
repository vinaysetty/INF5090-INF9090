#include <libtramp.h>
#include <string.h>
#include <fcntl.h>
#include <trampd.h>
#define EVER ;;

typedef unsigned char byte;
typedef unsigned int uint32;

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

uint32 GetInt32( byte *pBytes )
{
    return (uint32)(*(pBytes + 3) << 24 | *(pBytes + 2) << 16 | *(pBytes + 1) << 8 | *pBytes);
}

void *writeFile(void *threadid){
    long tid;
    tid = (long)threadid;
    printf("THREAD[%ld]: Hello world from thread: %ld\n", tid, tid);
    sleep(5);
    printf("THREAD[%ld]: I'm done\n", tid);
    pthread_exit(NULL);
}

char* substring(int start, int stop, const char *text)
{
    char *substring = (char *)(malloc(stop - start+1));
    printf("%d, %d, %s\n",start, stop, text);
   sprintf(substring, "%.*s\n", stop - start, &text[start]);
   return substring;
}

int createFolder(char* folder_name)
{
//    return mkdir(folder_name);
    return -1;
}

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
        char *temp_str;
        int file_size = -1;
        char *file_name;
        int bytes_rcvd = 0;
        FILE* fd;
        //Page Size
        int pageSize = 4096;
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
    char *message = (char *) malloc(size_msg*pageSize);
	// Get the nickname once
	tramp_get(label_usr, size_usr);

	// Get the messages continous
	tramp_subscribe(label_msg, size_msg);
    
   
	// Print waiting message to screen
	printf("%c[%d;%dm", 27, 1, 34);
	printf("Waiting for messages from %s...", chat_usr);
	printf("%c[%dm\n", 27, 0);

    if(createFolder("./downloads") != 0)
    {
        printf("Failed to create folder downloads\n");
        
    }
    int numPackets = 0;
	for(EVER) {
		// Check for new messages. This is either because of higher sequence number or because unsinged char has wrapped
		if(((unsigned char) *chat_msg > counter) || (counter == 255 && (unsigned char) *chat_msg == 1)) {

			// Update sequence number
			counter = (unsigned char) *chat_msg;

//			// Remove enter chatachter
//			if(chat_msg[strlen(chat_msg) - 1] == '\n') {
//				chat_msg[strlen(chat_msg) - 1] = '\0';
//			}

            memcpy(message,chat_msg+1,size_msg*pageSize-1);
            
            printf("Seq: %d\n", counter);
          

                        if(strncmp(message, "file:", strlen("file:")) == 0)
                        {
                            numPackets++;
                            printf("got file header\n");
                            char *substr = strstr(message, "|");
                            printf("%s\n", substr);
                            temp_str = substring(5, substr-message, message);
                            printf("%s\n", temp_str);
                            file_size = atoi(temp_str);
                            free(temp_str);
                            temp_str = NULL;
                            file_name = substring(0, strlen(substr+1), substr+1);
                            printf("%s,%d\n",trim(file_name), file_size);
                            char * full_file_name = (char *)malloc(strlen(file_name)+ strlen("./downloads/")+1);
                            strcpy(full_file_name, "./downloads/");
                            fd = fopen(strcat(full_file_name,file_name), "wb+");
                            free(file_name);
                            free(full_file_name);
                            full_file_name = NULL;
                            file_name = NULL;
                            continue;
                        }
                        
                        if(file_size >= 0)
                        {
                            numPackets++;
                         
//                             printf("num bytes in hex%x%x%x%x\n", message, message+1, message+2,message+3);
//                            uint32 bytes = GetInt32(message);
                            uint32 bytes = GetInt32(message);
                            printf("size of packet %u\n", bytes);
                            bytes_rcvd += fwrite(message+4, 1, bytes, fd);
                            printf("received so far: %d bytes %d packets \n", bytes_rcvd, numPackets);                            
                            fflush(fd);
                            if(bytes_rcvd == file_size)
                            {
                                file_size = -1;
                                fclose(fd);
                                bytes_rcvd = 0;
                                printf("Number of packets received: %d", numPackets);

                            }
                            
                            continue;
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
