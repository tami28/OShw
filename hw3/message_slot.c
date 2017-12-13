//
// Created by root on 07/12/2017.
//
#include "message_slot.h"
#include<linux/init.h>
#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include <linux/errno.h>
MODULE_LICENSE("GPL");


/*
 * Announce our functions
 */
static int deviceOpen(struct inode *, struct file *);
static int deviceRelease(struct inode *, struct file *);
static ssize_t deviceRead(struct file *, char *, size_t, loff_t *);
static ssize_t deviceWrite(struct file *, const char *, size_t, loff_t *);
static long deviceIoctl( struct file* file, unsigned int   ioctl_command_id, unsigned long  ioctl_param );
static void freeList(Channel* channelList);

static void freeMinors(void);
MinorList* minors = NULL;

struct file_operations fops =
        {
                .read           = deviceRead,
                .write          = deviceWrite,
                .open           = deviceOpen,
                .unlocked_ioctl = deviceIoctl,
                .release        = deviceRelease
            };


static int __init deviceInit(void){
    int rc = -1;
    printk(KERN_INFO "message_slot: Initializong module\n");
    // Register driver capabilities. Obtain major num
    rc = register_chrdev( DEVICE_MAJOR, DEVICE_NAME, &fops );

    // Negative values signify an error
    if( rc < 0 )
    {
        printk( KERN_ALERT "%s registraion failed for  %d\n",DEVICE_NAME, DEVICE_MAJOR);
        return rc;
    }

    printk(KERN_INFO "message_slot: registered major number %d\n", DEVICE_MAJOR);
    return 0;
}

static int deviceOpen(struct inode * in, struct file * fs){
    unsigned minor = iminor(in);
    printk(KERN_INFO "message_slot: Opening device with minor %d\n", minor);
    int found = 0;
    MinorList* curr = minors;
    MinorList* prev = NULL;
    if (minors != NULL){
        while(curr != NULL){
            if (curr->minor == minor){
                found = 1;
                break;
            }
            prev = curr;
            curr = curr->next;
        }
        if(found == 1){
            return SUCCESS;
        }
        prev->next = (MinorList*)kmalloc(sizeof(MinorList), GFP_KERNEL);
        if(prev->next == NULL){
            //TODO: errno
            return FAILED;
        }
        curr = prev->next;
    } else{
        minors = (MinorList*)kmalloc(sizeof(MinorList), GFP_KERNEL);
        if (minors == NULL){
            //TODO errno
            printk(KERN_ERR "message_slot: Couldn't allocte memory");
            return FAILED;
        }
        curr = minors;
    }
//Curr should hold now the place in which to initialize.
    curr->next = NULL;
    curr ->prev = prev;
    curr->minor = minor;
    curr ->channels = NULL;
    curr->curr = NULL;
    return SUCCESS;
}

static ssize_t deviceWrite(struct file * fs, const char * buff, size_t len, loff_t * something){
    unsigned minor = iminor(fs->f_path.dentry->d_inode);
    printk(KERN_INFO "message_slot: Got minor %d in write", minor);
    MinorList* currMinor = NULL;
    MinorList* curr = minors;
    ssize_t i =-1;
    while(curr != NULL){
        if(curr->minor == minor){
            currMinor=curr;
            break;
        }
        curr = curr->next;
    }
    if(currMinor == NULL || currMinor->curr == NULL){
        //TODO:: set errno here
        printk(KERN_INFO "message_slot: Minor %d doesn't exist or wasn't ioctl'ed", minor);
        return FAILED;
    }
    for(i = 0; i < len && i < (BUFFER_LENGTH-1); ++i )
    {
        get_user(currMinor->curr->message[i], &buff[i]);
    }
    printk(KERN_INFO "message_slot: current i: %d\n", i);
    currMinor->curr->message[i] = '\0';
    printk(KERN_INFO "message_slot: Finished write");
    return SUCCESS;

}

static ssize_t deviceRead(struct file * fs, char * buff, size_t len, loff_t * something){
    unsigned minor = iminor(fs->f_path.dentry->d_inode);
    MinorList* currMinor = NULL;
    MinorList* curr = minors;
    ssize_t i =-1;
    while(curr != NULL){
        if(curr->minor == minor){
            currMinor=curr;
            break;
        }
        curr = curr->next;
    }
    if(currMinor == NULL || currMinor->curr == NULL){
        //TODO:: set errno here
        return i;
    }
    for(i = 0; i < len && i < BUFFER_LENGTH; ++i )
    {
        put_user(currMinor->curr->message[i], &buff[i]);
    }
    return i;

}

static int deviceRelease(struct inode * in, struct file * fs){
    return 0;
}

static long deviceIoctl( struct file* file, unsigned int   ioctl_command_id, unsigned long  ioctl_param ){
    unsigned minor = iminor(file->f_path.dentry->d_inode);
    printk(KERN_INFO "message_slot: Got minor %d in ioctl", minor);
    MinorList* curr = minors;
    int found = 0;
    if(MSG_SLOT_CHANNEL != ioctl_command_id){
        printk(KERN_ERR "message_slot: Error in ioctl - not the right command id");
        //TODO is this what is supposed to be here?
        return FAILED;
    }
    while(curr != NULL){
        if (curr->minor == minor){
            found = 1;
            break;
        }
        curr = curr->next;
    }
    if(found == 0){
        printk(KERN_ERR "message_slot: No such minor (in ioctl)");
        //tODO: errno here
        return FAILED;
    }

    if (curr->curr != NULL && curr->curr->id ==ioctl_param){
        return SUCCESS;
    }
    Channel* wanted = curr->channels;
    found = 0;

    while(wanted != NULL){
        if (wanted->id == ioctl_param){
            found = 1;
            break;
        }
        wanted = wanted->next;
    }
    if (found == 1){
        curr->curr = wanted;
        return SUCCESS;
    }

    curr->curr = (Channel*)kmalloc(sizeof(Channel), GFP_KERNEL );
    if (curr->curr == NULL){
        printk(KERN_ERR "message_slot: couldn't kmalloc in ioctl");
        //todo errno here
        return FAILED;
    }
    curr->curr->next = curr->channels;
    if (curr->channels != NULL){
        curr ->channels->prev = curr->curr;
    } else{
        curr->channels = curr->curr;
    }
    curr->curr->prev = NULL;
    curr ->curr->id = ioctl_param;
    curr->curr->length = 0;
    printk(KERN_INFO "message_slot: Finished ioctl!");
    return SUCCESS;

}


//TODO!!!!! keep message lebgth and use is in read.
static void __exit cleanup(void){
    //free memory:
    freeMinors();
    // Unregister the device
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
}

static void freeMinors(void){
    if(minors == NULL){
        return;
    }

    MinorList* curr = minors->next;
    MinorList* prev = minors;
    while (prev!= NULL){
        curr = prev->next;
        freeList(prev->channels);
        kfree(prev);
        prev = curr;
    }

}

static void freeList(Channel* channelList){
    if(channelList == NULL){
        return;
    }
    Channel* curr = channelList->next;
    Channel* prev = channelList;
    while(prev!=NULL){
        curr = prev->next;
        kfree(prev);
        prev = curr;
    }
}

module_init(deviceInit);
module_exit(cleanup);