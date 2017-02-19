#include "pitches.h"
#include <stdio.h>
#include <stdlib.h>

// ----- Variablen -----

const int GAMEredPin = 5;   // PIN'S für GAMELED:   links
const int GAMEgreenPin = 6; //                      dritte von links
const int GAMEbluePin = 7;  //                      rechts
const int speaker = 8;      // 8-ohm speaker


// --------- Spielvariablen -----------

bool Stop = true;           // Spiellampe = Rot
int startbedingung = 0;     // Tickt 5 mal also 5 Messungen

enum zustand {Start, Play, Win, Lose};  // Zustand wo im Spiel man ist
enum zustand gamecheck = Start;

// Alle Variablen die einem Spieler zugewiesen werden
struct player {

    bool inGame = true;
    bool ready = false;
    int irSense = 0;        // IR-Entfernungssensor analog pin A0
    int distance = 0;
    int averaging = 0;
    int preval = 0;
    int val = 0;
    int calc = 0;
    int PLredPin = 0;       // PIN'S für playerLED: links
    int PLgreenPin = 0;     //                      dritte von links
    int PLbluePin = 0;      //                      rechts
};
//Spieler werden erstellt
//Todo: Auf Buttondruck Spieleranzahl erstellen!
struct player player1;
struct player player2;



// ----- Wird einmal bei Systemstart ausgeführt -----
void setup() {
    Serial.begin(9600); // Serial Monitor window
    // ----- Pins für player2LED -------
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    // ----- Pins für gameLED -------
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    // ----- Pins für player1LED -----
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT);
    // Struct Variablen initialisieren
    player1.PLredPin = 9;
    player1.PLgreenPin = 10;
    player1.PLbluePin = 11;
    player1.irSense = A0;

    player2.PLredPin = 2;
    player2.PLgreenPin = 3;
    player2.PLbluePin = 4;
    player2.irSense = A1;
    // ----- Systemstartmelodie -----
    systemstartMelodie();
}

// ----- Wird immer ausgeführt -----
void loop() {
    // ----------------------------------------- Startbedingung prüfen ------------------------------------------------
    BEGIN: startbedingung = 0;
    player1.inGame = true;
    player2.inGame = true;
    while (gamecheck == Start) {
        playerLedColor(&player1, 0, 0, 255);                // Alle LED's BLAU
        playerLedColor(&player2, 0, 0, 255);
        gameLedColor(0, 0, 255);
        irRead();
        // Todo: distance auf richtigen Wert setzen
        if((player1.distance > 420 && player1.distance < 730)
            && (player2.distance > 420 && player2.distance < 730)) { // Bei 5 erfolgreichen Tests gamecheck = Play
            Serial.println("Start Test -- Positiv: ");
            Serial.println(player1.distance);
            Serial.println(player2.distance);
            startbedingung++;
        }
        else {
            Serial.println("Start Test -- Negativ: ");            // Bei negativem Test startbedingung == 0
            Serial.println(player1.distance);
            Serial.println(player2.distance);
            startbedingung = 0;
        }

        if ( startbedingung == 5){                              // Startbedingung erfüllt
            startMelodie();                                     // Startmelodie
            playerLedColor(&player1, 0, 255, 0);                // Beide Spieler GRÜN
            playerLedColor(&player2, 0, 255, 0);                // Beide Spieler GRÜN
            gameLedColor(0, 255, 0);                            // gameLED GRÜN
            gamecheck = Play;
            Stop = false;
        }
    }

    //  ---------------- Startbedingung erfüllt. Spiel geht los! -----------------------------------------
    while (gamecheck == Play){
        /* --- Wenn große Lampe Grün ist ---
         * Siegbedingung prüfen und playerLED WEIß setzen */
        if ( Stop == false ) {
            for (int j = 0; j<=randomTime()*4; j++) {
                irRead();
                if ((player1.distance > 725)
                    && (player1.inGame == true)){
                    playerLedColor(&player1, 255, 255, 255);
                    gamecheck = Win;
                    break;
                }
                else if ((player2.distance > 725)
                        && (player1.inGame == true)){
                    playerLedColor(&player2, 255, 255, 255);
                    gamecheck = Win;
                    break;
                }
            }
            if (gamecheck == Play) changeLight();               // Rot werden
        }

        /* --- Wenn große Lampe rot ist ---
         * Abstandsmessung am Anfang, danach so lange ROT ist prüfen ob Wert sich verändert
         * FALLS JA         ---> Spieler draußen
         * Falls NEIN       ---> Wieder GRÜN werden
         * Falls ALLE JA    ---> Spiel vorbei */
        if ( Stop == true ) {
            for (int i=0; i<=2; i++ ) {                         // 2 Messungen um später Differenz zu vergleichen
                irRead();
                player1.preval = player1.distance;
                player2.preval = player2.distance;
            }
            Serial.print("Preval: ");
            Serial.println(player1.preval);
            Serial.println(player2.preval);
            for (int i=0; i<=randomTime()*4; i++ ) {            // 2x die Sekunde neue Messung
                irRead();
                if ( player1.inGame == true ) player1.val = player1.distance;
                if ( player2.inGame == true ) player2.val = player2.distance;
                Serial.print("Val: ");
                Serial.println(player1.val);
                Serial.println(player2.val);
                // --- Spieler1 hat sich bei Rot bewegt ---
                if (( calcDifference(&player1) > 30 )
                    && (player1.inGame == true)) {
                    player1.inGame = false;
                    playerLedColor(&player1, 255, 0, 0);        // Spieler ROT
                    spielerAusMelodie();
                }
                // --- Spieler2 hat sich bei Rot bewegt ---
                if (( calcDifference(&player2) > 30 )
                    && (player2.inGame == true)){
                    player2.inGame = false;
                    playerLedColor(&player2, 255, 0, 0);        // Spieler ROT
                    spielerAusMelodie();
                }
                // --- Beide Spieler sind draußen ---
                if ( player1.inGame == false && player2.inGame == false) {
                    gamecheck = Start;
                    verlorenMelodie();
                    goto BEGIN;
                }
            }
            if (gamecheck == Play) changeLight();               // Ist Spieler noch im Spiel . Grün werden
        }
    }
    // --- Siegesfall ÜBERGANGSLÖSUNG ---
    if ( gamecheck == Win ){
        gameLedColor(255, 200, 200);
        siegMelodie();
        delay(6000);                                            // ÜBERGANGSLÖSUNG
        gamecheck = Start;                                      // ÜBERGANGSLÖSUNG -. 3s warten und neustart
    }
}

