#include "rtl_433.h"
#include "util.h"

static int mebus433_callback(bitbuffer_t *bitbuffer) {
    bitrow_t *bb = bitbuffer->bb;
    //int temperature_before_dec;
    //int temperature_after_dec;
    char time_str[LOCAL_TIME_BUFLEN];

    float temperature;
    uint8_t humidity;
    uint8_t rolling_code;
    uint8_t channel;
    uint8_t battery_ok;
    uint8_t unknown1;
    uint8_t unknown2;
    uint8_t unknown3;
    data_t *data;

    local_time_str(0, time_str);

    /*
      Mebus protocol description:

      36 bits zero
      36 bits (4 1/2 bytes) information, repeated 4 times

      Byte 5    Byte 4    Byte 3    Byte 2    Byte 1
           4321 8765 4321 8765 4321 8765 4321 8765 4321
           hhhh hhhh yyyy TTTT TTTT bccx tttt rrrr rrrr

      Byte 1    Byte 2    Byte 3    Byte 4    Byte 5
      1234 5678 1234 5678 1234 5678 1234 5678 1234
      rrrr rrrr tttt xccb TTTT TTTT yyyy hhhh hhhh


      r: rolling code or battery voltage or checksum?
      t, T: temperature as signed integer in 10ths of °C (t: lower bits, T: higher bits)
      x: unknown 1 (extended channel code?)
      c: channel
      b: battery status
      y: unknown 2
      h: Humidity as integer number
q
    */

    // we think it's Mebus if it starts with zeros, has some data, has the "unknown2" set to 1111
    // and some repeating data
    if (bb[0][0] == 0 && bb[1][4] !=0 && (bb[1][3] & 0b01111000) && bb[1][4] == bb[12][4]){

        // Upper 4 bits of temperature are stored in byte 1, lower 8 bits are stored in byte 2
        // upper 4 bits of byte 1 are reserved for other usages.
        int16_t temp_i = (int16_t)((uint16_t)(bb[1][1] << 12 ) | bb[1][2]<< 4);
        temp_i = temp_i >> 4;
        temperature = temp_i / 10.0;

        // lower 4 bits of byte 4 and upper 4 bits of byte 5 contains
        // humidity as decimal value
        humidity = (bb[1][3] << 4 | bb[1][4] >> 4);

        rolling_code = bb[1][0];
        channel = ((bb[1][1] & 0b00110000) >> 4) + 1;
        battery_ok = bb[1][1] & 0b10000000;
        unknown1 = (bb[1][1] & 0b01000000) >> 6;
        unknown2 = (bb[1][3] & 0b11110000) >> 4;

        fprintf(stdout, "Sensor event:\n");
        fprintf(stdout, "protocol       = Mebus/433\n");
        fprintf(stdout, "address        = %i\n", rolling_code);
        fprintf(stdout, "channel        = %i\n", channel);
        fprintf(stdout, "battery        = %s\n", battery_ok ? "Ok" : "Low");
        fprintf(stdout, "unkown1        = %i\n", unknown1); // always 0?
        fprintf(stdout, "unkown2        = %i\n", unknown2); // always 1111?
        fprintf(stdout, "temperature    = %.1f°C\n", temperature);
        fprintf(stdout, "humidity       = %u %%\n", humidity);
        fprintf(stdout, "%02x %02x %02x %02x %02x\n",bb[1][0],bb[1][1],bb[1][2],bb[1][3],bb[1][4]);
        for (int i = 0; i <= 3; i++) {
          char bstr[5][9];
          for (int j = 0; j <= 4; j++) {
            strncpy(bstr[j], (char*)byte_to_binary(bb[i][j]), 9);
          }
          fprintf(stdout, "0b %s %s %s %s %s\n", bstr[0], bstr[1], bstr[2], bstr[3], bstr[4]);
        }

        data = data_make("time",          "",             DATA_STRING, time_str,
                         "model",         "",             DATA_STRING, "Mebus/433",
                         "rc",            "Rolling Code", DATA_INT,    rolling_code,
                         "channel",       "",             DATA_INT,    channel,
                         "battery",       "",             DATA_STRING, battery_ok ? "OK": "LOW",
                         "temperature_C", "",             DATA_FORMAT, "%.1f C", DATA_DOUBLE, temperature,
                         "humidity",      "",             DATA_FORMAT, "%u %%", DATA_INT, humidity,
                         "unknown1",      "Unknown 1",    DATA_INT,    unknown1,
                         "unknown2",      "Unknown 2",    DATA_INT,    unknown2,
                         NULL
                         );
        data_acquired_handler(data);

        return 1;
    }
    return 0;
}

r_device mebus433 = {
    .name           = "Mebus 433",
    .modulation     = OOK_PULSE_PPM_RAW,
    .short_limit    = 1200,
    .long_limit     = 2400,
    .reset_limit    = 6000,
    .json_callback  = &mebus433_callback,
    .disabled       = 0,
    .demod_arg      = 0,
};
