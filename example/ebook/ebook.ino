
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include "FS.h"
#include "SD_MMC.h"

extern const unsigned char gImage_black[];
extern const unsigned char gImage_red[];

#define BUTTON_PIN 12
#define PIN_SD_CMD 2
#define PIN_SD_CLK 42
#define PIN_SD_D0 41

#define FILE_NAME "/book.txt"

uint64_t cardSize = 0;
UBYTE *BlackImage, *RYImage; // Red or Yellow
File file;
int page_num = 1;


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

    UWORD Imagesize = ((EPD_4IN2B_V2_WIDTH % 8 == 0) ? (EPD_4IN2B_V2_WIDTH / 8) : (EPD_4IN2B_V2_WIDTH / 8 + 1)) * EPD_4IN2B_V2_HEIGHT;
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
    USBSerial.println("NewImage:BlackImage and RYImage\r\n");
    Paint_NewImage(BlackImage, EPD_4IN2B_V2_WIDTH, EPD_4IN2B_V2_HEIGHT, 0, WHITE);
    Paint_NewImage(RYImage, EPD_4IN2B_V2_WIDTH, EPD_4IN2B_V2_HEIGHT, 0, WHITE);

    // Select Image
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);

    file = SD_MMC.open(FILE_NAME);
    display_page();

    while (1)
    {
        if (digitalRead(BUTTON_PIN) == 0)
        {
            delay(100);
            if (digitalRead(BUTTON_PIN) == 0)
            {

                long button_time = millis();
                while (1)
                {
                    if (digitalRead(BUTTON_PIN) == 1)
                        break;
                    delay(500);
                }

                if ((millis() - button_time) < 3000)
                {
                    // Job
                    display_page();
                }
                else
                    break;
            }
        }
    }

    file.close();

    USBSerial.println("Clear...\r\n");
    EPD_4IN2B_V2_Clear();

    USBSerial.println("Goto Sleep...\r\n");
    EPD_4IN2B_V2_Sleep();

    free(BlackImage);
    free(RYImage);

    BlackImage = NULL;
    RYImage = NULL;
}

/* The main loop -------------------------------------------------------------*/
void loop()
{
}

int read_line(File &file, char *line, int length)
{
    int c = 0;
    int i = 0;
    while (file.available())
    {
        c = file.read();
        if (c == '\r')
            continue;
        else if (c == '\n')
        {
            break;
        }
        else
        {
            line[i++] = c;
        }

        if (i >= length - 1)
            break;
    }

    line[i] = '\0';
    return i + 1;
}

void display_page()
{
    char txt[12][30];

    USBSerial.println("New page\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    if (!file)
    {
        USBSerial.println("Failed to open file for reading");
        return;
    }

    Paint_DrawString_EN(20, 0, FILE_NAME, &Font24, BLACK, WHITE);
    char temp[30];
    sprintf(temp,"Page :%04d ",page_num++);
    USBSerial.println(temp);
    Paint_DrawString_EN(20, 280, temp, &Font16, BLACK, WHITE);

    for (int i = 0; i < 12; i++)
    {
        int c_cont = 0;
        c_cont = read_line(file, txt[i], 30);
        USBSerial.println(txt[i]);
        Paint_DrawString_EN(20, 30 + i * 20, txt[i], &Font16, WHITE, BLACK);
    }

    EPD_4IN2B_V2_Display(BlackImage, RYImage);
}