/* irfeed: hold all IR dma-buf fds of a target pid mapped, and continuously
 * write the brightest 640x512 mono8 buffer to <outpath> atomically.
 * Freestanding aarch64; raw syscalls only.
 * usage: irfeed <pid> <outpath> [fd...]
 */
typedef long i64;
typedef unsigned long u64;

static i64 sys6(long n, long a, long b, long c, long d, long e, long f) {
    register long x0 asm("x0") = a;
    register long x1 asm("x1") = b;
    register long x2 asm("x2") = c;
    register long x3 asm("x3") = d;
    register long x4 asm("x4") = e;
    register long x5 asm("x5") = f;
    register long x8 asm("x8") = n;
    asm volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8) : "memory", "cc");
    return x0;
}
static i64 sys1(long n, long a) { return sys6(n, a, 0, 0, 0, 0, 0); }
static i64 sys2(long n, long a, long b) { return sys6(n, a, b, 0, 0, 0, 0); }
static i64 sys3(long n, long a, long b, long c) { return sys6(n, a, b, c, 0, 0, 0); }
static i64 sys4(long n, long a, long b, long c, long d) { return sys6(n, a, b, c, d, 0, 0); }

#define SYS_openat 56
#define SYS_close 57
#define SYS_write 64
#define SYS_renameat 38
#define SYS_nanosleep 101
#define SYS_mmap 222
#define SYS_exit 93
#define SYS_pidfd_open 434
#define SYS_pidfd_getfd 438
#define SYS_clock_gettime 113
#define AT_FDCWD (-100L)
#define O_RDONLY 0
#define O_WRONLY 1
#define O_CREAT 0100
#define O_TRUNC 01000
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 0x20
#define CLOCK_MONOTONIC 1

#define IR_W 640
#define IR_H 512

#define MAXF 64
#define SZ 327680
#define MAPLEN 327680          /* already page-aligned: 327680 = 80*4096 */

