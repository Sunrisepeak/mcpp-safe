import legacy [[mcpp::allow("c-array")]];   // expect: c-varargs raw-pointers union; expect-waived: c-array

int partial_value() { return 2; }
