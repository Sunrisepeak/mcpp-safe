import legacy [[mcpp::allow("c-array, c-varargs, raw-pointers, union", "the C logging library, wrapped in app::log")]];   // expect-waived: c-array c-varargs raw-pointers union

int waived_value() { return 3; }
