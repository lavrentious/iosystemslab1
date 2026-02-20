extern char __bss[], __bss_end[], __stack_top[];

#define SBI_CONSOLE_PUTCHAR 0x01
#define SBI_CONSOLE_GETCHAR 0x02
#define SBI_SHUTDOWN 0x08


void putchar(char c) {
    __asm__ __volatile__ (
        "li a7, %0\n"
        "li a6, 0\n"
        "mv a0, %1\n"
        "ecall\n"
        : 
        : "i"(SBI_CONSOLE_PUTCHAR), "r"(c)
        : "a0", "a6", "a7", "memory"
    );
}

void print(const char *str) {
    for (int i = 0; str[i]; i++) {
        putchar(str[i]);
    }
}

void println(const char *str) {
    print(str);
    putchar('\n');
}


char getchar(void) {
    int c; 
    do {
        __asm__ __volatile__ (
            "li a7, %1\n"
            "li a6, 0\n"
            "ecall\n"
            "mv %0, a0\n"
            : "=r"(c)
            : "i"(SBI_CONSOLE_GETCHAR)
            : "a0", "a6", "a7", "memory"
        );
    } while (c == -1);
    return (char)c;
}

void kernel_main(void) {

    println("HI!");
    
    for (;;);
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j kernel_main\n"
        : 
        : [stack_top] "r" (__stack_top)
    );
}
