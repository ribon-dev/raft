#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "disk.h"
#include "disk_kaio.h"
#include "disk_options.h"
#include "disk_parse.h"
#include "disk_pwrite.h"
#include "disk_uring.h"
#include "fs.h"
#include "timer.h"

/* Allocate a buffer of the given size. */
static void allocBuffer(struct iovec *iov, size_t size)
{
    unsigned i;

    iov->iov_len = size;
    iov->iov_base = aligned_alloc(iov->iov_len, iov->iov_len);
    assert(iov->iov_base != NULL);

    /* Populate the buffer with some fixed data. */
    for (i = 0; i < size; i++) {
        *(((uint8_t *)iov->iov_base) + i) = i % 128;
    }
}

static void reportLatency(struct benchmark *benchmark,
                          time_t *latencies,
                          unsigned n)
{
    struct metric *m;
    m = BenchmarkGrow(benchmark, METRIC_KIND_LATENCY);
    MetricFillLatency(m, latencies, n);
}

static void reportThroughput(struct benchmark *benchmark,
                             time_t duration,
                             unsigned size)
{
    struct metric *m;
    unsigned megabytes = size / (1024 * 1024); /* N megabytes written */
    m = BenchmarkGrow(benchmark, METRIC_KIND_THROUGHPUT);
    MetricFillThroughput(m, megabytes, duration);
}

/* Benchmark sequential write performance. */
static int writeFile(struct diskOptions *opts,
                     bool raw,
                     struct benchmark *benchmark)
{
    struct timer timer;
    struct iovec iov;
    struct stat st;
    char *path;
    int fd;
    time_t *latencies;
    time_t duration;
    unsigned n = opts->size / (unsigned)opts->buf;
    int rv;

    assert(opts->size % opts->buf == 0);

    rv = stat(opts->dir, &st);
    if (rv != 0) {
        printf("stat '%s': %s\n", opts->dir, strerror(errno));
        return -1;
    }

    if (raw) {
        rv = FsOpenBlockDevice(opts->dir, &fd);
    } else {
        rv = FsCreateTempFile(opts->dir, n * opts->buf, &path, &fd);
    }
    if (rv != 0) {
        return -1;
    }

    if (opts->mode == DISK_MODE_DIRECT) {
        rv = FsSetDirectIO(fd);
        if (rv != 0) {
            return -1;
        }
    }

    allocBuffer(&iov, opts->buf);
    latencies = malloc(n * sizeof *latencies);
    assert(latencies != NULL);

    TimerStart(&timer);

    switch (opts->engine) {
        case DISK_ENGINE_PWRITE:
            rv = DiskWriteUsingPwrite(fd, &iov, n, latencies);
            break;
        case DISK_ENGINE_URING:
            rv = DiskWriteUsingUring(fd, &iov, n, latencies);
            break;
        case DISK_ENGINE_KAIO:
            rv = DiskWriteUsingKaio(fd, &iov, n, latencies);
            break;
        default:
            assert(0);
    }

    duration = TimerStop(&timer);

    free(iov.iov_base);

    if (rv != 0) {
        return -1;
    }

    reportLatency(benchmark, latencies, n);
    reportThroughput(benchmark, duration, opts->size);
    free(latencies);

    if (!raw) {
        rv = FsRemoveTempFile(path, fd);
        if (rv != 0) {
            return -1;
        }
    }

    return 0;
}

int DiskRun(int argc, char *argv[], struct report *report)
{
    struct diskMatrix matrix;
    unsigned i;
    int rv;

    DiskParse(argc, argv, &matrix);

    for (i = 0; i < matrix.n_opts; i++) {
        struct diskOptions *opts = &matrix.opts[i];
        struct benchmark *benchmark;
        struct stat st;
        bool raw = false; /* Raw I/O directly on a block device */
        char *name;

        rv = stat(opts->dir, &st);
        if (rv != 0) {
            printf("stat '%s': %s\n", opts->dir, strerror(errno));
            goto err;
        }

        if ((st.st_mode & S_IFMT) == S_IFBLK) {
            raw = true;
        }

        assert(opts->buf != 0);
        assert(opts->mode >= 0);

        if (!raw && opts->mode == DISK_MODE_DIRECT) {
            rv = FsCheckDirectIO(opts->dir, opts->buf);
            if (rv != 0) {
                goto err;
            }
        }

        rv = asprintf(&name, "disk:%s:%s:%zu", DiskEngineName(opts->engine),
                      DiskModeName(opts->mode), opts->buf);
        assert(rv > 0);
        assert(name != NULL);

        benchmark = ReportGrow(report, name);

        rv = writeFile(opts, raw, benchmark);
        if (rv != 0) {
            goto err;
        }
    }

    free(matrix.opts);

    return 0;

err:
    return -1;
}
