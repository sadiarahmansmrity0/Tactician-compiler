bool ready;
ready = true;
{
    int inner;
    inner = 2;
    if (ready) {
        print inner;
    }
}
if (ready == true) {
    print ready;
} else {
    print false;
}
while (!ready) {
    ready = true;
}
