#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <WiFiClient.h>



boolean read_EEPROM();
void write_EEPROM();
void checkConnection();
void mainpage();
void get_IP();
void clear_EEPROM();
void restart_ESP();



ESP8266WebServer webServer(80);
WiFiClient client;
PubSubClient mqtt_client;
const char *Server_MQTT="192.168.4.107";
const int  port_MQTT=1883;

char *topic_sub_test="from-esp8266-test";

char* ssid_ap = "Config_WiFi";
char* pass_ap = "12345678";

IPAddress ip_ap(192,168,1,1);
IPAddress gateway_ap(192,168,1,1);
IPAddress subnet_ap(255,255,255,0);
String ssid="";
String pass="";
String id="";
byte temp2[7]={110, 111, 100, 101,48, 50,32};




void callback(char *topic,byte *payload, unsigned int length){
  char msg[255]="";
  Serial.print("Received from ");
  Serial.println(topic);
  Serial.print("Message : ");
  for(size_t i=0; i < length;i++){
    msg[i]=(char)payload[i];
  }
  delay(10);
  Serial.print(msg);
  Serial.println();
  Serial.println("------------------------------------------");
}


const char MainPage[] PROGMEM = R"=====(
<!DOCTYPE html> 
<html>
 <head> 
     <title>CONFIG WIFI FOR NODE</title> 
     <style> 
        body {text-align:center;background-color:#222222; color:white}
        input {
          height:25px; 
          width:270px;
          font-size:20px;
          margin: 10px auto;
        }
        #content {
          border: white solid 1px; 
          padding:5px;  
          height:375px; 
          width:330px; 
          border-radius:20px;
          margin: 0 auto
        }

        .button_wifi{
          height:50px; 
          width:90px; 
          margin:10px 0;
          outline:none;
          color:white;
          font-size:15px;
          font-weight: bold;
        }
        #button_save {
          background-color:#00BB00;
          border-radius:5px;
        }
        #button_restart {
          background-color:#FF9900;
          border-radius:5px;
        }
        #button_reset {
          background-color:#CC3300;
          border-radius:5px;
        }
     </style>
     <meta name="viewport" content="width=device-width,user-scalable=0" charset="UTF-8">
 </head>
 <body> 
    <div><h1>CONFIG WIFI FOR NODE</h1></div>
    <div id="content"> 
      <div id="wifisetup" style="height:340px; font-size:20px; display:block";>
        <div style="text-align:left; width:270px; margin:5px 25px">SSID: </div>
        <div><input id="ssid"/></div>
        <div style="text-align:left; width:270px; margin:5px 25px">Password: </div>
        <div><input id="pass"/></div>
        <div style="text-align:left; width:270px; margin:5px 25px">ID: </div>
        <div><input id="id"/></div>
        <div>
          <button id="button_save" class="button_wifi" onclick="writeEEPROM()">SAVE</button>
          <button id="button_restart" class="button_wifi" onclick="restartESP()">RESTART</button>
          <button id="button_reset" class="button_wifi" onclick="clearEEPROM()">RESET</button>
        </div>
        <div>IP connected: <b><span id="ipconnected"></span></b></div>
        <div id="reponsetext"></div>
      </div>
    </div>
    <script>
      //-----------Hàm khởi tạo đối tượng request----------------
      function create_obj(){
        td = navigator.appName;
        if(td == "Microsoft Internet Explorer"){
          obj = new ActiveXObject("Microsoft.XMLHTTP");
        }else{
          obj = new XMLHttpRequest();
        }
        return obj;
      }
      //------------Khởi tạo biến toàn cục-----------------------------
      var xhttp = create_obj();//Đối tượng request cho setup wifi
      //===================Khởi tạo ban đầu khi load trang=============
      window.onload = function(){
        getIPconnect();//Gửi yêu cầu lấy IP kết nối
      }
      //===================IPconnect====================================
      //--------Tạo request lấy địa chỉ IP kết nối----------------------
      function getIPconnect(){
        xhttp.open("GET","/getIP",true);
        xhttp.onreadystatechange = process_ip;//nhận reponse 
        xhttp.send();
      }
      //-----------Kiểm tra response IP và hiển thị------------------
      function process_ip(){
        if(xhttp.readyState == 4 && xhttp.status == 200){
          //------Updat data sử dụng javascript----------
          ketqua = xhttp.responseText; 
          document.getElementById("ipconnected").innerHTML=ketqua;       
        }
      }
      function writeEEPROM(){
        if(Empty(document.getElementById("ssid"), "Please enter ssid!")&&Empty(document.getElementById("pass"), "Please enter password")){
          var ssid = document.getElementById("ssid").value;
          var pass = document.getElementById("pass").value;
          var id =  document.getElementById("id").value;
          xhttp.open("GET","/writeEEPROM?ssid="+ssid+"&pass="+pass+"&id="+id,true);
          xhttp.onreadystatechange = process;//nhận reponse 
          xhttp.send();
        }
      }
      function clearEEPROM(){
        if(confirm("Do you want to delete all saved wifi configurations?")){
          xhttp.open("GET","/clearEEPROM",true);
          xhttp.onreadystatechange = process;//nhận reponse 
          xhttp.send();
        }
      }
      function restartESP(){
        if(confirm("Do you want to reboot the device?")){
          xhttp.open("GET","/restartESP",true);
          xhttp.send();
          alert("Device is restarting! If no wifi is found please press reset!");
        }
      }
      //-----------Kiểm tra response -------------------------------------------
      function process(){
        if(xhttp.readyState == 4 && xhttp.status == 200){
          //------Updat data sử dụng javascript----------
          ketqua = xhttp.responseText; 
          document.getElementById("reponsetext").innerHTML=ketqua;       
        }
      }
     //----------------------------CHECK EMPTY--------------------------------
     function Empty(element, AlertMessage){
        if(element.value.trim()== ""){
          alert(AlertMessage);
          element.focus();
          return false;
        }else{
          return true;
        }
     }

    </script>
 </body> 
