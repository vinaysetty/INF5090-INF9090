#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dbus/dbus.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <limits.h>

#define TRAMP_DBUS_NAME "org.tramp-project"
#define EVER ;;
#define PORT 1337
#define MAX_PEERS 100
#define MAX_LABELS 1000
#define MSG_SIZE 80

void *dbus_listen();
void *server_listen();
void *connection(void *socket);
void *data(void *message);

void bootstrap();
void peer_connect(char *line);
int send_message(int socket, char *message);
int send_file(int socket, char* filename);
pthread_t peer_threads[MAX_PEERS];
pthread_t data_threads[MAX_LABELS];

int sockets[MAX_PEERS];
unsigned int peers = 0;

unsigned int num_data_threads = 0;

char	*own_labels[MAX_LABELS];
char	*labels[MAX_LABELS];
unsigned int total_labels = 0;
unsigned int total_own_labels = 0;
int		delay[MAX_LABELS][MAX_PEERS] = { {-1} };
int msg_delay[MAX_PEERS][256] = { {-1} };
int total_delay[MAX_PEERS];

int label_index(char *label);
int peer_index(int socket);
int label_present(char *label);

// Socket handlers
void handle_pub_message(int socket, char *message);
void handle_get_message(int socket, char *message);
void handle_yep_message(int socket, char *message);
void handle_fet_message(int socket, char *message);
void handle_fetc_message(int socket, char *message);
void handle_dat_message(int socket, char *message);
void handle_ack_message(int socket, char *message);

// RPC handlers
void handle_rpc_publish(DBusMessage *msg, DBusConnection *conn);
void handle_rpc_get(DBusMessage *msg, DBusConnection *conn);
void handle_rpc_subscribe(DBusMessage *msg, DBusConnection *conn);

// Debug
void print_labels();
void print_delays();
void send_dat_ack();
