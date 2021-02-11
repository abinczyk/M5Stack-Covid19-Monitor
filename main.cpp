/**************************************************************************
 * Covid-19 Data Monitor
 * 
 * Code to graphically display the current data of the Covid-19 pandemic
 * on a M5Stack (ESP32 MCU with integrated LCD)
 * 
 * Hague Nusseck @ electricidea
 * v1.3 24.March.2020
 * https://github.com/electricidea/M5Stack-Covid19-Monitor
 * 
 * 
 * Distributed as-is; no warranty is given.
**************************************************************************/

#include <Arduino.h>

#include <M5Stack.h>
// install the library:
// pio lib install "M5Stack"

// Free Fonts for nice looking fonts on the screen
#include "Free_Fonts.h"

#include "WiFi.h"
#include <WiFiClientSecure.h>
// Init the Secure client object
WiFiClientSecure client;

// Stuff for the Graphical output
// The M5Stack screen pixel is 320x240, with the top left corner of the screen as the origin (0,0)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
int max_y = 0;

// overall maximum to calculate the percentage of processing
#define max_number_countries 180
// preconfigured countries
// The names must match with the country names inside the JSON file
// 5 countries can be configured
// The first one should always be "All countries"
String country_names[6] = {"All countries", "China", "Germany", "US", "Italy", "Spain"}; 
// colors for the countries
const uint32_t country_color[6] = {LIGHTGREY, RED, GREEN, 0x51D, WHITE, MAGENTA};

// Array field for the collected data out of the JSON file
// collected_data[all, confirmed, deaths][country][data point]
int collected_data[2][6][SCREEN_WIDTH]; // space for two years
String data_name[] = {"confirmed", "deaths"};
// array to count the found data for each country
int data_count[6] = {0, 0, 0, 0, 0, 0};
// String to hold the last date found in the JSON file
std::string last_date = "";
// variable to switch between the graphical views
// 0 = Start screen and "GET DATA" Menu
// 1 = 
int display_state = 0;

// buffer for formatting numbers with thousand separator
// see function description for further  details
char format_buffer[11];
char thousand_separator = '.';

// WiFi network configuration:
// A simple method to configure multiple WiFi Access Configurations:
// Add the SSID and the password to the list.
// IMPORTANT: keep both arrays with the same length!
String WIFI_ssid[]     = {"Manfred 2019", "Work_ssid", "Mobile_ssid", "Best-Friend_ssid"};
String WIFI_password[] = {"01835915",  "Work_pwd",  "Mobile_pwd",  "Best-Friend_pwd"};

// the actual data are provided on this github page:
// https://github.com/pomber/covid19
// JSON time-series of coronavirus cases (confirmed, deaths and recovered) per country
// https://pomber.github.io/covid19/timeseries.json
const char*  data_server_name= "pomber.github.io";  // Server URL

// Certificate for Website:
// https://pomber.github.io
const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n" \
"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n" \
"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n" \
"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n" \
"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n" \
"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n" \
"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n" \
"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n" \
"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n" \
"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n" \
"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n" \
"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n" \
"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n" \
"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n" \
"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n" \
"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n" \
"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n" \
"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n" \
"+OkuE6N36B9K\n" \
"-----END CERTIFICATE-----\n";



//==============================================================
// function forward declaration
void scan_WIFI();
boolean connect_Wifi(const char * ssid, const char * password);
const char *formatNumber(int value, char *buffer, int len);
void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace);
void Clear_Screen();
int process_data();
void display_data_graph(int data_select);
void display_data_text(int data_select);


//==============================================================
// Print a small menu at the bottom of the display above the buttons
void print_menu(int menu_index){
    M5.Lcd.fillRect(0, M5.Lcd.height()-25, M5.Lcd.width(), 25, 0x7BEF);
    M5.Lcd.setCursor(0, 230);    
    M5.Lcd.setFreeFont(FF1);
    switch (menu_index) {
      case 0: {    
        M5.Lcd.print("      -       -        - ");
        break;       
      }
      case 1: {    
        M5.Lcd.print("           GET DATA      ");
        break;
      }
      case 2: {    
        M5.Lcd.print("      <       -        > ");
        break;
      }
      default: {
        M5.Lcd.print("      -       -        - ");
        break;
      }
    }
}

