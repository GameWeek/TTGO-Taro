#include <WiFi.h>
#include <JPEGDecoder.h>
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

const char AP_DEMO_HTTP_200_IMAGE[] = "HTTP/1.1 200 OK\r\nPragma: public\r\nCache-Control: max-age=1\r\nExpires: Thu, 26 Dec 2016 23:59:59 GMT\r\nContent-Type: image/";

typedef enum
{
  UPL_AP_STAT_MAIN = 1,           // GET /
  UPL_AP_STAT_LED_HIGH,           // GET /H
  UPL_AP_STAT_LED_LOW,            // GET /L
  UPL_AP_STAT_GET_FAVICON,        // GET /favicon.ico
  UPL_AP_STAT_POST_UPLOAD,        // POST /upload
  UPL_AP_STAT_POST_START_BOUNDRY, // POST /upload boundry
  UPL_AP_STAT_POST_GET_BOUNDRY,   // POST /upload boundry
  UPL_AP_STAT_POST_START_IMAGE,   // POST /upload image
  UPL_AP_STAT_POST_GET_IMAGE,     // POST /upload image
} UPL_AP_STAT_t;


const char* ssid = "TTGO";
const char* password = "codetyphon";
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

#define LED_PIN 4

WiFiServer server(80);

#define MAX_IMAGE_SIZE 65535
#define MAX_BUF_SIZE 1024
//#define IMAGE_DEBUG

int value = 0;
char boundbuf[MAX_BUF_SIZE];
int boundpos = 0;
char imagetypebuf[MAX_BUF_SIZE];
int imagetypepos = 0;
char imagebuf[MAX_IMAGE_SIZE];
int imagepos = 0;
String IPaddress;
void setup()
{
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("WiFi Connecting.", 120, 65, 4);
  bool ret;
  WiFi.softAP(ssid, password);
  delay(100);
  IPaddress = WiFi.softAPIP().toString();
  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString(IPaddress, 120, 65, 4);
  tft.setRotation(0);
  server.begin();
}


void printImage(WiFiClient client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:application/json");
  client.println();
  drawArrayJpeg((uint8_t *)imagebuf, imagepos, 0, 0);
}

void loop()
{
  int cnt;
  bool newconn = false;
  int stat;
  WiFiClient client = server.available(); // listen for incoming clients

  if (client)
  { // if you get a client,
    stat = 0;
    boundpos = 0;
    String currentLine = "";      // make a String to hold incoming data from the client
    while (client.connected())
    { // loop while the client's connected
      cnt = client.available();
      if (cnt)
      { // if there's bytes to read from the client,
#ifdef IMAGE_DEBUG
        if (newconn == false)
        {
          newconn = true;
        }
#endif
        char c = client.read(); // read a byte, then
#ifndef IMAGE_DEBUG
        if (stat != UPL_AP_STAT_POST_GET_IMAGE)
        {
#endif
#ifndef IMAGE_DEBUG
        }
#endif

        if (stat == UPL_AP_STAT_POST_GET_IMAGE)
        {
          if (imagepos < MAX_IMAGE_SIZE)
          {
            imagebuf[imagepos] = c;
            imagepos++;
          }
        }
        if (c == '\n')
        { // if the byte is a newline character
#ifdef IMAGE_DEBUG
#endif
          if (stat == UPL_AP_STAT_POST_START_BOUNDRY)
          {
            boundbuf[boundpos] = '\0';
            boundpos++;
#ifdef IMAGE_DEBUG
#endif
            stat = UPL_AP_STAT_POST_UPLOAD;
          }
          if (stat == UPL_AP_STAT_POST_START_IMAGE && currentLine.length() == 0)
          {
            imagetypebuf[imagetypepos] = '\0';
            imagetypepos++;
#ifdef IMAGE_DEBUG
#endif
            imagepos = 0;
            stat = UPL_AP_STAT_POST_GET_IMAGE;
          }
          // if you got a newline, then clear currentLine:
          currentLine = "";
          newconn = false;
        }
        else if (c != '\r')
        { // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
          if (stat == UPL_AP_STAT_POST_START_BOUNDRY)
          {
            if (boundpos < MAX_BUF_SIZE)
            {
              boundbuf[boundpos] = c;
              boundpos++;
            }
          }
          if (stat == UPL_AP_STAT_POST_START_IMAGE)
          {
            if (imagetypepos < MAX_BUF_SIZE)
            {
              imagetypebuf[imagetypepos] = c;
              imagetypepos++;
            }
          }
        }

        if (currentLine.endsWith("POST /upload "))
        {
          stat = UPL_AP_STAT_POST_UPLOAD;
        }
        if (stat == UPL_AP_STAT_POST_UPLOAD && currentLine.endsWith("Content-Type: multipart/form-data; boundary="))
        {
          stat = UPL_AP_STAT_POST_START_BOUNDRY;
        }
        if (stat == UPL_AP_STAT_POST_UPLOAD && currentLine.endsWith("Content-Type: image/"))
        {
          stat = UPL_AP_STAT_POST_START_IMAGE;
        }
        if (stat == UPL_AP_STAT_POST_GET_IMAGE && boundpos > 0 && currentLine.endsWith(boundbuf))
        {
          stat = UPL_AP_STAT_POST_UPLOAD;
          imagepos = imagepos - boundpos - 3;
#ifdef IMAGE_DEBUG
#endif
        }
      }
      else
      {
        if (stat == UPL_AP_STAT_POST_UPLOAD)
        {
          printImage(client);
          break;
        }
        break;
      }
    }
    // close the connection:
    client.stop();
  }

  delay(100);
}

/*====================================================================================
  This sketch contains support functions to render the Jpeg images.
  Created by Bodmer 15th Jan 2017
  ==================================================================================*/

// Return the minimum of two values a and b
#define minimum(a, b) (((a) < (b)) ? (a) : (b))

//====================================================================================
//   This function opens the Filing System Jpeg image file and primes the decoder
//====================================================================================
void drawArrayJpeg(uint8_t *buff_array, uint32_t buf_size, int xpos, int ypos)
{
  boolean decoded = JpegDec.decodeArray(buff_array, buf_size);
  if (decoded)
  {
    renderJPEG(xpos, ypos);
  }
}

//====================================================================================
//   Decode and paint onto the TFT screen
//====================================================================================
void renderJPEG(int xpos, int ypos) {
  // retrieve infomration about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.readSwappedBytes()) {

    // save a pointer to the image block
    pImg = JpegDec.pImage ;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
    {
      tft.pushRect(mcu_x, mcu_y, win_w, win_h, pImg);
    }
    else if ( (mcu_y + win_h) >= tft.height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime;
}
