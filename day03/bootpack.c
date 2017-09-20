
void io_hlt(void); //hlt

void HariMain(void) {
 fin:
    io_hlt();
    goto fin;
}
