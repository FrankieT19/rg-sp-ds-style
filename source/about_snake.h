/* Snake rules and geometry adapted from FrankieT19's DS Style v7.4,
 * ezkernelnew.c Launcher_Snake*; Apache-2.0, see DS-Style-source-LICENSE.txt.
 * Replaces GBA VBlank/video calls with the existing Linux UI, unchanged board. */
#define SNAKE_RGB(r,g,b) ((((r)<<3)|((r)>>2))<<16|(((g)<<3)|((g)>>2))<<8|((b)<<3)|((b)>>2))
static int about_page=-1,snake_active,snake_over,snake_dx=1,snake_dy;
static uint64_t snake_next;
static void about_header(const char*title,const char*number){blit(bars[colour],0,0,W,19);text(3,3,username,0xffffff,11);centered(73,3,94,title,0xffffff);if(number)text(237-(int)strlen(number)*6,3,number,0xffffff,10);}
#define LAUNCHER_SNAKE_COLS 20
#define LAUNCHER_SNAKE_ROWS 14
#define LAUNCHER_SNAKE_CELL 8
#define LAUNCHER_SNAKE_X 40
#define LAUNCHER_SNAKE_Y 33
#define LAUNCHER_SNAKE_MAX_LENGTH 120

typedef struct
{
    uint8_t x[LAUNCHER_SNAKE_MAX_LENGTH];
    uint8_t y[LAUNCHER_SNAKE_MAX_LENGTH];
    uint32_t length;
    uint32_t food_x;
    uint32_t food_y;
    uint32_t rng;
} LauncherSnakeState;

static LauncherSnakeState snake_state;
static LauncherSnakeState *Launcher_GetSnakeState(void){return &snake_state;}

#define launcher_snake_x (Launcher_GetSnakeState()->x)
#define launcher_snake_y (Launcher_GetSnakeState()->y)
#define launcher_snake_length (Launcher_GetSnakeState()->length)
#define launcher_snake_food_x (Launcher_GetSnakeState()->food_x)
#define launcher_snake_food_y (Launcher_GetSnakeState()->food_y)
#define launcher_snake_rng (Launcher_GetSnakeState()->rng)

static uint32_t Launcher_SnakeOccupies(uint32_t x, uint32_t y)
{
    uint32_t i;
    for(i = 0; i < launcher_snake_length; i++)
    {
        if(launcher_snake_x[i] == x && launcher_snake_y[i] == y)
            return 1;
    }
    return 0;
}

static void Launcher_SnakePlaceFood(void)
{
    uint32_t attempts;
    for(attempts = 0; attempts < 512; attempts++)
    {
        launcher_snake_rng = launcher_snake_rng * 1664525u + 1013904223u;
        launcher_snake_food_x = (launcher_snake_rng >> 16) % LAUNCHER_SNAKE_COLS;
        launcher_snake_food_y = (launcher_snake_rng >> 24) % LAUNCHER_SNAKE_ROWS;
        if(!Launcher_SnakeOccupies(launcher_snake_food_x, launcher_snake_food_y))
            return;
    }
    launcher_snake_food_x = 0;
    launcher_snake_food_y = 0;
}

static void Launcher_SnakeReset(void)
{
    uint32_t i;
    launcher_snake_length = 4;
    for(i = 0; i < launcher_snake_length; i++)
    {
        launcher_snake_x[i] = (LAUNCHER_SNAKE_COLS / 2) - i;
        launcher_snake_y[i] = LAUNCHER_SNAKE_ROWS / 2;
    }
    launcher_snake_rng = 0x13579BDFu ^ (uint32_t)ui_millis();
    Launcher_SnakePlaceFood();
}

static uint32_t Launcher_SnakeBoardColour(void)
{
    return dark_mode ? SNAKE_RGB(2, 2, 2) : SNAKE_RGB(29, 29, 29);
}

static uint32_t Launcher_SnakeGridColour(void)
{
    return dark_mode ? SNAKE_RGB(7, 7, 7) : SNAKE_RGB(24, 24, 24);
}

