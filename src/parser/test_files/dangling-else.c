int classify(int x, int y) {
    int r = 0;

    if (x > 0)
        if (y > 0)
            r = 1;
        else
            r = 2;

    return r;
}
