int total = 0;
int count = 10;
char flag;

int compute(int a, int b) {
    int result = a + b * 2 - 1;
    return result;
}

int run() {
    int i = 0;
    while (i < count) {
        total = total + i;
        i = i + 1;
    }

    for (int j = 0; j < count; j = j + 1) {
        if (j < 5) {
            total = total + j;
        } else {
            total = total - j;
        }
    }

    int x = compute(total, count);
    return x;
}