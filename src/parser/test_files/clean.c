class Point {
public:
    int x;
    int y;

    int sum(int a, int b) {
        int r = a + b;
        return r;
    }

    class Meta {
    private:
        int tag;
    };
};

int run() {
    int total = 0;
    int p = new Point;
    int q = new Point(1, 2);

    int i = 0;
    while (i < 10) {
        total = total + i * 2 - 1;
        i = i + 1;
    }

    for (int j = 0; j < 5; j = j + 1) {
        if (j < 3) {
            total = total + j;
        } else {
            total = total - j;
        }
    }

    delete p;
    delete q;
    return total;
}