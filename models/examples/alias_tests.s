DELETE TYPES;
READ FILE alias_tests.asc;

COMPILE at OF alii_of_array;
BROWSE {at};

COMPILE dt OF dense_array_of_alii;
BROWSE {dt};
ASSIGN {dt.n} 3 {};
RESUME {dt};

COMPILE mt OF mass;
BROWSE {mt};

COMPILE ot OF order_of_sparse;
BROWSE {ot};

COMPILE sat OF simple_alias;
BROWSE {sat};

COMPILE spt OF sparse_array_of_alii;
BROWSE {spt};