</html>
)=====";

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  delay(100);
  
  if(read_EEPROM()){
    checkConnection();
  }else{
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(ip_ap, gateway_ap, subnet_ap);
    WiFi.softAP(ssid_ap,pass_ap,1,false);
    Serial.println("Soft Access Point mode!");
    Serial.print("Please connect to");
    Serial.println(ssid_ap);
    Serial.print("Web Server IP Address: ");
    Serial.println(ip_ap);
  }
  webServer.begin();
  webServer.on("/",mainpage);
  webServer.on("/getIP",get_IP);
  webServer.on("/writeEEPROM",write_EEPROM);
  webServer.on("/restartESP",restart_ESP);
  webServer.on("/clearEEPROM",clear_EEPROM);

 if(WiFi.status()==WL_CONNECTED){
  mqtt_client.setClient(client);
  mqtt_client.setServer(Server_MQTT,port_MQTT);
  mqtt_client.setCallback(callback);
  while(!mqtt_client.connect(id.c_str())){
    Serial.print(".");
    delay(100);
    }
  Serial.println("Connected to Server");
  mqtt_client.setBufferSize(255);

 }
}
void loop() {
  webServer.handleClient();
  if(mqtt_client.connected()){

  mqtt_client.beginPublish(topic_sub_test,11,false);
  mqtt_client.write(temp2,sizeof(temp2));
  mqtt_client.write((byte)random(48,57));
  mqtt_client.write((byte)random(48,57));
  mqtt_client.write((byte)46);
  mqtt_client.write((byte)random(48,57));
  mqtt_client.endPublish();
  mqtt_client.disconnect();
  
  }
  else{
    delay(1000);
    mqtt_client.connect(id.c_str());
  }
  
}

void mainpage(){
  String s = FPSTR(MainPage);
  webServer.send(200,"text/html",s);
}
void get_IP(){
  String s = WiFi.localIP().toString();
  webServer.send(200,"text/html", s);
}
boolean read_EEPROM(){
  Serial.println("Reading EEPROM...");
  if(EEPROM.read(0)!=0){
    for (int i=0; i<32; ++i){
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (int i=32; i<96; ++i){
      pass += char(EEPROM.read(i));
    }
    Serial.print("PASSWORD: ");
    Serial.println(pass);
      for (int i=64; i<96; ++i){
      id += char(EEPROM.read(i));
    }
    Serial.print("ID: ");
    Serial.println(id);
    
    ssid = ssid.c_str();
    pass = pass.c_str();
    id   = id.c_str();
    return true;
  }else{
    Serial.println("Data wifi not found!");
    return false;
  }
}
void checkConnection() {
  Serial.println();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  Serial.print("Check connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid,pass);
  int count=0;
  while(count < 50){
    if(WiFi.status() == WL_CONNECTED){
      Serial.println();
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("Web Server IP Address: ");
      Serial.println(WiFi.localIP());
      return;
    }
    delay(200);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip_ap, gateway_ap, subnet_ap);
  WiFi.softAP(ssid_ap,pass_ap,1,false);
  Serial.println("Soft Access Point mode!");
  Serial.print("Please connect to");
  Serial.println(ssid_ap);
  Serial.print("Web Server IP Address: ");
  Serial.println(ip_ap);
}
void write_EEPROM(){
  ssid = webServer.arg("ssid");
  pass = webServer.arg("pass");
  id   = webServer.arg("id");
  Serial.println("Clear EEPROM!");
  for (int i = 0; i < 512; ++i) {
    EEPROM.write(i, 0);           
    delay(10);
  }
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(i, ssid[i]);
  }
  for (int i = 0; i < pass.length(); ++i) {
    EEPROM.write(32 + i, pass[i]);
  }
  for (int i = 0; i < id.length(); ++i) {
    EEPROM.write(64 + i, id[i]);
  }
  EEPROM.commit();
  Serial.println("Writed to EEPROM!");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASS: ");
  Serial.println(pass);
  String s = "Wifi configuration saved!";
  webServer.send(200, "text/html", s);
}
void restart_ESP(){
  ESP.restart();
}
void clear_EEPROM(){
  Serial.println("Clear EEPROM!");
  for (int i = 0; i < 512; ++i) {
    EEPROM.write(i, 0);           
    delay(10);
  }
  EEPROM.commit();
  String s = "Device has been reset!";
  webServer.send(200,"text/html", s);
}
