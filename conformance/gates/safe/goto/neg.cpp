int loop(int n) { while (n > 0) --n; return n; }
int early(int n) { if (n) return 1; return 0; }
int breaks(int n) { for (;;) { if (n-- == 0) break; } return n; }
int labelled_switch(int n) { switch (n) { case 1: return 1; default: return 0; } }
int go_to_named(int go_to) { return go_to; }