void setup() {
    // initialize the M5Stack object
    M5.begin();

    // configure the Lcd display
    M5.Lcd.setBrightness(100); // Brightness (0: Off - 255:Full)
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    Clear_Screen();
    // configure centered String output
    M5.Lcd.setTextDatum(CC_DATUM);
    M5.Lcd.setFreeFont(FF2);
    M5.Lcd.drawString("Covid-19 Monitor", (int)(M5.Lcd.width()/2), (int)(M5.Lcd.height()/2), 1);
    M5.Lcd.setFreeFont(FF1);
    M5.Lcd.drawString("Version 1.3 | 24.03.2020", (int)(M5.Lcd.width()/2), M5.Lcd.height()-20, 1);
    // wait 5 seconds before start file action
    delay(5000);
    // configure Top-Left oriented String output
    M5.Lcd.setTextDatum(TL_DATUM);
    // scan and display available WIFI networks
    Clear_Screen();
    scan_WIFI();
    delay(1000);
    // connect to WIFI
    // Try all access configurations until a connection could be established.
    int WIFI_location = 0;
    while(WiFi.status() != WL_CONNECTED){
      delay(1000);
      Clear_Screen();
      connect_Wifi(WIFI_ssid[WIFI_location].c_str(), WIFI_password[WIFI_location].c_str());
      WIFI_location++;
      if(WIFI_location >= (sizeof(WIFI_ssid)/sizeof(WIFI_ssid[0])))
        WIFI_location = 0;
    }
    M5.Lcd.println("");
    M5.Lcd.println("[OK] Connected to WiFi");
    delay(2000);
    // ready to download and visualize the data
    Clear_Screen();
    M5.Lcd.setTextDatum(CC_DATUM);
    M5.Lcd.setFreeFont(FF2);
    M5.Lcd.drawString("Covid-19 Monitor", (int)(M5.Lcd.width()/2), (int)(M5.Lcd.height()/6), 1);
    M5.Lcd.setFreeFont(FF1);
    // Country names
    int y_pos = (int)(M5.Lcd.height()/6)+50;
    for(int n=1; n<6; n++){
      M5.Lcd.drawString(country_names[n].c_str(), (int)(M5.Lcd.width()/2), y_pos, 1);
      y_pos = y_pos + 20;
    }
    M5.Lcd.setTextDatum(TL_DATUM);
    // print the "GET DATA" Menu
    display_state = 0;
    print_menu(1);
}


void loop() {
  M5.update();  
 
  // prev page
  if (M5.BtnA.wasPressed()){
    display_state--;
    if(display_state < 1)
      display_state = 8;  
    if(display_state < 3)
      display_data_graph(display_state);
    else
      display_data_text(display_state);
  }

  // get data
  if (M5.BtnB.wasPressed()){
    Clear_Screen();
    // set the certificate for the https connection to github.io
    M5.Lcd.println("[OK] set certificate");
    client.setCACert(root_ca);
    // connect to the server
    M5.Lcd.println("Starting connection...");
    if (!client.connect(data_server_name, 443))
      M5.Lcd.println("[ERR] Connection failed!");
    else {
      M5.Lcd.println("[OK] Connected to server");
      delay(200);
      // Make a HTTP request:
      client.println("GET https://pomber.github.io/covid19/timeseries.json HTTP/1.0");
      client.println("Host: pomber.github.io");
      client.println("Connection: close");
      client.println();
      // get the JSON data from the github server
      // and calculate the values
      process_data();
      // close the connection to the server
      client.stop();
      // display confirmed data
      display_state = 1; 
      display_data_graph(display_state);
    }
  }

  // next page
  if (M5.BtnC.wasPressed()){
    display_state++;
    if(display_state > 8)
      display_state = 1;  
    if(display_state < 3)
      display_data_graph(display_state);
    else
      display_data_text(display_state);
  }
}



