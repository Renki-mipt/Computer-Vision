#include <iostream>
#include <string>
#include <vector>
#include <limits.h>
#include <cstdio>
#include <fstream>

bool running = true;

typedef void (*functions16)(unsigned short i);
typedef void (*trap_functions)();

enum regist { R0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, RCNT };
unsigned short mem[UINT16_MAX] = { 0 };
unsigned short reg[RCNT] = { 0 };

unsigned short sext(unsigned short n, int b) { return ((n >> (b - 1)) & 1) ? (n | (0xFFFF << b)) : n; }

enum flags { FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2 };
void uf(int r) {
    if (reg[r] == 0) reg[RCND] = FZ;
    else if (reg[r] >> 15) reg[RCND] = FN;
    else reg[RCND] = FP;
}

unsigned short mr(unsigned short address) { return mem[address]; }
void mw(unsigned short address, unsigned short val) { mem[address] = val; }

#define DR(i) (((i)>>9)&0x7)
#define SR1(i) (((i)>>6)&0x7)
#define SR2(i) ((i)&0x7)
#define FIMM(i) ((i>>5)&01)
#define IMM(i) ((i)&0x1F)
#define POFF(i) sext((i)&0x3F, 6)
#define POFF9(i) sext((i)&0x1FF, 9)
#define POFF11(i) sext((i)&0x7FF, 11)

void add(unsigned short i) { reg[DR(i)] = reg[SR1(i)] + (FIMM(i) ? sext(IMM(i), 5) : reg[SR2(i)]); uf(DR(i)); }
void and_ (unsigned short i) { reg[DR(i)] = reg[SR1(i)] & (FIMM(i) ? sext(IMM(i), 5) : reg[SR2(i)]); uf(DR(i)); }
void ldi(unsigned short i) { reg[DR(i)] = mr(mr(reg[RPC] + POFF9(i))); uf(DR(i)); }
void not_ (unsigned short i) { reg[DR(i)] = ~reg[SR1(i)]; uf(DR(i)); }
void br(unsigned short i) { if (reg[RCND] & ((i) >> 9) & 0x7) { reg[RPC] += POFF9(i); } }
void jsr(unsigned short i) { reg[R7] = reg[RPC]; reg[RPC] = (((i) >> 11) & 1) ? reg[RPC] + POFF11(i) : reg[((i) >> 6) & 0x7]; }
void jmp(unsigned short i) { reg[RPC] = reg[((i) >> 6) & 0x7]; }
void ld(unsigned short i) { reg[DR(i)] = mr(reg[RPC] + POFF9(i)); uf(DR(i)); }
void ldr(unsigned short i) { reg[DR(i)] = mr(reg[SR1(i)] + POFF(i)); uf(DR(i)); }
void lea(unsigned short i) { reg[DR(i)] = reg[RPC] + POFF9(i); uf(DR(i)); }
void st(unsigned short i) { mw(reg[RPC] + POFF9(i), reg[DR(i)]); }
void sti(unsigned short i) { mw(mr(reg[RPC] + POFF9(i)), reg[DR(i)]); }
void str(unsigned short i) { mw(reg[SR1(i)] + POFF(i), reg[DR(i)]); }
void rti(unsigned short i) {}
void res(unsigned short i) {}
void tgetc() { reg[R0] = getchar(); }
void tout() { std::cout << reg[R0]; }
void tputs() {
    unsigned short* p = mem + reg[R0];
    while (*p) {
        char ch = (char)*p;
        std::cout << ch;
        p++;
    }
}
void tin() { reg[R0] = getchar(); std::cout << reg[R0]; }
void thalt() { running = false; }
void tinu16() { std::cin >> reg[R0]; }
void toutu16() { std::cout << reg[R0] << '\n'; }
trap_functions trap_fs[8] = { tgetc, tout, tputs, tin, thalt, tinu16, toutu16 };
void trap(unsigned short i) { trap_fs[(i) & 0xFF - 0x20](); }
functions16 funcs[16] = { br, add, ld, st, jsr, and_, ldr, str, rti, not_, ldi, sti, jmp, res, lea, trap };

int main() {
    std::string file_name;
    std::cin >> file_name;
    //read programm
    std::ifstream in(file_name);
    if (in.is_open())
    {
        int shift = 0;
        unsigned short x;
        while (in >> std::hex >> x)
        {
            if (shift == UINT16_MAX) {
                break;
            }
            mem[shift++] = x;
        }
    }
    in.close();
    //start programm
    reg[RPC] = 0;
    while (running) {
        unsigned short i = mr(reg[RPC]++);
        funcs[(i) >> 12](i);
    }
    return 0;
}