// ----------------------------------------------- Funktionen ---------------------------------------------------------

// Durchschnitt aus den letzten 5 Messwerten (um Messungenauigkeiten vorzubeugen)
void irRead() {
    player1.averaging = 0;
    player2.averaging = 0;

    for (int i=0; i<5; i++) { // sampling of 5 readings from sensor
        player1.distance = analogRead(player1.irSense);
        player2.distance = analogRead(player2.irSense);
        player1.averaging = player1.averaging + player1.distance;
        player2.averaging = player2.averaging + player2.distance;
        delay(55);     // Wait 55 ms between each read
                        // According to datasheet time between each read
                        //  is -38ms +/- 10ms. Waiting 55 ms assures each
                        //  read is from a different sample
    player1.distance = player1.averaging / 5;  // Average out readings
    player2.distance = player2.averaging / 5;  // Average out readings
    }
}

// playerLED in bestimmter Farbe leuchten lassen (RGB)
void playerLedColor(struct player *player, int a, int b, int c) {
    analogWrite(player->PLredPin, a);
    analogWrite(player->PLgreenPin, b);
    analogWrite(player->PLbluePin, c);
}

// GameLED in bestimmter Farbe leuchten lassen (RGB)
void gameLedColor(int a, int b, int c) {
    analogWrite(GAMEredPin, a);
    analogWrite(GAMEgreenPin, b);
    analogWrite(GAMEbluePin, c);
}

// Betrag der Differenz aus 2 Messungen errechnen (Spielerabhängig)
int calcDifference(struct player *player) {
    player->calc = abs(player->val-player->preval);
    return player->calc;
}

/* gibt je nachdem ob Rot- oder Grünphase ist eine zufällige Zeit aus
 * ROT: zwischen 1-10s
 * GRÜN: zwischen 1-3s */

const int randomTime(){
    int randomtime = 0;
    if (Stop == true) {
        randomtime = rand() %10 + 1;
    }
    else {
        randomtime = rand() %4 + 1;
    }
    return(randomtime);
}

// Wechselt die Farbe der gameLED und den bool Stop

void changeLight() {
    if(Stop == false){
        Stop = true;
        gameLedColor(255, 0, 0);
    }
    else {
        Stop = false;
        gameLedColor(0, 255, 0);
    }
}

// ------------------------------------------- Melodien ---------------------------------------------------------------

void systemstartMelodie() {
    // ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
    int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};

    for (int thisNote = 0; thisNote <= 8; thisNote++){
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(speaker, melody[thisNote], noteDuration);
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        noTone(speaker);
    }
}

void startMelodie() {
// ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_A3, NOTE_A3, NOTE_A3, NOTE_C4};
    int noteDurations[] = {4, 4, 4, 4};

    for (int thisNote = 0; thisNote <= 4; thisNote++){
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(speaker, melody[thisNote], noteDuration);
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        noTone(speaker);
    }
}
void spielerAusMelodie() {
    int melody[] = {NOTE_C3, NOTE_A2};
    int noteDurations[] = {2, 2};

    for (int thisNote = 0; thisNote <= 2; thisNote++){
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(speaker, melody[thisNote], noteDuration);
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        noTone(speaker);
    }
}

void verlorenMelodie() {
    //----- Noten für Melodie und Länge der Noten MARIOMELODIE -----
    int melody[] = {NOTE_G3, NOTE_D4, 0, NOTE_D4, NOTE_D4, NOTE_C4, NOTE_B3, NOTE_G3, NOTE_E3, 0, NOTE_E3, NOTE_C3};
    int noteDurations[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};


    for (int thisNote = 0; thisNote <= 12; thisNote++){
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(speaker, melody[thisNote], noteDuration);
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        noTone(speaker);
    }
}

void siegMelodie() {
// ----- Noten für Melodie und Länge der Noten -----
    int melody[] = {NOTE_C4, NOTE_E4, NOTE_G4, 0, NOTE_C5, NOTE_G4, NOTE_C5};
    int noteDurations[] = {4, 4, 4, 4, 2, 4, 2};

    for (int thisNote = 0; thisNote <= 7; thisNote++){
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(speaker, melody[thisNote], noteDuration);
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        noTone(speaker);
    }
}

