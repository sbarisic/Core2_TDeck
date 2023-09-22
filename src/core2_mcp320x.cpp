#include <core2.h>

#if !defined(CORE2_DISABLE_MCP320X)
#include <Mcp320x.h>

SPISettings spi_settings;
SemaphoreHandle_t lock = NULL;

MCP3201 adc(MCP320X_ADC_VREF, MCP320X_CS_CHANNEL1); // ovisno o tipu AD konvertera MCP 3201,3202,3204,3208
MCP3201 adc1(MCP320X_ADC_VREF, MCP320X_CS_CHANNEL2);
#endif

void core2_adc_chipselect_enable(core2_adc_channel_t Ch)
{
    if (Ch & CORE2_ADC_CH1)
        digitalWrite(MCP320X_CS_CHANNEL1, HIGH);

    if (Ch & CORE2_ADC_CH2)
        digitalWrite(MCP320X_CS_CHANNEL2, HIGH);
}

void core2_adc_chipselect_enable_all()
{
    digitalWrite(MCP320X_CS_CHANNEL1, HIGH);
    digitalWrite(MCP320X_CS_CHANNEL2, HIGH);
}

void core2_adc_chipselect_disable(core2_adc_channel_t Ch)
{
    if (Ch & CORE2_ADC_CH1)
        digitalWrite(MCP320X_CS_CHANNEL1, LOW);

    if (Ch & CORE2_ADC_CH2)
        digitalWrite(MCP320X_CS_CHANNEL2, LOW);
}

void core2_adc_chipselect_disable_all()
{
    digitalWrite(MCP320X_CS_CHANNEL1, LOW);
    digitalWrite(MCP320X_CS_CHANNEL2, LOW);
}

bool core2_adc_lock()
{
#if defined(CORE2_DISABLE_MCP320X)
    return false;
#else
    return core2_lock_begin(lock);
#endif
}

void core2_adc_unlock(bool was_locked)
{
#if defined(CORE2_DISABLE_MCP320X)
    return;
#else
    if (was_locked)
        core2_lock_end(lock);
#endif
}

void core2_adc_read_ex(float *VoltArray, float *Factors, core2_adc_channel_t Ch, bool UseLock)
{
#if defined(CORE2_DISABLE_MCP320X)
    {
        VoltArray[0] = -1.00f;
        return;
    }
#else

    bool Locked = !UseLock;
    int ChIdx = -1;

    if (UseLock)
        Locked = core2_adc_lock();

    if (Locked)
    {
        core2_adc_chipselect_enable(Ch);
        SPI.beginTransaction(spi_settings);

        uint16_t raw = 0;
        uint16_t analog = 0;

        if (Ch & CORE2_ADC_CH1)
        {
            raw = adc.read(MCP3201::Channel::SINGLE_0);
            analog = adc.toAnalog(raw);

            ChIdx++;
            VoltArray[ChIdx] = (float)analog * (Factors == NULL ? 1 : Factors[ChIdx]);
        }

        if (Ch & CORE2_ADC_CH2)
        {
            raw = adc1.read(MCP3201::Channel::SINGLE_0);
            analog = adc1.toAnalog(raw);

            ChIdx++;
            VoltArray[ChIdx] = (float)analog * (Factors == NULL ? 1 : Factors[ChIdx]);
        }

        SPI.endTransaction();
        core2_adc_chipselect_disable(Ch);

        if (UseLock && Locked)
        {
            core2_adc_unlock(Locked);
        }
    }

#endif
}

void core2_adc_read(float *Volt1, float *Volt2)
{
#if !defined(CORE2_DISABLE_MCP320X)
    // dprintf("core2_adc_read()\n");

    if (core2_lock_begin(lock))
    {
        core2_adc_chipselect_enable_all();
        SPI.beginTransaction(spi_settings);

        //---------------------------------------------------------
        uint16_t raw = adc.read(MCP3201::Channel::SINGLE_0);
        uint16_t raw1 = adc1.read(MCP3201::Channel::SINGLE_0);

        // get analog value
        uint16_t val = adc.toAnalog(raw);
        uint16_t val1 = adc1.toAnalog(raw1);

        float voltage1 = val * 4.795 / 1000;
        float voltage2 = val1 * 9.215 / 1000; //- val * 4.795 / 1000;

        *Volt1 = voltage1;
        *Volt2 = voltage2;

        // dprintf("core2_adc_read(): Volt1 = %f, Volt2 = %f\n", voltage1, voltage2);

        //---------------------------------------------------------

        SPI.endTransaction();
        core2_adc_chipselect_disable_all();

        core2_lock_end(lock);
    }
#else
    *Volt1 = 0;
    *Volt2 = 0;
#endif
}

bool core2_mcp320x_init()
{
#if defined(CORE2_DISABLE_MCP320X)
    dprintf("core2_mcp320x_init() - SKIPPING, DISABLED\n");
    return false;
#else

    dprintf("core2_mcp320x_init()\n");
    lock = core2_lock_create();

    // configure PIN mode
    pinMode(MCP320X_CS_CHANNEL1, OUTPUT);
    pinMode(MCP320X_CS_CHANNEL2, OUTPUT);

    spi_settings = SPISettings(MCP320X_ADC_CLK, MSBFIRST, SPI_MODE0);

    return true;
#endif
}