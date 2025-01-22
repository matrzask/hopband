#include "esp_adc/adc_oneshot.h"
#include <math.h>
#include "esp_err.h"
#include "esp_log.h"

#define analogPin ADC_CHANNEL_6
#define beta 3950
#define referenceResistor 10000 // Reference resistor value in ohms
#define nominalResistance 8000  // Resistance at 25 degrees C
#define nominalTemperature 25   // Temperature for nominal resistance (in Celsius)
#define adcMaxValue 4095        // Maximum ADC value for 12-bit resolution
#define adcVoltage 3.3          // ADC reference voltage

float getTempReading()
{
    // ADC configuration
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, analogPin, &config);

    // Read ADC value
    int adcValue;
    adc_oneshot_read(adc1_handle, analogPin, &adcValue);

    // Deinitialize ADC
    adc_oneshot_del_unit(adc1_handle);

    // Convert ADC value to resistance
    float resistance = referenceResistor * (adcValue / (float)(adcMaxValue - adcValue));

    // Calculate temperature using the Steinhart-Hart equation
    float steinhart;
    steinhart = resistance / nominalResistance;       // (R/Ro)
    steinhart = log(steinhart);                       // ln(R/Ro)
    steinhart /= beta;                                // 1/B * ln(R/Ro)
    steinhart += 1.0 / (nominalTemperature + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                      // Invert
    steinhart -= 273.15;                              // Convert to Celsius

    return steinhart;
}