//==============================================================
// Scan for available Wifi networks
// print result als simple list
void scan_WIFI() {
      M5.Lcd.println("WiFi scan ...");
      // WiFi.scanNetworks returns the number of networks found
      int n = WiFi.scanNetworks();
      if (n == 0) {
          M5.Lcd.println("[ERR] no networks found");
      } else {
          //M5.Lcd.println("[OK] %i networks found:\n",n);
          for (int i = 0; i < n; ++i) {
              // Print SSID for each network found
              M5.Lcd.printf("  %i: ",i+1);
              M5.Lcd.println(WiFi.SSID(i));
              delay(10);
          }
      }
}

//==============================================================
// establish the connection to an Wifi Access point
boolean connect_Wifi(const char * ssid, const char * password){
  // Establish connection to the specified network until success.
  // Important to disconnect in case that there is a valid connection
  WiFi.disconnect();
  M5.Lcd.println("Connecting to ");
  M5.Lcd.println(ssid);
  delay(500);
  //Start connecting (done by the ESP in the background)
  WiFi.begin(ssid, password);
  // read wifi Status
  wl_status_t wifi_Status = WiFi.status();  
  int n_trials = 0;
  // loop while waiting for Wifi connection
  // run only for 5 trials.
  while (wifi_Status != WL_CONNECTED && n_trials < 5) {
    // Check periodicaly the connection status using WiFi.status()
    // Keep checking until ESP has successfuly connected
    // or maximum number of trials is reached
    wifi_Status = WiFi.status();
    n_trials++;
    switch(wifi_Status){
      case WL_NO_SSID_AVAIL:
          M5.Lcd.println("[ERR] SSID not available");
          break;
      case WL_CONNECT_FAILED:
          M5.Lcd.println("[ERR] Connection failed");
          break;
      case WL_CONNECTION_LOST:
          M5.Lcd.println("[ERR] Connection lost");
          break;
      case WL_DISCONNECTED:
          M5.Lcd.println("[ERR] WiFi disconnected");
          break;
      case WL_IDLE_STATUS:
          M5.Lcd.println("[ERR] WiFi idle status");
          break;
      case WL_SCAN_COMPLETED:
          M5.Lcd.println("[OK] WiFi scan completed");
          break;
      case WL_CONNECTED:
          M5.Lcd.println("[OK] WiFi connected");
          break;
      default:
          M5.Lcd.println("[ERR] unknown Status");
          break;
    }
    delay(500);
  }
  if(wifi_Status == WL_CONNECTED){
    // connected
    M5.Lcd.print("IP address: ");
    M5.Lcd.println(WiFi.localIP());
    return true;
  } else {
    // not connected
    M5.Lcd.println("");
    M5.Lcd.println("[ERR] unable to connect Wifi");
    return false;
  }
}


//==============================================================
// Function to format a integer number into a string 
// with thousand separator and a fixed length
// useful to get a nice right aligned string output
// inspired from:
// https://stackoverflow.com/questions/1449805/how-to-format-a-number-from-1123456789-to-1-123-456-789-in-c
// Use as following:
//
// char buffer[11]; // 10 char + \0 = 11
// M5.Lcd.printf("%s\n", formatNumber(1234567, format_buffer, sizeof(buffer)));
// output: " 1.234.567"
const char *formatNumber (int value, char *buffer, int len) {
    int charCount;
    char *endOfbuffer = buffer+len;
    int savedValue = value;
    if (value < 0)
        value = - value;
    // \0 Termination char at the end
    *--endOfbuffer = 0;
    charCount = -1;
    do{
        if (++charCount == 3){
            charCount = 0;
            *--endOfbuffer = thousand_separator;
            if(endOfbuffer <= buffer)
              break;
        }
        *--endOfbuffer = (char) (value % 10 + '0');
    } while ((value /= 10) != 0 && endOfbuffer > buffer);
    // add a minus sign if the original value was negative
    if (savedValue < 0 && endOfbuffer > buffer)
        *--endOfbuffer = '-';
    // fill up with spaces, to the full string length
    while(endOfbuffer > buffer){
      *--endOfbuffer = ' ';
    }
    return endOfbuffer;
}

//==============================================================
// A String-Replace function...
// from:
// https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

