
#include"data_struct_def.h"
#include "event_param.h"
int preprocess_data(SampleData *sample);

int spo2_init();

void spo2_exit();

void spo2_reset();

int spo2_process(PpgData *ppg, int *spo2);