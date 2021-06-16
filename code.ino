[code]
/*
 * Otváranie brány prezvnením v2.2
 * Upravené pre GSM modul GA6-B (skúšané s modelom od Goouuu Tech)
 *                            MiroR marec 2020
 *                            23.3.2020 je korona vírus
 * Ak nefunguje mobilna sieť skús CA6_test.ino alebo posielač AT príkazov A6_debug.ino
 * Pokračujem 6.8.2020 MiroR
 * Ruším všetky vymoženosti! Toto je len jednoúčelová vec na otváranie brány.
 * Čísla sa zadávajú iba tu!
 * Signál sa meria každé tri minúty
 */
//*** Do budúcna chcem dorobiť AT+CPBR=1,10 //prečíta zoznam uložených čísiel a mien na SIM karte (napr. prvých desať)


//*** Povolené čísla
String PA[][2] = { //PhoneAutorized, číslo, meno
      {"+421nnnnnnnnn","Xxxx1"},
      {"+421nnnnnnnnn","Xxxx2"},
      {"+421nnnnnnnnn","Xxxx3"},
      {"+421nnnnnnnnn","Xxxx4"},
      {"+421nnnnnnnnn","Xxxx5"},
      {"+421nnnnnnnnn","Xxxx6"},
      {"+421nnnnnnnnn","Xxxx7"},
      {"+420nnnnnnnnn","Xxxx8"},
      {"+420nnnnnnnnn","Xxxx9"},
      {"+421nnnnnnnnn","Xxxx10"},
};  


#include "Nokia_5110.h"
//*** Tento Nokia test má 0-14 znakov a 0-5 riadkov

#define RST 9
#define CE 8
#define DC 7
#define DIN 6
#define CLK 5

Nokia_5110 lcd = Nokia_5110(RST, CE, DC, DIN, CLK);

#include <SoftwareSerial.h>
SoftwareSerial GA6(10,11); // RX,TX
int Incomingch; //int Incomingxx;
String data;//Fdata;
const int maxSIM = 10;//začínam od 1
String cislo; String meno;
boolean label=true;


//*** menu ***
String nadpis="** GSM rele **";
String aktual="   MiroR  2020";
String zobak="> ";
String Poradie = "";  
String Mobil = "";  
String Meno = ""; 
//*** end menu ***

//int resetPIN = 2;
int rele = 4;
int led = 12;
int contrast=53; //60 is the default value set by the driver
int period = 180000;//čas opakovaného merania signálu raz za 30 sec. (30000) alebo 3 min. (180000)
unsigned long time_now = 0;
uint32_t runTime = -99999;       // time for next update

void setup() {
  pinMode(rele,OUTPUT);
  digitalWrite(rele,HIGH);
  pinMode(led,OUTPUT);
  digitalWrite(led,LOW);  
  lcd.setTemperatureCoefficient(3);//teplotný koeficient riadiaceho napätia LCD hodnoty 1-3
  lcd.setContrast(contrast); 
  Header(nadpis,aktual);
  delay(3000);  
  Serial.begin(9600); 
  GA6.begin(9600);
  Serial.println("A6_Otv_brany_4.ino\n");
 /*  
  GA6.println("AT+IPR=9600");//ak treba zmena, lebo továrenské nastavenie je 115200
*/   



//*** menu ***
  Serial.println(F("|***************************************|"));
  Serial.println(F("|***|  Otvaranie brany prezvonenim  |***|"));
  Serial.println(F("|***|         GSM modul A6          |***|"));
  Serial.println(F("|***|            MiroR              |***|"));
  Serial.println(F("|***************************************|"));
  Serial.println(F(""));
//*** end menu ***
  TEST:
  Serial.print(F("Nazov modulu: "));
  GA6.println("AT+GMM");//model A6 //bez otázniková rutina
  delay(500);
  while(GA6.available()){
   Serial.write(GA6.read());//Forward what Software Serial received to Serial Port
  }
  if(simReady()){Serial.println("\nTEST OK");}else{delay(1000);goto TEST;}//testujem či žije modul miesto bežného AT žiadam rovno značku výrobcu do blba :)
  GA6.println("AT+CLIP=1");//vynucujem si používanie CLIPU // príkaz
  while(GA6.find("OK")){}
  GA6.println("AT+CSQ");//meranie sily signálu  
  while(GA6.find("OK")){}
  readAuthorized();
}

