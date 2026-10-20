/* Execute the actual SETBOOT payload on emu2's 8086 CPU core.
 * BIOS, BDOS, XIOS and disks below are entirely synthetic/in-memory.
 * The emu2 source is included read-only, never copied or modified.
 */
#include <assert.h>
#include <stdarg.h>
#include EMU_CPU_SOURCE
uint8_t memory[0x100000];
volatile int exit_cpu;
int opt_fullscreen;
char *prog_name = "setboot-test";
void print_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(2);
}
void debug(enum debug_type t, const char *fmt, ...) {
    (void)t;
    (void)fmt;
}
int debug_active(enum debug_type t) {
    (void)t;
    return 0;
}
const char *disa(const uint8_t *a, uint16_t b, int c) {
    (void)a;
    (void)b;
    (void)c;
    return "";
}
void emu_get_time(EMU_CLOCK_TYPE *t) { memset(t, 0, sizeof(*t)); }
void emu_advance_time(int u, EMU_CLOCK_TYPE *t) {
    (void)u;
    (void)t;
}
int emu_compare_time(EMU_CLOCK_TYPE *t) {
    (void)t;
    return 1;
}
void emulator_update(void) {}
uint8_t read_port(unsigned p) {
    (void)p;
    abort();
}
void write_port(unsigned p, uint8_t v) {
    (void)p;
    (void)v;
    abort();
}
void bios_routine(unsigned n) {
    fprintf(stderr, "unexpected emulator BIOS %u\n", n);
    abort();
}

static unsigned sys = 0x60000, pd = 0x200, vc = 0x300, uda = 0x7000;
static uint8_t disk[512];
static int mode, failread, failwrite, mismatch, absent, busy, queuefail,
    releasefail;
static int nmis, os_calls;
static unsigned reboot_flag;
static int reads, writes, resets, rebooted, locked, lock_calls, unlock_calls,
    flushes, quit, exitcode, had_write;
