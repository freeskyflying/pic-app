#ifndef _ADC_H_
#define _ADC_H_

typedef struct
{

	unsigned int powerVolt;
	unsigned int sosKey;

} Adc_Result_t;

void adcDetectStart(void);

extern Adc_Result_t * gAdcResult;

#endif

