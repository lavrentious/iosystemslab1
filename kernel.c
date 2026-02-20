extern char __bss[], __bss_end[], __stack_top[];

#define SBI_CONSOLE_PUTCHAR 0x01
#define SBI_CONSOLE_GETCHAR 0x02
#define SBI_SHUTDOWN 0x08

struct sbiret {
  long error;
  union {
    long value;
    unsigned long uvalue;
  };
};

void putchar(char c) {
  __asm__ __volatile__("li a7, %0\n"
                       "li a6, 0\n"
                       "mv a0, %1\n"
                       "ecall\n"
                       :
                       : "i"(SBI_CONSOLE_PUTCHAR), "r"(c)
                       : "a0", "a6", "a7", "memory");
}

char getchar(void) {
  int c;
  do {
    __asm__ __volatile__("li a7, %1\n"
                         "li a6, 0\n"
                         "ecall\n"
                         "mv %0, a0\n"
                         : "=r"(c)
                         : "i"(SBI_CONSOLE_GETCHAR)
                         : "a0", "a6", "a7", "memory");
  } while (c == -1);
  return (char)c;
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

void print_int(long num) {
  if (num == 0) {
    putchar('0');
    return;
  }

  char buf[32];
  int i = 0;

  while (num > 0) {
    buf[i++] = (num % 10) + '0';
    num /= 10;
  }

  while (i > 0) {
    putchar(buf[--i]);
  }
}

long read_int(void) {
  long num = 0;
  while (1) {
    char c = getchar();

    if (c == '\r' || c == '\n') {
      putchar('\n');
      break;
    }

    if (c >= '0' && c <= '9') {
      putchar(c);
      num = num * 10 + (c - '0');
    }
  }
  return num;
}

// help
void print_help(void) {
  println("Available commands:");
  println("1: Get SBI specification version");
  println("2: Get number of counters");
  println("3: Get details of a counter");
  println("4: System Shutdown");
}

// sbi calls
struct sbiret get_sbi_spec_version(void) {
  register long a0 __asm__("a0");
  register long a1 __asm__("a1");
  register long a6 __asm__("a6") = 0;
  register long a7 __asm__("a7") = 0x10;

  __asm__ __volatile__("ecall"
                       : "=r"(a0), "=r"(a1)
                       : "r"(a6), "r"(a7)
                       : "memory");

  struct sbiret ret;
  ret.error = a0;
  ret.value = a1;
  return ret;
}

struct sbiret get_pmu_num_counters(void) {
  register long a0 __asm__("a0");
  register long a1 __asm__("a1");
  register long a6 __asm__("a6") = 0;
  register long a7 __asm__("a7") = 0x504D55;

  __asm__ __volatile__("ecall"
                       : "=r"(a0), "=r"(a1)
                       : "r"(a6), "r"(a7)
                       : "memory");

  struct sbiret ret;
  ret.error = a0;
  ret.value = a1;
  return ret;
}

struct sbiret get_pmu_counter_info(unsigned long counter_idx) {
  register long a0 __asm__("a0") = counter_idx;
  register long a1 __asm__("a1");
  register long a6 __asm__("a6") = 1;
  register long a7 __asm__("a7") = 0x504D55;

  __asm__ __volatile__("ecall"
                       : "+r"(a0), "=r"(a1)
                       : "r"(a6), "r"(a7)
                       : "memory");

  struct sbiret ret;
  ret.error = a0;
  ret.value = a1;
  return ret;
}

void shutdown(void) {
  register long a6 __asm__("a6") = 0;
  register long a7 __asm__("a7") = SBI_SHUTDOWN;
  __asm__ __volatile__("ecall\n" : : "r"(a6), "r"(a7) : "memory");
}

void kernel_main(void) {
  println("HI!");
  print_help();

  while (1) {
    char c = getchar();
    if (c == '1') {
      struct sbiret ret = get_sbi_spec_version();

      long minor = ret.value & 0xFFFFFF;
      long major = (ret.value >> 24) & 0x7F;

      print("sbi specification version: ");
      print_int(major);
      putchar('.');
      print_int(minor);
      putchar('\n');

    } else if (c == '2') {
      struct sbiret ret = get_pmu_num_counters();
      print("number of pmu counters: ");
      print_int(ret.value);
      putchar('\n');
    } else if (c == '3') {
      print("\r\nenter counter idx: ");
      long idx = read_int();

      struct sbiret ret = get_pmu_counter_info(idx);
      if (ret.error < 0) {
        print("invalid counter idx\n");
      } else {
        print("counter info: ");
        print_int(ret.value);
        putchar('\n');
      }
    } else if (c == '4') {
      println("shutting down...");
      shutdown();
    } else if (c == 'h') {
      print_help();
    } else {
      print("invalid command: ");
      putchar(c);
      print(" (enter 'h' for help)\n");
    }
  }
}

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
  __asm__ __volatile__("mv sp, %[stack_top]\n"
                       "j kernel_main\n"
                       :
                       : [stack_top] "r"(__stack_top));
}
