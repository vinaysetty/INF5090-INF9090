CC = gcc
LD = ld
#CC = arm-linux-gnueabi-gcc
#LD = arm-linux-gnueabi-ld
FLAGS = -Wall -D DEBUG
LOG_FILES = $(wildcard log_*.txt)

all: lib daemon app

lib:
	@echo "Compiling TRAMP library..."
	@$(CC) $(FLAGS) -o libtramp.so libtramp.c -fPIC -shared  `pkg-config --cflags --libs dbus-1`

daemon:
	@echo "Compiling TRAMP daemon..."
	@$(CC) $(FLAGS) trampd.c -o trampd `pkg-config --cflags --libs dbus-1` -lpthread

app: lib
	@echo "Compiling producer application..."
	@$(CC) app_producer.c -o app_producer `pkg-config --cflags --libs dbus-1` -L. -ltramp -I.
	@echo "Compiling consumer application..."
	@$(CC) app_consumer.c -o app_consumer `pkg-config --cflags --libs dbus-1` -L. -ltramp -I.

clean:
	@echo "Cleaning..."
ifneq ($(strip $(LOG_FILES)),)
	@tar czf logs_`date +%H%M%S`.tgz log_*.txt
endif
	@killall --quiet app_producer app_consumer trampd || true
	@./shm_clear.sh -y
	@rm -f *.o *.so *~ app_producer app_consumer trampd log_*.txt

rt:
	@echo "Starting TRAMP daemon..."
	@./trampd > log_trampd_`hostname`.txt

rp:
	@echo "Starting producer application..."
	@LD_LIBRARY_PATH=. ./app_producer Bob

rc:
	@echo "Starting consumer application..."
	@LD_LIBRARY_PATH=. ./app_consumer > log_consumer_`hostname`.txt

help:
	@echo "The following make targets are available:"
	@echo "  help   - Print this message"
	@echo "  lib    - Compile TRAMP shared library"
	@echo "  daemon - Compile TRAMP daemon"
	@echo "  app    - Compile shared library, app_producer and app_consumer"
	@echo "  all    - Compile everything"
	@echo "  rt     - Start TRAMP daemon and log to file"
	@echo "  rp     - Start app_producer with username Bob"
	@echo "  rc     - Start app_consumer and log to file"
	@echo "  clean  - Remove temporary files, logs and binaries"

