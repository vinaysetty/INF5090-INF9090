#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <dbus/dbus.h>

#define TRAMP_DBUS_NAME "org.tramp-project"

/**
 *
 * Associate a label to a state of a given size.
 *
 * @param label identificator for the state
 * @param size number of bytes of the state
 * @return pointer to the data structure (state)
 */
void*	tramp_initialize(char *label, size_t size);

/**
 *
 * Make state available for other devices.
 *
 * @param label identificator for the state
 * @param size number of bytes of the state
 */
void	tramp_publish(char *label, size_t size);

/**
 * Receive current instance of a state.
 *
 * @param label identificator for the state
 * @param size number of bytes of the state
 */
void	tramp_get(char *label, size_t size);

/**
 *
 * Receive continuous state updates for a label.
 *
 * @param label identificator for the state
 * @param size number of bytes of the state
 */
void	tramp_subscribe(char *label, size_t size);