//==============================================================
// Clear the entire screen and add one row
// The added row is important. Otherwise the first row is not visible
void Clear_Screen(){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("");
}

//==============================================================
// receive the JSON file from the server
// go through the file line by line
// search for keywords and sum the values
int process_data(){
  // Number of lines received from the server and processed
  int line_count = 0;
  // total number of countries found in the JSON file
  int countries_found = 0;
  for(int n=0; n<6; n++)
    data_count[n] = 0;
  // strings to hold the received data
  std::string rcv_line = "";
  std::string analyze_line = "";
  // skip the header data
  while (client.connected()) {
    rcv_line = client.readStringUntil('\n').c_str();
    if (rcv_line == "\r") {
      M5.Lcd.println("[OK] header received");
      break;
    }
  }
  // A small state machine is used to find the configured countries
  // and collect the data for these countries seperately
  // parse_state = 0 --> looking for a configured countrie
  // parse_state = 1, 2, 3, 4, 5 --> country found. collecting data
  int parse_state = 0;

  // Basic format information from the JSOM data:
  // Country START string:
  // "  \"Germany\": [\n"
  // Country END string:
  // "  ],\n" or "  ]\n"
  // value strings:
  // "      \"confirmed\": 0,\n"
  // "      \"deaths\": 0,\n"
  // "      \"recovered\": 0\n"
  
  // if there are incoming bytes available
  while (client.available()) {
    // get one line from the server data
    rcv_line = client.readStringUntil('\n').c_str();
    // looking for the "country seperator"
    if (rcv_line.find(": [", 0)  != std::string::npos){ 
      // count the overall number of countries
      countries_found++;
      // the data from all countries should summed up
      data_count[0] = 0;
      // print the percentage of processing
      Clear_Screen();
      M5.Lcd.setTextDatum(CC_DATUM);
      M5.Lcd.setFreeFont(FF3);
      char text_buffer[32];
      snprintf(text_buffer, sizeof(text_buffer), "%3.0f%%",
              (100.0/max_number_countries)*countries_found);
      M5.Lcd.drawString(text_buffer, (int)(M5.Lcd.width()/2), (int)(M5.Lcd.height()/2), 1);
    }
    // count the number of confirmed, deaths and recovered over all countries
    // looking for a "confirmed" data line
    if (rcv_line.find("confirmed", 0)  != std::string::npos){
      analyze_line = rcv_line;
      // delete everything out of the string exept the number
      ReplaceStringInPlace(analyze_line, "      \"confirmed\": ", "");
      ReplaceStringInPlace(analyze_line, ",", "");
      ReplaceStringInPlace(analyze_line, "\n", "");
      int value = atoi(analyze_line.c_str());
      // init the value for "all countries" (index 0) with the values of the first country
      if(countries_found == 1){
        collected_data[0][0][data_count[0]] = value;
      } else {
        // Otherwise add the values of the other countries
        collected_data[0][0][data_count[0]] = collected_data[0][0][data_count[0]]+value;
      }
      // if we are inside an configure country section
      if(parse_state > 0){
        collected_data[0][parse_state][data_count[parse_state]]=value;
      }
    }
    // looking for a "deaths" data line
    if (rcv_line.find("deaths", 0)  != std::string::npos){
      analyze_line = rcv_line;
      // delete everything out of the string exept the number
      ReplaceStringInPlace(analyze_line, "      \"deaths\": ", "");
      ReplaceStringInPlace(analyze_line, ",", "");
      ReplaceStringInPlace(analyze_line, "\n", "");
      int value = atoi(analyze_line.c_str());
      // init the value for all countries with the values of the first country
      if(countries_found == 1){
        collected_data[1][0][data_count[0]] = value;
      } else {
        // Otherwise add the values of the other countries
        collected_data[1][0][data_count[0]] = collected_data[1][0][data_count[0]]+value;
      }
      data_count[0]++;
      // Array can contain maximum SCREEN_WIDTH datapoints
      if(data_count[0] >= SCREEN_WIDTH)
        data_count[0] = 0;
      // if we are inside an configure country section
      if(parse_state > 0){
        collected_data[1][parse_state][data_count[parse_state]]=value;
        data_count[parse_state]++;
        if(data_count[parse_state] >= SCREEN_WIDTH)
          data_count[parse_state] = 0;
      }
    }
    // store the last "date" line to get the last actualization date
    // looking for a "date" data line
    if (rcv_line.find("date", 0)  != std::string::npos){
      last_date = rcv_line;
    }
    // simple state machine to select configured countries
    switch (parse_state) {
      case 0: { // search for an configured country
        for(int n=1; n<6; n++){
          if (rcv_line.find(country_names[n].c_str(), 0)  != std::string::npos){
            parse_state = n;
          }
        }
        break;
      }
      case 1: case 2: case 3: case 4: case 5: {
        // check for END of country-section
        if (rcv_line.find("]", 0)  != std::string::npos){
          parse_state = 0;
        }
        break;
      }
      default: {
        // nothing to do here
        break;
      }

    }
    line_count++;
    // Sometimes, we need to wait for new data from the server
    // to not exit the loop before all data are received
    // Last data line is "}"
    for(int n=0; n<10; n++){
      if (!client.available() && rcv_line != "}"){
        delay(550);
      }
    }
  }
  // delete everything out of the last_date string exept the date
  ReplaceStringInPlace(last_date, "      \"date\": \"", "");
  ReplaceStringInPlace(last_date, "\",", "");
  ReplaceStringInPlace(last_date, "\n", "");
  return line_count;
}

