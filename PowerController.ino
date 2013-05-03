
#include <LiquidCrystal.h>

#define NONE_PIN -1
#define BUTTON1_PIN 8
#define BUTTON2_PIN 9
#define BUTTON3_PIN 10
#define OUT1_PIN 11
#define OUT2_PIN 12
#define LED_PIN 13

#define LCD_RS_PIN 7
#define LCD_ENABLE_PIN 6
#define LCD_D4_PIN 5
#define LCD_D5_PIN 4
#define LCD_D6_PIN 3
#define LCD_D7_PIN 2

#define STATE_NONE -1   // No state change
#define STATE_START 0
#define STATE_OUTPUT_SELECT 1
#define STATE_OUTPUT_SHOW 2
#define STATE_OUTPUT_EDIT 3
#define STATE_END (STATE_OUTPUT_EDIT + 1)     // Keep this last_state + 1

#define OS_NONE -1
#define OS_OFF 0
#define OS_ON 1
#define OS_END (OS_ON + 1)

char *OS_OFF_TEXT = "Off";
char *OS_ON_TEXT = "On";

struct state {
  int stateId;

  char *topRowFixedText;
  char *btn1FixedText;
  char *btn2FixedText;
  char *btn3FixedText;
  
  char * (*topRowTextFunc)();
  char * (*btn1TextFunc)();
  char * (*btn2TextFunc)();
  char * (*btn3TextFunc)();
  
  int (*btn1Function)();
  int (*btn2Function)();
  int (*btn3Function)();
};

struct state states[] = {
  { STATE_START, 
    "PowerManager", "Outputs", NULL, NULL, 
    NULL, NULL, NULL, NULL,
    toStateOutputSelect, NULL, NULL },
  { STATE_OUTPUT_SELECT, 
    NULL, "Next", "Top", "Set",
    currentOutputStateText, NULL, NULL, NULL,
    nextOutput, toStateStart, toStateOutputShow },
  { STATE_OUTPUT_SHOW,
    NULL, NULL, "Up", NULL,
    currentOutputStateText, nextOutputStateBtnText, NULL, NULL,
    toStateOutputEdit, toStateOutputSelect, NULL },
  { STATE_OUTPUT_EDIT,
    NULL, NULL, "Up", "Yes",
    switchStateText, nextOutputStateBtnText, NULL, NULL,
    toStateOutputEdit, toStateOutputSelect, stateCommit },
  { STATE_END, 
    NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL },
};

struct output {
  char *name;
  int state;
  int pendingState;
  int pin;
};

struct output outputs[] = {
  { "Boiler", OS_OFF, OS_NONE, OUT1_PIN },
  { "Charger", OS_ON, OS_NONE, OUT2_PIN },
  { NULL, OS_NONE, OS_NONE, NONE_PIN },
};

int currentState = 0;
int prevBtnDown = 0;
int btnDown = 0;
int currentOutput = 0;

// LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(LCD_RS_PIN, LCD_ENABLE_PIN,
                  LCD_D4_PIN, LCD_D5_PIN,
                  LCD_D6_PIN, LCD_D7_PIN);

int toStateOutputSelect() {
  return STATE_OUTPUT_SELECT;
}

int toStateOutputShow() {
  outputs[currentOutput].pendingState = outputs[currentOutput].state;

  return STATE_OUTPUT_SHOW;
}

int toStateStart() {
  return STATE_START;
}

int toStateOutputEdit() {
  toggleOutputPendingState();
  return STATE_OUTPUT_EDIT;
}

int stateCommit() {  
  outputs[currentOutput].state = outputs[currentOutput].pendingState;
  
  return STATE_OUTPUT_SHOW;
}

int nextOutput() {
  ++currentOutput;
  if (NULL == outputs[currentOutput].name) {
    currentOutput = 0;
  }
  return STATE_NONE;
}

void toggleOutputPendingState() {
  ++outputs[currentOutput].pendingState;
  if (outputs[currentOutput].pendingState >= OS_END) {
    outputs[currentOutput].pendingState = OS_OFF;
  }
}

char *switchStateText() {
  static char switchBuffer[30];  // NOTE Limits the length of outputs
  
  strcpy(switchBuffer, "Switch ");
  strcat(switchBuffer, osOutputStateText(outputs[currentOutput].pendingState));
  strcat(switchBuffer, "?");
  
  return switchBuffer;
}

