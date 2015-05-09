#define TRACE_DELAY 1000  // original = 2500
#define NUM_POINTS  19
#define X           6
#define Y           5
#define upPin       4     // the up button pin
#define downPin     3     // the down button pin
#define speakerPin  2     // the down button pin
#define BASE_RATE   10
#include "pitches.h"

char n_led = 1;
char led_pins[] = {8};
char led[] = {100};
char dled[] = {1};

void setup()
{
  pinMode(upPin, INPUT);
  pinMode(downPin, INPUT);
  for (int n = 0; n < n_led; n++){
     pinMode(led_pins[n], OUTPUT);
  }

  pinMode(speakerPin, OUTPUT);
  pinMode(X, OUTPUT);
  pinMode(Y, OUTPUT);
  TCCR0A = (1<<COM0A1 | 0<<COM0A0 | 1<<COM0B1 | 0<<COM0B0 | 1<<WGM01  | 1<<WGM00);
  TCCR0B = (0<<FOC0A | 0<<FOC0B | 0<<WGM02 | 0<<CS02  | 0<<CS01 | 1<<CS00);
  TIMSK0 = (0<<OCIE0B | 0<<TOIE0 | 0<<OCIE0A);
}

typedef struct point_t {
    int x;
    int y;
} point_t;

int draw_point(point_t point, point_t offset);
int draw(point_t *points, int point_len, point_t offset);
int update_fish(point_t *fish_pos);
int is_fish_eaten(point_t whale_pos, point_t *fish_pos);
int reset_fish(point_t *fish_pos);
void update_game_state();
void play_next_note();

const unsigned char whale_points = 21;
point_t whale[whale_points] = {
    {75, 5}, {65, 0}, {60, 5}, {10, 5}, {0, 15}, {5, 40}, {15, 20},
    {30, 35}, {15, 15}, {25, 15}, {45, 35}, {75, 45}, {100, 35},
    {105, 20}, {100, 15}, {75, 15}, {105, 10}, {100, 5}, {75, 5}, {65, 0}, {75, 5}
};

const unsigned char fish_points = 8;
point_t fish[fish_points] = {
    {0, 10}, {10, 20}, {20, 10}, {30, 30}, {30, 0}, {20, 10}, {5, 0}, {0, 12},
};

const unsigned char MAX_XY =  255;
point_t bounding_box[4] = {
    {0, 0}, {0, MAX_XY}, {MAX_XY, MAX_XY}, {MAX_XY, 0}
};

#define crossed_pox_points 8
point_t crossed_box[crossed_pox_points] = {
    {0, 0}, {0, MAX_XY}, {MAX_XY, MAX_XY}, {MAX_XY, 0}, {0, 0}, 
    {MAX_XY, MAX_XY}, {0, MAX_XY}, {MAX_XY, 0}
};

int draw_point(point_t point, point_t offset)
{
    analogWrite(X, point.x + offset.x);
    analogWrite(Y, point.y + offset.y);
    delayMicroseconds(TRACE_DELAY);
    return 0;
}

int draw(point_t *points, int point_len, point_t offset)
{
    for (int t = 0; t < point_len; t++){
        draw_point(points[t], offset);
    }
    return 0;
}

point_t whale_pos = {0, 0};
point_t no_offset = {0, 0};
point_t fish_1 = {MAX_XY*3/4, 100};
unsigned char missed_fish = 0;
int streak = 0;

const unsigned char measure = 24;
const unsigned char WN = measure;
const unsigned char HN = measure/2;
const unsigned char QN = measure/4;
const unsigned char EN = measure/8;
const unsigned char DQN = QN + EN;
const unsigned char SONG_LEN = 63;