//==============================================================
// display the data as curves / graphs including a legend
void display_data_graph(int data_select){
  print_menu(2);
  delay(200);
  M5.Lcd.fillScreen(BLACK);
  // get maximum value to scale the y-axis
  max_y = 0;
  for(int n=1; n<6; n++){
    for(int i = 0; i < data_count[n]; i++){
      if(collected_data[data_select-1][n][i] > max_y)
        max_y = collected_data[data_select-1][n][i];
    }
  }
  // draw line graph
  for(int n=1; n<6; n++){
    int x_scale = round(float(SCREEN_WIDTH) / data_count[n]);
    for(int i = 1; i < data_count[n]; i++){
      M5.Lcd.drawLine((i-1)*x_scale, SCREEN_HEIGHT-round((float(SCREEN_HEIGHT) / max_y) * collected_data[data_select-1][n][i-1]), 
                      i*x_scale, SCREEN_HEIGHT-round((float(SCREEN_HEIGHT) / max_y) * collected_data[data_select-1][n][i]), country_color[n]);
    }
  }
  // draw legend
  M5.Lcd.setFreeFont(FF1);
  M5.Lcd.setCursor(0, 0);
  // headline
  M5.Lcd.printf("\n%s (%s)\n\n", data_name[data_select-1], last_date.c_str());
  // Country name and value
  for(int n=1; n<6; n++){
    M5.Lcd.setTextColor(country_color[n]);
    M5.Lcd.printf("%s:\n%s\n", country_names[n].c_str(), formatNumber(collected_data[data_select-1][n][data_count[n]-1], format_buffer, sizeof(format_buffer)));
  }
  M5.Lcd.setTextColor(WHITE);
}

//==============================================================
// display the data as summarized text output
void display_data_text(int data_select){
  Clear_Screen();
  print_menu(2);
  // draw text output
  M5.Lcd.setFreeFont(FF2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(country_color[data_select-3]);
  M5.Lcd.printf("\n%s:\n\n", country_names[data_select-3].c_str());
  M5.Lcd.setFreeFont(FF1);
  M5.Lcd.setTextColor(WHITE);
  int n_confirmed = collected_data[0][data_select-3][data_count[data_select-3]-1];
  int n_deaths = collected_data[1][data_select-3][data_count[data_select-3]-1];
  M5.Lcd.printf("  confirmed:  %s\n", formatNumber(n_confirmed, format_buffer, sizeof(format_buffer)));
  M5.Lcd.printf("  deaths:     %s\n\n",    formatNumber(n_deaths, format_buffer, sizeof(format_buffer)));
  M5.Lcd.printf("  death rate:    %6.2f%%\n", (100.0/n_confirmed) * n_deaths);
}