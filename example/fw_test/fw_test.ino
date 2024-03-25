// 使用 1.0.0  版本的库 esp32-waveshare-epd 在文件夹： C:\Users\maker\Documents\Arduino\libraries\esp32-waveshare-epd 
// 使用 2.0.0  版本的库 Wire 在文件夹： C:\Users\maker\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\libraries\Wire 
// 使用 2.0.0  版本的库 FS 在文件夹： C:\Users\maker\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\libraries\FS 
// 使用 2.0.0  版本的库 SD_MMC 在文件夹： C:\Users\maker\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\libraries\SD_MMC 


#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include "FS.h"
#include "SD_MMC.h"

extern const unsigned char gImage_black[];
extern const unsigned char gImage_red[];

// #define BUTTON_PIN 0

#define SWITCH_PIN_0 12
#define SWITCH_PIN_1 5
#define SWITCH_PIN_2 17
#define SWITCH_PIN_3 47
#define SWITCH_PIN_4 48
#define SWITCH_PIN_5 18

#define PIN_SD_CMD 2
#define PIN_SD_CLK 42
#define PIN_SD_D0 41

uint64_t cardSize = 0;

int switch_list[6] = {SWITCH_PIN_0, SWITCH_PIN_1, SWITCH_PIN_2, SWITCH_PIN_3, SWITCH_PIN_4, SWITCH_PIN_5};

void setup()
{
    pin_set();

    USBSerial.begin(115200);
    USBSerial.println("EPD_4IN2B_V2_test Demo\r\n");

    DEV_Module_Init();

    USBSerial.println("e-Paper Init and Clear...\r\n");
    EPD_4IN2B_V2_Init();
    EPD_4IN2B_V2_Clear();
    DEV_Delay_ms(500);

    // Create a new image cache named IMAGE_BW and fill it with white
    UBYTE *BlackImage, *RYImage; // Red or Yellow
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

    // Pure Screen
    USBSerial.println("All BLACK\r\n");
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawRectangle(0, 0, 400, 300, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    EPD_4IN2B_V2_Display(BlackImage, RYImage);
    DEV_Delay_ms(3000);

    USBSerial.println("All RED\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);
    Paint_DrawRectangle(0, 0, 400, 300, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    EPD_4IN2B_V2_Display(BlackImage, RYImage);
    DEV_Delay_ms(3000);

    USBSerial.println("show image for array\r\n");
    EPD_4IN2B_V2_Display(gImage_black, gImage_red);
    DEV_Delay_ms(3000);

    /*Horizontal screen*/
    // 1.Draw black image
    USBSerial.println("Draw black image\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 90, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 100, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 110, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawLine(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(70, 70, 20, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawRectangle(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(10, 0, "Makerfabs", &Font16, BLACK, WHITE);
    Paint_DrawNum(10, 50, 987654321, &Font16, WHITE, BLACK);

    // 2.Draw red image
    USBSerial.println("Draw red image\r\n");
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);
    Paint_DrawCircle(160, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(210, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
    Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);

    if (sd_check() == 1)
    {
        char s[40];
        sprintf(s, "SD_MMC Size: %lluMB", cardSize);
        Paint_DrawString_EN(10, 200, s, &Font24, WHITE, BLACK);
    }
    else
    {
        Paint_DrawString_EN(10, 200, "SD Card check failed", &Font24, BLACK, WHITE);
    }

    Paint_DrawString_EN(10, 250, "Press Any Switch Sleep", &Font24, BLACK, WHITE);

    USBSerial.println("EPD_Display\r\n");
    EPD_4IN2B_V2_Display(BlackImage, RYImage);
    DEV_Delay_ms(2000);

    free(BlackImage);
    free(RYImage);
    BlackImage = NULL;
    RYImage = NULL;

    while (1)
    {
        if (switch_detect() != -1)
        {
            USBSerial.println("Clear...\r\n");
            EPD_4IN2B_V2_Clear();
            USBSerial.println("Goto Sleep...\r\n");
            EPD_4IN2B_V2_Sleep();

            break;
        }
    }
}

/* The main loop -------------------------------------------------------------*/
void loop()
{
    switch_detect();
    delay(10);
}

void pin_set()
{
    pinMode(SWITCH_PIN_0, INPUT);
    pinMode(SWITCH_PIN_1, INPUT);
    pinMode(SWITCH_PIN_2, INPUT);

    pinMode(SWITCH_PIN_3, INPUT);
    pinMode(SWITCH_PIN_4, INPUT);
    pinMode(SWITCH_PIN_5, INPUT);
}

int switch_detect()
{

    for (int i = 0; i < 6; i++)
    {
        if (digitalRead(switch_list[i]) == 0)
        {
            delay(100);
            if (digitalRead(switch_list[i]) == 0)
            {
                USBSerial.print("Switch Num:");
                USBSerial.println(i);
                return i;
            }
        }
    }

    return -1;
}

int sd_check()
{
    SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
    if (!SD_MMC.begin("/sdcard", true, true))
    {
        USBSerial.println("Card Mount Failed");
        return 0;
    }
    uint8_t cardType = SD_MMC.cardType();

    if (cardType == CARD_NONE)
    {
        USBSerial.println("No SD_MMC card attached");
        return 0;
    }

    USBSerial.print("SD_MMC Card Type: ");
    if (cardType == CARD_MMC)
    {
        USBSerial.println("MMC");
    }
    else if (cardType == CARD_SD)
    {
        USBSerial.println("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        USBSerial.println("SDHC");
    }
    else
    {
        USBSerial.println("UNKNOWN");
    }

    cardSize = SD_MMC.cardSize() / (1024 * 1024);
    USBSerial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

    return 1;
}