import app.table;   // its own dialect denies C arrays: what it exports it waived itself

int trusted(int i) { return app::lookup(i); }