static void Launcher_DrawSnakeFood(void)
{
    rect(LAUNCHER_SNAKE_X + launcher_snake_food_x * LAUNCHER_SNAKE_CELL + 2,
          LAUNCHER_SNAKE_Y + launcher_snake_food_y * LAUNCHER_SNAKE_CELL + 2,
          5, 5, SNAKE_RGB(31, 0, 0));
}

static void Launcher_DrawSnakeSegment(uint32_t x, uint32_t y)
{
    rect(LAUNCHER_SNAKE_X + x * LAUNCHER_SNAKE_CELL + 1,
          LAUNCHER_SNAKE_Y + y * LAUNCHER_SNAKE_CELL + 1,
          7, 7, theme_accent[colour]);
}

static void Launcher_ClearSnakeSegment(uint32_t x, uint32_t y)
{
    rect(LAUNCHER_SNAKE_X + x * LAUNCHER_SNAKE_CELL + 1,
          LAUNCHER_SNAKE_Y + y * LAUNCHER_SNAKE_CELL + 1,
          7, 7, Launcher_SnakeBoardColour());
}

static unsigned snake_high;static int snake_high_loaded,snake_high_dirty;
static void snake_high_load(void){if(snake_high_loaded)return;snake_high_loaded=1;char p[PATHLEN];statepath(p,"snake-high-score.txt");FILE*f=fopen(p,"r");unsigned n;if(f){if(fscanf(f,"%u",&n)==1&&n<=116)snake_high=n;fclose(f);}}
static void snake_high_update(void){unsigned n=launcher_snake_length-4;if(n>snake_high){snake_high=n;snake_high_dirty=1;}}
static void snake_high_save(void){snake_high_update();if(!snake_high_dirty)return;char p[PATHLEN],v[24];statepath(p,"snake-high-score.txt");snprintf(v,sizeof v,"%u\n",snake_high);if(atomic_text(p,v))snake_high_dirty=0;}
static void Launcher_DrawSnakeScore(void){char score[24];snprintf(score,sizeof score,"%u (%u)",launcher_snake_length-4,snake_high);text(237-(int)strlen(score)*6,3,score,0xffffff,20);}

static void Launcher_DrawSnakeBoard(void)
{
    const int board_w = LAUNCHER_SNAKE_COLS * LAUNCHER_SNAKE_CELL;
    const int board_h = LAUNCHER_SNAKE_ROWS * LAUNCHER_SNAKE_CELL;
    uint32_t board = Launcher_SnakeBoardColour();
    uint32_t grid = Launcher_SnakeGridColour();
    uint32_t i;

    rect(LAUNCHER_SNAKE_X, LAUNCHER_SNAKE_Y, board_w, board_h, board);
    for(i = 0; i <= LAUNCHER_SNAKE_COLS; i++)
        rect(LAUNCHER_SNAKE_X + i * LAUNCHER_SNAKE_CELL, LAUNCHER_SNAKE_Y, 1, board_h + 1, grid);
    for(i = 0; i <= LAUNCHER_SNAKE_ROWS; i++)
        rect(LAUNCHER_SNAKE_X, LAUNCHER_SNAKE_Y + i * LAUNCHER_SNAKE_CELL, board_w + 1, 1, grid);

    Launcher_DrawSnakeFood();
    for(i = launcher_snake_length; i > 0; i--)
    {
        uint32_t part = i - 1;
        Launcher_DrawSnakeSegment(launcher_snake_x[part], launcher_snake_y[part]);
    }
    Launcher_DrawSnakeScore();
}

