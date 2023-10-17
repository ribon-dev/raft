/* Enqueue asynchronous tasks. */

#ifndef RAFT_TASK_H_
#define RAFT_TASK_H_

#include "../include/raft.h"

/* Create and enqueue a RAFT_SEND_MESSAGE task to send the given message to the
 * server with the given ID and address.
 *
 * Errors:
 *
 * RAFT_NOMEM
 *     The r->tasks array could not be resized to fit the new task.
 */
int TaskSendMessage(struct raft *r,
                    raft_id id,
                    const char *address,
                    struct raft_message *message);

/* Create and enqueue a RAFT_PERSIST_ENTRIES task to persist the given entries
 * starting at the given index.
 *
 * Errors:
 *
 * RAFT_NOMEM
 *     The r->tasks array could not be resized to fit the new task.
 */
int TaskPersistEntries(struct raft *r,
                       raft_index first_index,
                       struct raft_entry entries[],
                       unsigned n);

/* Create and enqueue a RAFT_PERSIST_TERM_AND_VOTE task to persist the given
 * term and vote.
 *
 * Errors:
 *
 * RAFT_NOMEM
 *     The r->tasks array could not be resized to fit the new task.
 */
int TaskPersistTermAndVote(struct raft *r, raft_term term, raft_id voted_for);

/* Create and enqueue a RAFT_PERSIST_SNAPSHOT task to persist a single chunk of
 * the snapshot with the given metadata, starting at the given offset.
 *
 * Errors:
 *
 * RAFT_NOMEM
 *     The r->tasks array could not be resized to fit the new task.
 */
int TaskPersistSnapshot(struct raft *r,
                        struct raft_snapshot_metadata metadata,
                        size_t offset,
                        struct raft_buffer chunk,
                        bool last);

/* Create and enqueue a RAFT_LOAD_SNAPSHOT task to load a single chunk of the
 * snapshot at the given index starting at the given offset.
 *
 * Errors:
 *
 * RAFT_NOMEM
 *     The r->tasks array could not be resized to fit the new task.
 */
int TaskLoadSnapshot(struct raft *r, raft_index index, size_t offset);

/* Create and enqueue a RAFT_APPLY_COMMAND task to apply to the application FSM
 * the given command, contained in the entry at the given index.
 *
 * Errors:
 *
 * RAFT_NOMEM
 *     The r->tasks array could not be resized to fit the new task.
 */
int TaskApplyCommand(struct raft *r,
                     raft_index index,
                     const struct raft_buffer *command);

/* Create and enqueue a RAFT_TAKE_SNAPSHOT task to reset the state of the
 * application FSM using the snapshot at the given index.
 *
 * Errors:
 *
 * RAFT_NOMEM
 *     The r->tasks array could not be resized to fit the new task.
 */
int TaskRestoreSnapshot(struct raft *r, raft_index index);

#endif /* RAFT_TASK_H_ */
