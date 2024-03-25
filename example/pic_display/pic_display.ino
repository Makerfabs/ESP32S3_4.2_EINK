

#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include "FS.h"
#include "SD_MMC.h"

#define SCREEN_W 400
#define SCREEN_H 300

#define BUTTON_PIN 12
#define PIN_SD_CMD 2
#define PIN_SD_CLK 42
#define PIN_SD_D0 41

#define FILE_NAME "/qr400300.bmp"

UBYTE *BlackImage, *RYImage; // Red or Yellow
File file;

void setup()
{
    USBSerial.begin(115200);
    USBSerial.println("EPD_4IN2B_V2_test Demo\r\n");
    DEV_Module_Init();
    pinMode(BUTTON_PIN, INPUT);

    SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
    if (!SD_MMC.begin("/sdcard", true, true))
    {
        USBSerial.println("Card Mount Failed");
        while (1)
            delay(100);
    }

    USBSerial.println("e-Paper Init and Clear...\r\n");
    EPD_4IN2B_V2_Init();
    EPD_4IN2B_V2_Clear();
    DEV_Delay_ms(500);

    // Create a new image cache named IMAGE_BW and fill it with white

    long Imagesize = SCREEN_W * SCREEN_H / 8;
    // long Imagesize = ((EPD_4IN2B_V2_WIDTH % 8 == 0) ? (EPD_4IN2B_V2_WIDTH / 8) : (EPD_4IN2B_V2_WIDTH / 8 + 1)) * EPD_4IN2B_V2_HEIGHT;

    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        USBSerial.println("Failed to apply for black memory...\r\n");
        while (1)
            ;
    }
    if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        USBSerial.println("Failed to apply for red memory...\r\n");
        while (1)
            ;
    }

    char temp[20];
    sprintf(temp, "image size = %ld", Imagesize);
    USBSerial.println(temp);

    USBSerial.println("NewImage:BlackImage and RYImage\r\n");
    Paint_NewImage(BlackImage, SCREEN_W, SCREEN_H, 0, WHITE);
    Paint_NewImage(RYImage, SCREEN_W, SCREEN_H, 0, WHITE);

    // Select Image
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);

    print_img(SD_MMC, FILE_NAME, SCREEN_W, SCREEN_H);

    EPD_4IN2B_V2_Display(BlackImage, RYImage);

    while (1)
    {
        if (digitalRead(BUTTON_PIN) == 0)
        {
            delay(100);
            if (digitalRead(BUTTON_PIN) == 0)
            {
                break;
            }
        }
    }

    USBSerial.println("Clear...\r\n");
    EPD_4IN2B_V2_Clear();

    USBSerial.println("Goto Sleep...\r\n");
    EPD_4IN2B_V2_Sleep();

    free(BlackImage);
    free(RYImage);

    BlackImage = NULL;
    RYImage = NULL;
}

void loop()
{
}

// Display image from file
void print_img(fs::FS &fs, String filename, int x, int y)
{
    File f = fs.open(filename, "r");
    if (!f)
    {
        Serial.println("Failed to open file for reading");
        f.close();
        return;
    }

    f.seek(54);
    int X = x;
    int Y = y;
    uint8_t RGB[3 * X];
    for (int row = 0; row < Y; row++)
    // for (int row = 0; row < Y; row++)
    {
        f.seek(54 + 3 * X * row);
        f.read(RGB, 3 * X);

        uint8_t temp = 0;
        char index = 0;
        for (int colum = 0; colum < 400; colum++)
        {

            if (RGB[colum * 3] > 127)
            {
                temp = temp * 2 + 1;
            }
            else
            {
                temp = temp * 2 + 0;
            }

            // temp = 0xf0; 1111 1111

            index++;
            if (index > 7)
            {
                BlackImage[(X * row + colum) / 8] = temp;

                USBSerial.println(temp);

                temp = 0;
                index = 0;
            }
        }
    }

    f.close();
}