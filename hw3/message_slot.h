/*
 * Taken from recitation::
 */
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE



/*
 * From here based on recitation, some internet examples and my brain:
 */

#define BUFFER_LENGTH 129
#define DEVICE_MAJOR 285
#define DEVICE_NAME "tamis_char_device"
#define MSG_SLOT_CHANNEL _IOW(DEVICE_MAJOR, 0, unsigned int)
#define SUCCESS 0
#define FAILED -1

/*
 * Thiis struct will hold data for a single channel.
 */
typedef struct Channel Channel;
struct Channel{
    int id;
    char message[BUFFER_LENGTH];
    int length;
    struct Channel* prev;
    struct Channel* next;
//    bool looped; Don't know if this is needed??
};

typedef struct MinorList MinorList;
struct MinorList{
    unsigned minor;
    Channel* channels;
    Channel* curr;
    MinorList* prev;
    MinorList* next;

};
