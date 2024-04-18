

#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include "FS.h"
#include "SD_MMC.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define SCREEN_W 400
#define SCREEN_H 300

#define SWITCH_PIN_0 12
#define SWITCH_PIN_1 5
#define SWITCH_PIN_2 17
#define SWITCH_PIN_3 47
#define SWITCH_PIN_4 48
#define SWITCH_PIN_5 18

#define PIN_SD_CMD 2
#define PIN_SD_CLK 42
#define PIN_SD_D0 41

UBYTE *BlackImage, *RYImage; // Red or Yellow
File file;

#define FILE_COUNT 3
int file_count = 0;
String file_list[FILE_COUNT] = {"/1.bmp", "/2.bmp", "/3.bmp"};

int switch_list[6] = {SWITCH_PIN_0, SWITCH_PIN_1, SWITCH_PIN_2, SWITCH_PIN_3, SWITCH_PIN_4, SWITCH_PIN_5};

WiFiServer server(80);

void setup()
{
    hardware_init();

    WiFi.mode(WIFI_AP);
    WiFi.softAP("Makefabs_Eink");
    IPAddress myIP = WiFi.softAPIP();
    USBSerial.print("AP IP address: ");
    USBSerial.println(myIP);

    server.begin();

    eink_init();
    img_task();
}

void loop()
{
    WiFiClient client = server.available(); // listen for incoming clients
    int fresh_flag = 0;

    if (client)
    {                                     // if you get a client,
        USBSerial.println("New Client."); // print a message out the serial port
        String currentLine = "";          // make a String to hold incoming data from the client
        while (client.connected())
        { // loop while the client's connected
            if (client.available())
            {                           // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                USBSerial.write(c);     // print it out the serial monitor
                if (c == '\n')
                { // if the byte is a newline character

                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        // the content of the HTTP response follows the header:
                        client.print("Click <a href=\"/H\">here</a> to Change Picture.<br>");

                        // The HTTP response ends with another blank line:
                        client.println();
                        // break out of the while loop:
                        break;
                    }
                    else
                    { // if you got a newline, then clear currentLine:
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }

                // Check to see if the client request was "GET /H" or "GET /L":
                if (currentLine.endsWith("GET /H"))
                {
                    file_count++;
                    if (file_count > 2)
                        file_count = 0;
                    fresh_flag = 1;

                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println();
                    client.print("Begin to fresh.<br>");
                    client.println();
                }
            }
        }
        // close the connection:
        client.stop();
        USBSerial.println("Client Disconnected.");
    }

    if (fresh_flag == 1)
        img_task();

    if (switch_detect() != -1)
    {
        USBSerial.println("Clear...\r\n");
        EPD_4IN2B_V2_Clear();

        USBSerial.println("Goto Sleep...\r\n");
        EPD_4IN2B_V2_Sleep();

        free(BlackImage);
        free(RYImage);

        BlackImage = NULL;
        RYImage = NULL;
    }
}

void hardware_init()
{
    pinMode(SWITCH_PIN_0, INPUT);
    pinMode(SWITCH_PIN_1, INPUT);
    pinMode(SWITCH_PIN_2, INPUT);

    pinMode(SWITCH_PIN_3, INPUT);
    pinMode(SWITCH_PIN_4, INPUT);
    pinMode(SWITCH_PIN_5, INPUT);

    USBSerial.begin(115200);
    USBSerial.println("EPD_4IN2B_V2_test Demo\r\n");

    SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
    if (!SD_MMC.begin("/sdcard", true, true))
    {
        USBSerial.println("Card Mount Failed");
        while (1)
            delay(100);
    }
}

void eink_init()
{
    USBSerial.println("e-Paper Init and Clear...\r\n");
    DEV_Module_Init();
    EPD_4IN2B_V2_Init();
    EPD_4IN2B_V2_Clear();
    DEV_Delay_ms(500);

    // Create a new image cache named IMAGE_BW and fill it with white

    long Imagesize = SCREEN_W * SCREEN_H / 8;
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
}

void img_task()
{
    USBSerial.println("NewImage:BlackImage and RYImage\r\n");
    Paint_NewImage(BlackImage, SCREEN_W, SCREEN_H, 0, WHITE);
    Paint_NewImage(RYImage, SCREEN_W, SCREEN_H, 0, WHITE);

    // Select Image
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);

    print_img_br(SD_MMC, file_list[file_count].c_str(), SCREEN_W, SCREEN_H);

    EPD_4IN2B_V2_Display(BlackImage, RYImage);
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

// Display image from file
void print_img(fs::FS &fs, String filename, int x, int y)
{
    File f = fs.open(filename, "r");
    if (!f)
    {
        USBSerial.println("Failed to open file for reading");
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

            // 判断是否为白色
            if (RGB[colum * 3] > 127)
            {
                // 白色为1
                temp = temp * 2 + 1;
            }
            else
            {
                // 黑色为0
                temp = temp * 2 + 0;
            }

            // temp = 0xf0; 1111 1111

            index++;
            if (index > 7)
            {
                // BlackImage[(X * row + colum) / 8] = temp;
                BlackImage[(X * (299 - row) + colum) / 8] = temp;

                // USBSerial.println(temp);

                temp = 0;
                index = 0;
            }
        }
    }

    f.close();
}

// Display image from file
void print_img_br(fs::FS &fs, String filename, int x, int y)
{
    File f = fs.open(filename, "r");
    if (!f)
    {
        USBSerial.println("Failed to open file for reading");
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

        uint8_t temp_black = 0;
        uint8_t temp_red = 0;
        char index = 0;
        for (int colum = 0; colum < 400; colum++)
        {

            // 判断是否为白色 r>200 g>200
            if (RGB[colum * 3] > 150 && RGB[colum * 3 + 2] > 150)
            {
                // 白色为1
                temp_black = temp_black * 2 + 1;
                temp_red = temp_red * 2 + 1;
            }
            // 判断是否为红色 r>200 g<200
            else if (RGB[colum * 3] < 150 && RGB[colum * 3 + 2] > 150)
            {
                temp_black = temp_black * 2 + 1;
                temp_red = temp_red * 2 + 0;
            }
            else
            {
                // 黑色为0
                temp_black = temp_black * 2 + 0;
                temp_red = temp_red * 2 + 1;
            }

            index++;
            if (index > 7)
            {
                BlackImage[(X * (299 - row) + colum) / 8] = temp_black;
                RYImage[(X * (299 - row) + colum) / 8] = temp_red;

                // USBSerial.println(temp);

                temp_black = 0;
                temp_red = 0;
                index = 0;
            }
        }
    }

    f.close();
}