static int race, console_busy, remote, priority_calls, expected_unit = 0x80;
static unsigned base = 0x10130;
static char output[16384];
static unsigned outlen;
static void emit(unsigned a) {
    while (memory[a] != '$') {
        assert(outlen + 1 < sizeof(output));
        output[outlen++] = memory[a++];
    }
    output[outlen] = 0;
}
static void disk_bios(void) {
    unsigned fn = wregs[AX] >> 8;
    if (mode == 2) {
        assert(locked);
        assert(memory[sys + vc + 14] & 8);
        assert(get16(sys + pd + 6) & 2);
        assert(memory[sys + pd + 5] == 150);
        assert(!IF);
    }
    if (fn == 0) {
        resets++;
        CF = 0;
        return;
    }
    assert(fn == 2 || fn == 3);
    assert(wregs[CX] == 1);
    assert((wregs[DX] >> 8) == 0);
    assert((wregs[DX] & 255) == expected_unit);
    assert((wregs[AX] & 255) == 1);
    unsigned addr = sregs[ES] * 16 + wregs[BX];
    assert((addr & 65535) <= 65024);
    assert(addr >= base + 256 && addr + 512 <= base + 0x10000);
    if (fn == 2)
        reads++;
    else
        writes++;
    if (absent ||
        (fn == 2 && (failread == 1 || (failread == 2 && had_write))) ||
        (fn == 3 && failwrite)) {
        wregs[AX] = 0x400;
        CF = 1;
        return;
    }
    if (fn == 2) {
        memcpy(memory + addr, disk, 512);
        if (mismatch && had_write)
            memory[addr + 10] ^= 1;
    } else {
        memcpy(disk, memory + addr, 512);
        had_write = 1;
    }
    wregs[AX] = 1;
    CF = 0;
}
static void os_call(int dos) {
    os_calls++;
    unsigned fn = dos ? wregs[AX] >> 8 : wregs[CX] & 255;
    assert((dos && mode == 0) || (!dos && mode != 0));
    switch (fn) {
    case 0:
        quit = 1;
        break;
    case 2:
        assert(outlen + 1 < sizeof(output));
        output[outlen++] = wregs[DX] & 255;
        output[outlen] = 0;
        break;
    case 9:
        emit(sregs[DS] * 16 + wregs[DX]);
        break;
    case 12:
        assert(!dos);
        wregs[AX] = mode == 1 ? 0x22 : 0x1422;
        break;
    case 13:
        flushes++;
        break;
    case 0x4c:
        assert(dos);
        exitcode = wregs[AX] & 255;
        quit = 1;
        break;
    case 135:
        assert(mode == 2);
        assert(!memcmp(memory + sregs[DS] * 16 + wregs[DX] + 8, "MXdisk  ", 8));
        wregs[AX] = queuefail == 1 ? 1 : 0;
        break;
    case 137:
        assert(mode == 2);
        lock_calls++;
        wregs[AX] = queuefail == 2 ? 1 : 0;
        if (!wregs[AX])
            locked = 1;
        if (race)
            put16(sys + 0x88, 1);
        break;
    case 139:
        assert(mode == 2);
        assert(locked);
        unlock_calls++;
        wregs[AX] = releasefail ? 1 : 0;
        if (!releasefail)
            locked = 0;
        break;
    case 145:
        assert(mode == 2);
        memory[sys + pd + 5] = wregs[DX] & 255;
        priority_calls++;
        break;
    case 153:
        wregs[AX] = remote ? 5 : 0;
        break;
    case 154:
        sregs[ES] = sys / 16;
        wregs[BX] = 0;
        break;
    case 156:
        sregs[ES] = sys / 16;
        wregs[BX] = pd;
        break;
    case 163:
        wregs[AX] = 0x1431;
        break;
    default:
        fprintf(stderr, "unexpected OS call %u\n", fn);
        abort();
    }
    /* Exercise the output helpers with volatile numeric registers. */
    if (!dos && (fn == 2 || fn == 9)) {
        wregs[AX] = 0xbeef;
        wregs[BX] = 0xbeef;
        wregs[CX] = 0xcafe;
        wregs[DX] = 0xface;
    }
    /* Deliberately trash ES after ordinary BDOS calls, like real BDOS may. */
    if (!dos && fn != 154 && fn != 156)
        sregs[ES] = 0xdead;
}
static void far_return(void) {
    ip = PopWord();
    sregs[CS] = PopWord();
}
static void xios_call(void) {
    assert(mode == 2);
    assert(sregs[DS] == sys / 16);
    assert(sregs[ES] == uda);
    assert((wregs[AX] & 255) == 14);
    wregs[AX] = wregs[CX] == 0 ? 0x500 : 0x600;
    wregs[BX] = wregs[AX];
    far_return();
}
int main(int argc, char **argv) {
    assert(argc ==
           7); /* payload, mode, tail, input MBR, output MBR, scenario */
    mode = atoi(argv[2]);
    const char *scenario = argv[6];
    failread = !strcmp(scenario, "readfail")     ? 1
               : !strcmp(scenario, "verifyread") ? 2
                                                 : 0;
    failwrite = !strcmp(scenario, "writefail");
    mismatch = !strcmp(scenario, "mismatch");
    absent = !strcmp(scenario, "absent");
    busy = !strcmp(scenario, "busy");
    race = !strcmp(scenario, "race");
    console_busy = !strcmp(scenario, "consolebusy");
    remote = !strcmp(scenario, "remote");
    queuefail = !strcmp(scenario, "queueopen")   ? 1
                : !strcmp(scenario, "queueread") ? 2
                                                 : 0;
    releasefail = !strcmp(scenario, "queuerelease");
    if (!strcmp(scenario, "disk2"))
        expected_unit = 0x81;
    if (!strcmp(scenario, "disk128"))
        expected_unit = 0xff;
    if (!strcmp(scenario, "boundary"))
        base = 0x1fef0;
    memset(memory, 0xa5, sizeof(memory));
    init_cpu();
    cpuSetLevel(1); /* Reject 186+ instructions. */
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    unsigned len = fread(memory + base + 256, 1, 60000, f);
    fclose(f);
    assert(len > 0);
    memset(memory + base, 0, 256);
    if (mode == 0) {
        put16(base, 0x20cd);
        memory[base + 5] = 0x9a;
    } else {
        put16(base, len + 256);
        put16(base + 3, base / 16);
        memory[base + 5] = 1;
    }
    unsigned n = strlen(argv[3]);
    assert(n <= 127);
    memory[base + 128] = n;
    memcpy(memory + base + 129, argv[3], n);
    if (n < 127)
        memory[base + 129 + n] = 13;
    f = fopen(argv[4], "rb");
    assert(f);
    assert(fread(disk, 1, 512, f) == 512);
    fclose(f);
    memset(memory + sys, 0, 0x1000);
    put16(sys + 0x28, 0x100);
    put16(sys + 0x2a, 0xe000);
    put16(sys + 0x54, vc);
    put16(sys + pd + 0x10, uda);
    memory[sys + pd + 5] = 200;
    put16(sys + pd + 6, 0x80);
    put16(sys + vc + 14, console_busy ? 1 : 0);
    put16(sys + 0x88, busy ? 1 : 0);
    put16(sys + 0x500, 2);
    put16(sys + 0x502, 2);
    put16(sys + 0x600, 0x100);
    put16(sys + 0x602, 0xf000);
    put16(0x380, 0x100);
    put16(0x382, 0xe000);
    if (!strcmp(scenario, "bad_prefix"))
        put16(base, 0);
    if (!strcmp(scenario, "bad_model"))
        memory[base + 5] = 0;
    if (!strcmp(scenario, "bad_code"))
        put16(base + 3, 0x1234);
    if (!strcmp(scenario, "null_e0"))
        put32(0x380, 0);
    if (!strcmp(scenario, "other_call5"))
        memory[base + 5] = 0xea;
    for (unsigned v = 0; v < 4; v++)
        sregs[v] = base / 16;
    if (!strcmp(scenario, "bad_ds"))
        sregs[DS] = 0x1234;
    if (!strcmp(scenario, "other_es"))
        sregs[ES] = 0x1234;
    sregs[SS] = 0x8000;
    wregs[SP] = 0x1234;
    ip = 256;
    IF = 1;
    segment_override = NoSeg;
    for (unsigned step = 0; step < 1000000 && !quit; step++) {
        if (sregs[CS] == 0xffff && ip == 0) {
            rebooted = 1;
            reboot_flag = get16(0x472);
            assert(reboot_flag == 0 || reboot_flag == 0x1234);
            assert(!IF);
            quit = 1;
            break;
        }
        if (sregs[CS] == 0xe000 && ip == 0x100) {
            xios_call();
            continue;
        }
        if (sregs[CS] == 0xf000 && ip == 0x100) {
            disk_bios();
            unsigned carry = CF;
            far_return();
            unsigned flags = PopWord();
            ExpandFlags(flags);
            CF = carry;
            continue;
        }
        unsigned a = sregs[CS] * 16 + ip;
        if (memory[a] == 0xcd) {
            unsigned intr = memory[a + 1];
            ip += 2;
            if (intr == 2) {
                nmis++;
                quit = 1;
            } else if (intr == 0x21 || intr == 0xe0)
                os_call(intr == 0x21);
            else if (intr == 0x20) {
                assert(mode == 0);
                quit = 1;
            } else if (intr == 0x13) {
                assert(mode != 2);
                disk_bios();
            } else
                abort();
            continue;
        }
        start_ip = ip;
        do_instruction_limited(FETCH_B());
    }
    assert(quit);
    if (nmis)
        assert(!os_calls && !reads && !writes && !rebooted);
    if (mode == 2) {
        assert(memory[sys + pd + 5] == 200);
        assert(get16(sys + pd + 6) == 0x80);
        assert(get16(sys + vc + 14) == (console_busy ? 1 : 0));
        assert(!locked || releasefail);
    }
    if (rebooted)
        assert(!failread && !failwrite && !mismatch);
    f = fopen(argv[5], "wb");
    assert(f);
    assert(fwrite(disk, 1, 512, f) == 512);
    fclose(f);
    printf("nmi=%d reads=%d writes=%d resets=%d reboot=%d locks=%d unlocks=%d "
           "flushes=%d code=%d resetflag=%u\n",
           nmis, reads, writes, resets, rebooted, lock_calls, unlock_calls,
           flushes, exitcode, reboot_flag);
    printf("%s", output);
    return 0;
}
