int a;
bool b;
a = 0;
b = false;
while (a < 4) {
    if (!(a == 2)) { a = a + 1; } else { a = a + 2; }
    while (b || a < 0) { b = !b; }
}