void loop() {
if(label){
  GA6.println("AT+CSQ");//meranie sily signálu po prvom pripojení 
  delay(100);
  label=false;
  } 
if(millis() >= time_now + period){//periodické meranie sily signálu
  time_now += period;      
  GA6.println("AT+CSQ");
  delay(10);       
  }   
while(GA6.available()){// Čakám na prichádzajúci hovor alebo správy od GSM modulu
   Incomingch = GA6.read(); // Načítam data zo sériovej linky
if (Incomingch == 10 || Incomingch ==13){ //Ak príde LF alebo CR znamená to, že dokončila jedna veta
// Serial.print("Test data: ");Serial.println(data);  //Len pre testovací výpis
if(data.indexOf("+CLIP:")>-1){//Kto volá a filter známich účastníkov
  data.replace("+CLIP: ","");
  cislo = getValue(data,',',0);//||
//  Serial.print("Vola: ");Serial.println(cislo); //len pre testovacie účely
  for(int c=0;c<maxSIM+1;c++){//porovnávam všetky povolené čísla
    String pos = PA[c][0].substring(1,14);//povolené čísla sú v poli PA[]
    cislo.replace("\"","");//mažem nepotrebné znaky
    if(pos==cislo){
       delay(10);
        GA6.println("ATH");//položím hovor, aby nebol spoplatnený
        Status(cislo,PA[c][1]);//na serial port a LCD vypíšem volané číslo
        Rele();
        return;//odskok zo slučky
      }
      if(c>=maxSIM){
        delay(100);//ak dáš tento čas dostatočne dlhý (>10000), môžeš priviesť neznámeho až do hlasovej schránky a tak mu spoplatníš hovor
        GA6.println("ATH");//braána sa samozrejme neotvorí
        Status(cislo,"Bez opravnenia");
      }
 }
}
else if(data.indexOf("+CSQ:")>-1){//sila a kvalita signálu
  data.replace("+CSQ: ","");  
  String Sig = getValue(data,',',0);
  Serial.print("\nRSSI (sila signalu; 2 - 31 /0-100%/)= ");Serial.print(Sig);Serial.print("\n");
  Serial.print("BER  (chybove BITY; 99 je bezchybne)= ");Serial.print(getValue(data,',',1));Serial.print("\n");
  int sig = map(Sig.toInt(),2,31,0,100);
  Serial.print("Sila signalu v % = ");Serial.println(sig);
  
  //lcd.clear(2,0,83);//Čistím ak bola náhodou predtým značka Resetuj GSM! clear(riadok,od,do)
  LCDvypis(2,"Signal  ~ "+String(sig)+"%");
    }                           
 /* tento chvost nemeniť! */
 data = "";    
    }else
    {
  String newchar = String (char(Incomingch)); //konvertujte znak na reťazec pomocou reťazcových objektov
  data = data +newchar; // zreťazenie reťazca
  }
 }  
}


boolean simReady(){//test
GA6.println("AT");
delay(100);
if (GA6.find("OK")) {
Serial.print(F("Modul reaguje na zakladny prikaz AT\n"));  
Serial.print(F("Kontrola siete (max. 1 minutu).."));
for (int i = 0; i<20; i++) {
// time to register on network 2.5 sec
for (int i=0;i<5;i++) {

digitalWrite(led, HIGH); // LED blink
//Serial.print("HIGH ");
delay(250);
digitalWrite(led, LOW);
//Serial.print("LOW ");
delay(250);
}
Serial.print(".");

GA6.println("AT+CREG?"); //0=not registered 0,1=registered on home network 0,2=not registered but searching 0,3=denied 0,4=unknown 0,5=roaming
delay(1000);
if (GA6.find("1,1")) {
Serial.println(F("\nRegistracia do siete OK"));
GA6.println("AT+CSQ");
return true;
  }
}
Serial.println(F("\nNepodarilo sa zaregistrovat za 1 minutu"));
return false;
}
else
  {
Serial.println(F("GA6 - modul nekomunikuje!"));
  lcd.setCursor(0, 3);//stlpec, riadok
  lcd.print("Resetuj GSM!");
return false;
  }
}    


void Rele(){
    Serial.println("Otvaram branu");
    digitalWrite(rele,LOW);
    lcd.setCursor(0, 5);
    lcd.print("Inpulz ON ");  
    delay(2000);
    digitalWrite(rele,HIGH);
    lcd.setCursor(0, 5);
    lcd.print("Inpulz OFF");  
} 


String getValue(String data, char separator, int index){ /*Vďaka za toto https://arduino.stackexchange.com/a/9170 */
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void Status(String cislo,String kto){
  cislo.replace("\"",""); kto.replace("\"","");
  lcd.clear(3,0,83);
  lcd.clear(4,0,83);lcd.clear(5,0,83);
  lcd.setCursor(0, 3);//stlpec, riadok
  lcd.print(cislo); 
  lcd.setCursor(0, 4);
  lcd.print(kto);
  Serial.print("Cislo: "+cislo);
  Serial.println(" meno: "+kto);
    }

void Header(String nadpis,String aktual){
  lcd.clear(0,0,83);lcd.clear(1,0,83);
  lcd.setCursor(2, 0);//stlpec, riadok
  lcd.print(nadpis); 
  lcd.setCursor(0, 1);
  lcd.print(aktual);  
    }
    
void LCDvypis(int riadok,String text){
  cislo.replace("\"","");
  lcd.setCursor(0, riadok);//stlpec, riadok
  lcd.print(text); 
    }     

void readAuthorized(){
  Serial.println("Opravneny uzivatelia:");
  for(int m=0;m<maxSIM;m++){
    Serial.print(m+1);
    Serial.print(":");
    Serial.print(PA[m][0]);
    Serial.print(", ");
    Serial.println(PA[m][1]);  
  }
  Serial.println("\n");
}


[/code]
