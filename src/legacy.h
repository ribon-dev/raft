/* Compatibility layer between v1.x and v0.x. */

#ifndef RAFT_LEGACY_H_
#define RAFT_LEGACY_H_

#include "../include/raft.h"

/* Pass the given event to raft_step() and execute the resulting tasks using the
 * legacy raft_io interface. */
int LegacyForwardToRaftIo(struct raft *r, struct raft_event *event);

/* Fail all pending client requests with RAFT_LEADERSHIPLOST. */
void LegacyFailPendingRequests(struct raft *r);

/* Fire the callbacks of all completed client requests. */
void LegacyFireCompletedRequests(struct raft *r);

/* Initialize a leadership transfer request. */
void LegacyLeadershipTransferInit(struct raft *r,
                                  struct raft_transfer *req,
                                  raft_id id,
                                  raft_transfer_cb cb);

#endif /* RAFT_LEGACY_H_ */
