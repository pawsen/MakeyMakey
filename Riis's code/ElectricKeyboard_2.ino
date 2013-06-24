#define NBUTTONS 11  // number of buttons
#define NREPEATS 1   // repetitions of '0' before buttonpress is accepted

uint8_t buffer[8];   /* Keyboard report buffer */

char pinbut[NBUTTONS] = {2,3,4,5,6,7,8,9,10,11,12};
//char pinbut[NBUTTONS] = {2,3,4,5,6};
unsigned char counter[NBUTTONS];

void setup() {
  char i;
  Serial.begin(9600);
  for (i=0; i<NBUTTONS; i++)
  {
    pinMode(pinbut[i], INPUT);    //set to input
    digitalWrite(pinbut[i], LOW); //disable internal pullup
  }
  //Serial.println("Done Initializing..");
}

void loop() {
  char i;
  for (i=0; i<NBUTTONS; i++)
  {
    if(digitalRead(pinbut[i]) == 0)
    {
      if (counter[i] >= NREPEATS)
      {
        buffer[2] = i+5;                // key 16 = 'm'
        Serial.write(buffer, 8);	// Send keypress
        //Serial.println((int)buffer[2]);
      }
      else
      {
        counter[i]++;
      }
    }
    else
    {
      buffer[2] = 0;
      Serial.write(buffer, 8);	// Send key release     
      counter[i] = 0;
    }    
    //delay(1);
  }
}

