int count;
float ratio;
bool active;
bool ready;

count = 2 + 3 * 4;
ratio = count / 2.0;
active = !(count < 20) || false;
ready = active && count != 0;

{
    int count;
    count = 1;
    if (count < 2) {
        print count;
        { bool inner; inner = true; print inner; }
    } else {
        print ratio;
    }
}

while (count > 0 && ready) {
    count = count - 1;
    print count;
}
print ratio;