char *osOutputStateText(int outputState) {
  switch (outputState) {
    case OS_ON:
      return OS_ON_TEXT;
      break;
    case OS_OFF:
    default:
      return OS_OFF_TEXT;
      break;
  } 
  return "";
}

char *nextOutputStateBtnText() {
  return osOutputStateText(outputs[currentOutput].pendingState + 1);
}

char *currentOutputStateText() {
  static char stateBuffer[30];  // NOTE: Limits the length of outputs!

  strcpy(stateBuffer, outputs[currentOutput].name);
  strcat(stateBuffer, " ");
  strcat(stateBuffer, osOutputStateText(outputs[currentOutput].state));

  return stateBuffer;
}

void findNextState(int nextState) {
  if (nextState >= STATE_END) {
    currentState = STATE_START;
  }
  else if (STATE_NONE == nextState) {
    // Keep the current state
    return;
  }
  currentState = nextState;
}

struct state *findCurrentState() {
   return &(states[currentState]);
}

int checkButtons() {
  int buttons[] = { BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, -1 };
  
  for (int i = 0; -1 != buttons[i]; ++i) {
    if (LOW == digitalRead(buttons[i])) {
      return buttons[i];
    }
  }

  return 0;
}

char *stateText(char *fixedText,
                char * (*textFunc)()) {
  if (NULL != textFunc) {
    return textFunc();
  }
  if (NULL != fixedText) {
    return fixedText;
  }
  return "";
}

void printTexts(struct state *state) {
  // Print to Serial for debugging
  Serial.print("Top row: ");
  Serial.println(stateText(state->topRowFixedText,
                           state->topRowTextFunc));
  Serial.print("Bottom row: ");
  Serial.print(stateText(state->btn1FixedText,
                         state->btn1TextFunc));
  Serial.print(" | ");
  Serial.print(stateText(state->btn2FixedText,
                        state->btn2TextFunc));
  Serial.print(" | ");
  Serial.println(stateText(state->btn3FixedText,
                           state->btn3TextFunc));
  
  // Prepare the texts for the LCD                         
  char bottomRow[20];
  strcpy(bottomRow, stateText(state->btn1FixedText,
                              state->btn1TextFunc));
  strcat(bottomRow, " ");
  strcat(bottomRow, stateText(state->btn2FixedText,
                              state->btn2TextFunc));
  strcat(bottomRow, " ");
  strcat(bottomRow, stateText(state->btn3FixedText,
                              state->btn3TextFunc));
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(stateText(state->topRowFixedText,
                      state->topRowTextFunc));
  lcd.setCursor(0, 1);
  lcd.print(bottomRow);
}

int mapOutputState(int state) {
  if (OS_ON == state) {
    return HIGH;
  }
  return LOW;
}

void setOutputs() {
  int i = 0;
  
  while (NONE_PIN != outputs[i].pin) {
    digitalWrite(outputs[i].pin, mapOutputState(outputs[i].state));
    ++i; 
  }
}


void update() {
  struct state *state = findCurrentState();
  printTexts(state);
  setOutputs();
}

void setLed(int btnDown) {
  if (0 == btnDown) {
    digitalWrite(LED_PIN, LOW);
  }
  else {
    digitalWrite(LED_PIN, HIGH);
  }
}

void setupPins() {
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(OUT1_PIN, OUTPUT);
  pinMode(OUT2_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}

void setup() {
  Serial.begin(9600);
  setupPins();
  lcd.begin(12, 2);
  update();
}

void loop() {
  btnDown = checkButtons();
  if (btnDown != prevBtnDown) {
     struct state *s = findCurrentState();
 
     int (*func)() = NULL;
    
     switch (btnDown) {
       case BUTTON1_PIN:
         func = s->btn1Function;
         break;
       case BUTTON2_PIN:
         func = s->btn2Function;
         break;
       case BUTTON3_PIN:
         func = s->btn3Function;
         break;
       default:
         break;
     }
     
     if (NULL != func) {
       findNextState(func());
       update();
     }
 
     prevBtnDown = btnDown;
     setLed(btnDown);
  }

  delay(100);
}

