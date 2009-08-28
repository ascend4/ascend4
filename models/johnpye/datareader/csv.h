#ifndef DATAREADER_CSV_H
#define DATAREADER_CSV_H

#include "dr.h"

DataReaderDataFn datareader_csv_data;
DataReaderHeaderFn datareader_csv_header;
DataReaderEofFn datareader_csv_eof;
DataReaderIndepFn datareader_csv_time;
DataReaderValFn datareader_csv_vals;

#endif
