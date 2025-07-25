#include <M5Unified.h>

static constexpr const size_t record_number     = 256;
static constexpr const size_t record_length     = 720;//record line length, recommend lcd width
static constexpr const size_t record_size       = record_number * record_length;
static constexpr const size_t record_samplerate = 17000;
static int16_t prev_y[record_length];
static int16_t prev_h[record_length];
static size_t rec_record_idx  = 2;
static size_t draw_record_idx = 0;
static int16_t *rec_data;

static int32_t w;

void setup(void) {
    M5.begin();

    w = M5.Display.width();

    M5.Display.startWrite();
    M5.Display.setRotation(0);
    M5.Display.setTextDatum(top_center);
    M5.Display.setTextColor(WHITE);
    M5.Display.setFont(&fonts::FreeSansBoldOblique18pt7b);

    rec_data = (typeof(rec_data))heap_caps_malloc(record_size *sizeof(int16_t), MALLOC_CAP_8BIT);
    memset(rec_data, 0, record_size * sizeof(int16_t));
    M5.Speaker.setVolume(255);

    // Since the microphone and speaker cannot be used at the same time,
    // turn off the speaker here.
    M5.Speaker.end();
    M5.Mic.begin();
    M5.Display.fillCircle(300, 30, 20, RED);
    M5.Display.drawString("REC", w / 2, 18);
    M5.Display.drawString("Touch screen to play the record", w / 2, 200);
}

void loop(void) {
    M5.update();

    if (M5.Mic.isEnabled()) {
        static constexpr int shift = 6;
        auto data = &rec_data[rec_record_idx * record_length];
        if (M5.Mic.record(data, record_length, record_samplerate)) {
            data = &rec_data[draw_record_idx * record_length];

            if (w > record_length - 1) {
                w = record_length - 1;
            }
            for (int32_t x = 0; x < w; ++x) {
                M5.Display.writeFastVLine(x, prev_y[x], prev_h[x],
                                              TFT_BLACK);
                int32_t y1 = (data[x] >> shift);
                int32_t y2 = (data[x + 1] >> shift);
                if (y1 > y2) {
                    int32_t tmp = y1;
                    y1          = y2;
                    y2          = tmp;
                }
                int32_t y = ((M5.Display.height()) >> 1) + y1;
                int32_t h = ((M5.Display.height()) >> 1) + y2 + 1 - y;
                prev_y[x] = y;
                prev_h[x] = h;
                M5.Display.writeFastVLine(x, prev_y[x], prev_h[x], WHITE);
            }

            M5.Display.display();
            M5.Display.fillCircle(300, 30, 20, RED);
            M5.Display.drawString("REC", w / 2, 18);
            M5.Display.drawString("Touch screen to play the record", w / 2, 200);
            if (++draw_record_idx >= record_number) {
                draw_record_idx = 0;
            }
            if (++rec_record_idx >= record_number) {
                rec_record_idx = 0;
            }
        }
    }

    if (M5.Touch.getCount() && M5.Touch.getDetail(0).wasClicked()) {
        if (M5.Speaker.isEnabled()) {
            while (M5.Mic.isRecording()) {
                delay(1);
            }
            M5.Display.clear();
            M5.Display.fillCircle(300, 30, 20, GREEN);
            M5.Display.drawString("PLAY", w / 2, 18);

            // Since the microphone and speaker cannot be used at the same time, 
            // turn off the microphone here.
            M5.Mic.end();
            M5.Speaker.begin();


            int start_pos = rec_record_idx * record_length;
            if (start_pos < record_size) {
                M5.Speaker.playRaw(&rec_data[start_pos],
                                       record_size - start_pos,
                                       record_samplerate, false, 1, 0);
            }
            if (start_pos > 0) {
                M5.Speaker.playRaw(rec_data, start_pos, record_samplerate,
                                       false, 1, 0);
            }
            do {
                delay(1);
                M5.update();
            } while (M5.Speaker.isPlaying());

            // Since the microphone and speaker cannot be used at the same time, 
            // turn off the speaker here.
            M5.Speaker.end();
            M5.Mic.begin();

            M5.Display.clear();
            M5.Display.fillCircle(300, 30, 20, RED);
            M5.Display.drawString("REC", w / 2, 18);
        }
    }
} 