int notes[SONG_LEN] = {
  NOTE_A5, NOTE_A5, NOTE_A5, NOTE_A5, NOTE_A5, NOTE_A5,
  NOTE_A5, NOTE_D4, NOTE_F4, NOTE_A5,
  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5,
  NOTE_G5, NOTE_C4, NOTE_E4, NOTE_G5,
  NOTE_A5, NOTE_A5, NOTE_A5, NOTE_A5, NOTE_A5, NOTE_A5,
  NOTE_A5, NOTE_B5, NOTE_C5, NOTE_D5, 
  NOTE_C5, NOTE_A5, NOTE_G5, NOTE_E5,
  NOTE_D4, NOTE_D4, 
  NOTE_A5, NOTE_A5, NOTE_A5, 
  NOTE_A5, NOTE_D4, NOTE_F4, NOTE_A5, 
  NOTE_G5, NOTE_G5, NOTE_G5, 
  NOTE_G5, NOTE_C4, NOTE_E4, NOTE_G5, 
  NOTE_A5, NOTE_A5, NOTE_A5, 
  NOTE_A5, NOTE_B5, NOTE_C5, NOTE_D4, 
  NOTE_C5, NOTE_A5, NOTE_G4, NOTE_E4, 
  NOTE_D4, NOTE_D4, 
};
char durations[SONG_LEN] = {
  QN, EN, EN, QN, EN, EN,
  QN, QN, QN, QN,
  QN, EN, EN, QN, EN, EN,
  QN, QN, QN, QN,
  QN, EN, EN, QN, EN, EN,
  QN, QN, QN, QN,
  QN, QN, QN, QN,
  HN, HN,
  HN, DQN, EN,
  QN, QN, QN, QN, 
  HN, DQN, EN,
  QN, QN, QN, QN, 
  HN, DQN, EN,
  QN, QN, QN, QN, 
  QN, QN, QN, QN, 
  HN, HN,
};
int last_note = 0;
int last_note_duration = 0;
unsigned char song_rest = 0;
unsigned char rest_duration = 0;

int reset_fish(point_t *fish_pos)
{
    fish_pos->y = random(0, 200);
    fish_pos->x = MAX_XY;
}

int is_fish_eaten(point_t whale_pos, point_t *fish_pos)
{
    unsigned int dx = whale_pos.x + 80 - fish_pos->x;
    unsigned int dy = whale_pos.y + 40 - fish_pos->y;
    if ((abs(dx) < 10) && (abs(dy) < 85)){
       streak++;
       return 1; 
    }
    return 0;
}

int update_fish(point_t *fish_pos)
{
    unsigned char dx = random(BASE_RATE/3, BASE_RATE*1.2);    
    if (fish_pos->x <= dx){
      reset_fish(fish_pos);
      missed_fish++;
      for (int t = 0; t < 75; t ++){
        draw(crossed_box, crossed_pox_points, no_offset);
        tone(speakerPin, NOTE_E3, 40);
        tone(speakerPin, NOTE_F4, 40);
      }
      streak = 0;
    }
    fish_pos->x -= dx;
    
    char dy = random(-streak, streak);
    if (fish_pos->y > -2*dy && fish_pos->y < MAX_XY - 2*dy - 50){
      fish_pos->y += dy;
    }

    if (fish_pos->y < 0)      { fish_pos->y = 0;      }
    if (fish_pos->y > MAX_XY) { fish_pos->y = MAX_XY; }
    return 0;
}

void update_game_state(){
    if ((digitalRead(upPin) == HIGH) && (whale_pos.y < MAX_XY - BASE_RATE- 50)) { 
        whale_pos.y += BASE_RATE*1.5;
    } else if ((digitalRead(downPin) == HIGH) && (whale_pos.y > BASE_RATE)){    
        whale_pos.y -= BASE_RATE;
    }
    
    unsigned char sink = random(0, BASE_RATE);
    if (whale_pos.y > sink*2){
       whale_pos.y -= sink;
    }

    if (is_fish_eaten(whale_pos, &fish_1)){
       reset_fish(&fish_1);
    }

    update_fish(&fish_1);
}

void play_next_note()
{
  if (song_rest){
    // we're resting, are we done?
    if (song_rest > rest_duration){
      // rest over
      song_rest = 0;
    } else {
      // increment rest count
      song_rest++;
      return;
    }
  }
   if (last_note_duration > durations[last_note]){
     // we're done with this note, take a rest
     last_note_duration = 0;
     last_note++;
     song_rest = 1;
   } else {
     last_note_duration++;
     tone(speakerPin, notes[last_note], 40);
   }
   if (last_note >= SONG_LEN){
     last_note = 0;
     song_rest = 1;
   }

}

void loop()
{

    update_game_state();
    draw(whale, whale_points, whale_pos);
    draw(fish, fish_points, fish_1);
    
    int led_cycle = 10;
    
    play_next_note();
    for (int n; n < n_led; n++){
      led[n] += dled[n];
      if (led[n] >= 254 || led[n] >= 0){
        dled[n] = -dled[n]; 
      }
      analogWrite(led_pins[n], led[n]);
    }    
    
}
