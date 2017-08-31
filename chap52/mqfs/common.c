/* common.c - Implementation of shared functions between client and server of mqfs.
 *
 * See the common.h for more information about 'mqfs'.
 *
 * Author: Renato Mascarenhas Costa
 */

#include "common.h"

void
setQueueAttributes(struct mq_attr *attr) {
	attr->mq_flags = 0;
	attr->mq_maxmsg = 10;
	attr->mq_msgsize = MSG_LEN;
}