static uint32_t Launcher_SnakeStep(int dx, int dy, uint32_t *tail_x, uint32_t *tail_y, uint32_t *grew)
{
    int new_x = (int)launcher_snake_x[0] + dx;
    int new_y = (int)launcher_snake_y[0] + dy;
    uint32_t ate;
    uint32_t i;

    if(new_x < 0 || new_x >= LAUNCHER_SNAKE_COLS || new_y < 0 || new_y >= LAUNCHER_SNAKE_ROWS)
        return 0;
    if(Launcher_SnakeOccupies((uint32_t)new_x, (uint32_t)new_y))
        return 0;

    *tail_x = launcher_snake_x[launcher_snake_length - 1];
    *tail_y = launcher_snake_y[launcher_snake_length - 1];
    ate = ((uint32_t)new_x == launcher_snake_food_x && (uint32_t)new_y == launcher_snake_food_y);
    *grew = ate && launcher_snake_length < LAUNCHER_SNAKE_MAX_LENGTH;
    if(*grew)
        launcher_snake_length++;
    for(i = launcher_snake_length - 1; i > 0; i--)
    {
        launcher_snake_x[i] = launcher_snake_x[i - 1];
        launcher_snake_y[i] = launcher_snake_y[i - 1];
    }
    launcher_snake_x[0] = (uint8_t)new_x;
    launcher_snake_y[0] = (uint8_t)new_y;
    if(ate)
        Launcher_SnakePlaceFood();
    return 1;
}

static void Launcher_DrawSnakeGameOver(void)
{
    const int x = 51;
    const int y = 65;
    const int w = 138;
    const int h = 38;
    rect(x, y, w, h, theme_accent[colour]);
    text(x + (w - 9 * 6) / 2, y + 6, "Game over", 0xffffff, 37);
    text(x + (w - 17 * 6) / 2, y + 21, "A: Again  B: Back", 0xffffff, 37);
}


#undef SNAKE_RGB
static void snake_reset(void){snake_high_load();Launcher_SnakeReset();snake_dx=1;snake_dy=0;snake_over=0;snake_next=ui_millis()+134;}
static int snake_tick(void){
 if(!snake_active||snake_over||ui_millis()<snake_next)return 0;
 uint32_t tx,ty,grew;snake_next+=134;if(snake_next<ui_millis())snake_next=ui_millis()+134;
 if(!Launcher_SnakeStep(snake_dx,snake_dy,&tx,&ty,&grew)){snake_over=1;snake_high_save();}else snake_high_update();return 1;
}
static int about_action(int k){
 if(about_page<0)return 0;
 if(snake_active){
  if(k==BACK){snake_high_save();snake_active=0;extra_revision++;return 1;}
  if(snake_over){if(k==ACCEPT||k==START){snake_reset();extra_revision++;}return 1;}
  if(k==UP&&snake_dy==0){snake_dx=0;snake_dy=-1;}else if(k==DOWN&&snake_dy==0){snake_dx=0;snake_dy=1;}else if(k==LEFT&&snake_dx==0){snake_dx=-1;snake_dy=0;}else if(k==RIGHT&&snake_dx==0){snake_dx=1;snake_dy=0;}return 1;
 }
 if(k==BACK){about_page=-1;extra_revision++;}
 else if(k==START){snake_active=1;snake_reset();extra_revision++;}
 else if((k==ACCEPT||k==RIGHT||k==PGDN)&&about_page<1){about_page++;extra_revision++;}
 else if((k==LEFT||k==PGUP)&&about_page>0){about_page--;extra_revision++;}
 return 1;
}
static void about_draw(void){
 blit(ui_background(1),0,0,W,H);
 if(snake_active){about_header("Snake",NULL);Launcher_DrawSnakeBoard();if(snake_over)Launcher_DrawSnakeGameOver();return;}
 static const char*lines[][9]={
 {"DS Style", "Created by FrankieT19.", "Originally a Nintendo DS-inspired", "GBA frontend for EZ-FLASH Omega", "and Omega Definitive Edition.", "", "Free and open source.", "", "RG SP port v1.0"},
 {"Credits", "", "LCD grid: based on lcd1x", "Gigaherz and jdgleaver", "", "Pixel Transparency:", "Matt Akins (mattakins)", "", ""}};
 char number[8];snprintf(number,sizeof number,"%d/2",about_page+1);about_header(tr("About"),number);
 for(int i=0;i<9;i++){const char*s=tr(lines[about_page][i]);if(i==0)centered(0,24+i*14,W,s,0);else text(14,24+i*14,s,0,36);}
}