static long slen(const char *s) { long n = 0; while (s[n]) n++; return n; }
static long pdec(const char *s) { long v = 0; while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0'); return v; }
static void pw2(const char *s) { sys3(SYS_write, 2, (long)s, slen(s)); }
static void pnum(long v) {
    char b[24]; long k = 0, t = 0; char tmp[24];
    if (v == 0) b[k++] = '0';
    if (v < 0) { b[k++] = '-'; v = -v; }
    while (v > 0) { tmp[t++] = '0' + (v % 10); v /= 10; }
    while (t > 0) b[k++] = tmp[--t];
    sys3(SYS_write, 2, (long)b, k);
}

__attribute__((naked)) void _start(void) {
    asm volatile(
        "mov x3, sp\n"
        "ldr x0, [x3]\n"
        "add x1, x3, #8\n"
        "b start_c\n");
}

void start_c(long argc, char **argv) {
    if (argc < 4) {
        pw2("usage: irfeed <pid> <outpath> [fd...]\n");
        sys1(SYS_exit, 2);
    }
    long pid = pdec(argv[1]);
    char *outp = argv[2];
    long nf = argc - 3;
    if (nf > MAXF) nf = MAXF;

    /* build tmp path = outpath + ".tmp" */
    char tmpp[128]; long k = 0;
    while (outp[k] && k < 120) { tmpp[k] = outp[k]; k++; }
    tmpp[k++] = '.'; tmpp[k++] = 't'; tmpp[k++] = 'm'; tmpp[k++] = 'p'; tmpp[k] = 0;

    /* rotated-RGBA output path = outpath + ".rgba" (reuses this tmp scheme) */
    char rgp[160]; long k2 = 0;
    while (outp[k2] && k2 < 140) { rgp[k2] = outp[k2]; k2++; }
    const char *ext = ".rgba"; for (long j = 0; ext[j]; j++) rgp[k2++] = ext[j]; rgp[k2] = 0;
    char rgtp[168]; long k3 = 0;
    while (rgp[k3] && k3 < 160) { rgtp[k3] = rgp[k3]; k3++; }
    rgtp[k3++] = '.'; rgtp[k3++] = 't'; rgtp[k3++] = 'm'; rgtp[k3++] = 'p'; rgtp[k3] = 0;
    char rgs[176]; long k4 = 0;
    while (rgp[k4] && k4 < 165) { rgs[k4] = rgp[k4]; k4++; }
    const char *se = ".seq"; for (long j = 0; se[j]; j++) rgs[k4++] = se[j]; rgs[k4] = 0;
    unsigned long long gseq = 0;
#ifdef IRFEED_RAW_ONLY
    char stamp_path[160], stamp_tmp[168];
    long sk = 0;
    while (outp[sk] && sk < 140) { stamp_path[sk] = outp[sk]; sk++; }
    const char *stamp_ext = ".stamp";
    for (long j = 0; stamp_ext[j]; j++) stamp_path[sk++] = stamp_ext[j];
    stamp_path[sk] = 0;
    for (long j = 0; j <= sk; j++) stamp_tmp[j] = stamp_path[j];
    stamp_tmp[sk++] = '.'; stamp_tmp[sk++] = 't'; stamp_tmp[sk++] = 'm'; stamp_tmp[sk++] = 'p'; stamp_tmp[sk] = 0;
#endif
    /* The system compositor rotates on the GPU and only needs mono8. Keep the
     * original RGBA output for the separate PC viewer build. */
#ifdef IRFEED_RAW_ONLY
    unsigned char *rgba = 0;
#else
    unsigned char *rgba = (unsigned char *)sys6(SYS_mmap, 0, SZ * 4, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    if ((long)rgba < 0) rgba = 0;

    i64 pidfd = sys2(SYS_pidfd_open, pid, 0);

    unsigned char *maps[MAXF];
    for (long i = 0; i < nf; i++) {
        long tfd = pdec(argv[3 + i]);
        i64 fd = -1;
        if (pidfd >= 0) fd = sys4(SYS_pidfd_getfd, pidfd, tfd, 0, 0);
        if (fd < 0) {
            char p[64]; long m = 0;
            const char *pre = "/proc/"; const char *mid = "/fd/";
            for (long j = 0; pre[j]; j++) p[m++] = pre[j];
            for (long j = 0; argv[1][j]; j++) p[m++] = argv[1][j];
            for (long j = 0; mid[j]; j++) p[m++] = mid[j];
            for (long j = 0; argv[3 + i][j]; j++) p[m++] = argv[3 + i][j];
            p[m] = 0;
            fd = sys4(SYS_openat, AT_FDCWD, (long)p, O_RDONLY, 0);
        }
        if (fd < 0) { maps[i] = 0; continue; }
        i64 b = sys6(SYS_mmap, 0, MAPLEN, PROT_READ, MAP_SHARED, fd, 0);
        maps[i] = (b < 0) ? 0 : (unsigned char *)b;
    }
    pw2("irfeed: pid="); pnum(pid); pw2(" fds="); pnum(nf); pw2("\n");

    /* timespec for 8ms sleep */
    long ts[2]; ts[0] = 0;
#ifdef IRFEED_RAW_ONLY
    ts[1] = 2000000; /* reduce publication jitter at a 60 Hz sensor cadence */
#else
    ts[1] = 8000000;
#endif

    unsigned long prev[MAXF]; long have[MAXF];
#ifdef IRFEED_RAW_ONLY
    long pending[MAXF] = {0};
    unsigned long long changed_at[MAXF] = {0};
#endif
    for (long i = 0; i < MAXF; i++) { prev[i] = 0; have[i] = 0; }
    unsigned long iters = 0; unsigned long good = 0; long drop = 0;
    for (;;) {
#ifdef IRFEED_RAW_ONLY
        long clock[2];
        sys2(SYS_clock_gettime, CLOCK_MONOTONIC, (long)clock);
        unsigned long long observed = (unsigned long long)clock[0] * 1000000000ULL + clock[1];
#endif
        long pick = -1; unsigned long ps = 0;
        for (long i = 0; i < nf; i++) {
            unsigned char *m = maps[i];
            if (!m) continue;
            unsigned long s = 0;
            for (long j = 0; j < SZ; j += 64) s += m[j];
#ifdef IRFEED_RAW_ONLY
            /* Do not publish a DMA buffer while its sampled pixels still
             * change. One stable polling interval avoids treating partial
             * writes as extra camera frames at the faster polling rate. */
            if (!have[i] || s != prev[i]) {
                prev[i] = s; have[i] = 1; pending[i] = 1;
                changed_at[i] = observed;
                continue;
            }
            if (!pending[i]) continue;
            pending[i] = 0;
#else
            if (have[i] && s == prev[i]) continue;   /* unchanged -> stale, skip */
            prev[i] = s; have[i] = 1;
#endif
            if (s > ps) { ps = s; pick = i; }        /* freshest frame this tick */
        }
        if (pick < 0) { sys6(SYS_nanosleep, (long)ts, 0, 0, 0, 0, 0); continue; }

        /* hold-last-good: if the freshly-arrived frame is a dark idle burst,
         * keep the previous output instead of flashing black. Force accept
         * after ~1.6s so a genuinely dark scene still comes through. */
        if (good == 0 || ps > good) good = ps;
        if (ps * 2 < good && drop < 200) {
            drop++; iters++;
            sys6(SYS_nanosleep, (long)ts, 0, 0, 0, 0, 0);
            continue;
        }
        drop = 0;
        good = (good * 15 + ps) / 16;

        unsigned char *src = maps[pick];
        i64 ofd = sys4(SYS_openat, AT_FDCWD, (long)tmpp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (ofd >= 0) {
            long total = 0;
            while (total < SZ) {
                i64 w = sys3(SYS_write, ofd, (long)(src + total), SZ - total);
                if (w <= 0) break;
                total += w;
            }
            sys1(SYS_close, ofd);
            if (total == SZ) {
                long renamed = sys4(SYS_renameat, AT_FDCWD, (long)tmpp, AT_FDCWD, (long)outp);
#ifdef IRFEED_RAW_ONLY
                if (renamed == 0) {
                    gseq++;
                    /* Time of the last observed DMA change, not a hardware
                     * exposure timestamp. Pair it with this publication. */
                    unsigned long long stamp[2] = {gseq, changed_at[pick]};
                    i64 stampfd = sys4(SYS_openat, AT_FDCWD, (long)stamp_tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (stampfd >= 0) {
                        long written = sys3(SYS_write, stampfd, (long)stamp, 16);
                        sys1(SYS_close, stampfd);
                        if (written == 16) sys4(SYS_renameat, AT_FDCWD, (long)stamp_tmp, AT_FDCWD, (long)stamp_path);
                    }
                    i64 sfd = sys4(SYS_openat, AT_FDCWD, (long)rgs, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (sfd >= 0) { sys3(SYS_write, sfd, (long)&gseq, 8); sys1(SYS_close, sfd); }
                }
#endif
            }
        }
        if (rgba) {
            unsigned int *dst = (unsigned int *)rgba;
            for (long r = 0; r < IR_H; r++) {
                long row = r * IR_W;
                for (long c = 0; c < IR_W; c++) {
                    unsigned int v = src[row + c];
                    dst[(IR_W - 1 - c) * IR_H + (IR_H - 1 - r)] = 0xff000000u | (v << 16) | (v << 8) | v;
                }
            }
            long rlen = (long)SZ * 4;
            i64 rofd = sys4(SYS_openat, AT_FDCWD, (long)rgtp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (rofd >= 0) {
                long rt = 0;
                while (rt < rlen) {
                    i64 w = sys3(SYS_write, rofd, (long)(rgba + rt), rlen - rt);
                    if (w <= 0) break;
                    rt += w;
                }
                sys1(SYS_close, rofd);
                sys4(SYS_renameat, AT_FDCWD, (long)rgtp, AT_FDCWD, (long)rgp);
                gseq++;
                i64 sfd = sys4(SYS_openat, AT_FDCWD, (long)rgs, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (sfd >= 0) { sys3(SYS_write, sfd, (long)&gseq, 8); sys1(SYS_close, sfd); }
            }
        }
        iters++;
        if ((iters % 300) == 0) { pw2("irfeed: it="); pnum(iters); pw2(" pick="); pnum(pick); pw2(" score="); pnum((long)ps); pw2("\n"); }
        sys6(SYS_nanosleep, (long)ts, 0, 0, 0, 0, 0);
    }
}
