#include <vo_stdio.h>
#include <lcd.h>
#include <vo_gpio.h>
#include <string.h>
#include <vo_fat.h>
#include <rgb565.h>
#include <romfont1005e.h>
#include <sysmemory.h>
#include <vo_fat.h>
#include <vo_fatdirops.h>
#include <libaudiodec.h>
#include <vstypes.h>
#include <uimessages.h>
#include <apploader.h>
#include <volink.h>

#include "UI.h"

DLLIMPORT(cycVolume)
extern int cycVolume;

DLLIMPORT(batteryLevel)
extern int batteryLevel;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// #include "lcd-ili9341.h"

#define FILE_NAME_CHARS 256

// typedef enum Menu_State {
//     INIT_SCREEN = 0,
//     MAIN_MENU,
//     SONG_MENU1,
//     PLAYLIST_MENU,
//     NOW_PLAYING,
// } Menu_State_t;


#define INIT_SCREEN   0
#define MAIN_MENU     1
#define SONG_MENU     2
#define PLAYLIST_MENU 3
#define NOW_PLAYING   4

#define NORMAL_MODE 0
#define SHUFFLE_MODE 1
#define REPEAT_MODE 2


#define PAD4 4
#define HEADER_START_X 0
#define HEADER_START_Y 0
#define HEADER_END_X lcd0.width-1
#define HEADER_END_Y 16

#define MAIN_WINDOW_START_X 0
#define MAIN_WINDOW_START_Y 20
#define MAIN_WINDOW_END_X lcd0.width-1
#define MAIN_WINDOW_END_Y lcd0.height-1

#define LIST_ITEM_START_X 12
#define LIST_ITEM_END_X   lcd0.width-12
#define LIST_ITEM_HEIGHT  30

#define PLAYBACK_START_X 0
#define PLAYBACK_START_Y lcd0.height-30

#define ALBUM_ART_MAX_WIDTH  150
#define ALBUM_ART_MAX_HEIGHT 150

typedef struct {
    u_int16      menu_state : 4;
    u_int16      paused     : 1;
    u_int16      menu_item  : 8;
    u_int16      mode       : 3;
} UI_State_t;

static UI_State_t sg_UI_STATE;

static char sg_SONG_NAME[50];
static char sg_ALBUM_NAME[50];
static char sg_ARTIST_NAME[50];

// menu items
#define MENU_LENGTH 6
#define MAX_PLAYLISTS 16
#define MAX_MENU_ITEMS MAX_PLAYLISTS + 2  // 22 playlists + now playing and all music
#define MENU_ITEM_LENGTH 40

static char sg_MENU_ITEMS[MAX_MENU_ITEMS][MENU_ITEM_LENGTH];
static u_int16 sg_LIST_INDEX;
static u_int16 sg_LIST_LENGTH;


// static u_int16 sg_SONG_LENGTH;
static u_int16 sg_PLAYBACK_TIME;
static u_int16 sg_PLAYBACK_PERCENT_COMPLETE;

u_int16 LcdDrawBox(u_int16 x1, u_int16 y1, u_int16 x2, u_int16 y2, u_int16 border_width,
                   u_int16 border_color, u_int16 fill_color) {
    LcdFilledRectangle(x1, y1, x2, y2, NULL, border_color);
    LcdFilledRectangle(x1 + border_width, y1 + border_width, x2 - border_width, y2 - border_width, NULL, fill_color);
}

void LcdClearScreen() {
    // Clear the screen
	LcdFilledRectangle(0,0,lcd0.width-1,lcd0.height-1,0,lcd0.backgroundColor);
}

void LcdClearMainWindow() {
    // clear the main window
    LcdFilledRectangle(MAIN_WINDOW_START_X, MAIN_WINDOW_START_Y, MAIN_WINDOW_END_X, MAIN_WINDOW_END_Y,
                       NULL, lcd0.backgroundColor);
}

void uiResetSong() {
    strcpy(sg_SONG_NAME, "Unknown Song");
    strcpy(sg_ALBUM_NAME, "Unknown Album");
    strcpy(sg_ARTIST_NAME, "Unknown Artist");
}

void uiMetadataDecodeCallBack(s_int16 index, u_int16 message, u_int32 value) {
    switch(message) {
    case UIMSG_TEXT_SONG_NAME:
        strcpy(sg_SONG_NAME, (char*) value);
        break;
    case UIMSG_TEXT_ARTIST_NAME:
        strcpy(sg_ARTIST_NAME, (char*) value);
        break;
    case UIMSG_TEXT_ALBUM_NAME:
        strcpy(sg_ALBUM_NAME, (char*) value);
    // default:
        // printf("Error: Invalid UI message for metadata decode\n");
    }
}


ioresult uiInit(void) {
    int i, j;
    LcdInit(0);

    LcdDrawBox((lcd0.width/2)-65, (lcd0.height/2)-25, (lcd0.width/2)+65,
               (lcd0.height/2)+25, 2, COLOR_BLACK, lcd0.backgroundColor);
    lcd0.textColor = COLOR_BLACK;
    LcdTextOutXY((lcd0.width/2)-60, (lcd0.height/2), "AMP3 Booting...");

    sg_UI_STATE.menu_state = INIT_SCREEN;
    sg_UI_STATE.mode = NORMAL_MODE;
    sg_UI_STATE.paused = TRUE;

    uiResetSong();
    
    sg_PLAYBACK_PERCENT_COMPLETE = 0;
    sg_PLAYBACK_TIME = 0;

    // load playlist names
    sg_LIST_INDEX = 0;
    sg_LIST_LENGTH = 0;

    // put in default memory
    for (i = 0; i < MAX_MENU_ITEMS; ++i) {
        for (j = 0; j < MENU_ITEM_LENGTH; ++j) {
            sg_MENU_ITEMS[i][j] = '\0';
        }
    }

    uiLoadPlaylistNames();
}

void uiLoadHeader()
{
    // clear the header area
    LcdFilledRectangle(HEADER_START_X, HEADER_START_Y, HEADER_END_X, HEADER_END_Y, NULL, lcd0.backgroundColor);
    // add data to header
    lcd0.textColor = COLOR_BLACK;

    // display title
    LcdTextOutXY(HEADER_START_X + PAD4, HEADER_START_Y + PAD4, "AMP3 Player");

    // display mode
    uiDisplayMode(sg_UI_STATE.mode);

    // display volume percentage
    uiDisplayVolume();

    // display battery percentage
    uiDisplayBattery();

    // draw border at bottom
    LcdFilledRectangle(HEADER_START_X, HEADER_END_Y, HEADER_END_X, HEADER_END_Y+1, NULL, COLOR_BLACK);
}

void uiDisplayMode(u_int16 mode) {
    sg_UI_STATE.mode = mode;
    // clear current section on screen
    LcdFilledRectangle(((HEADER_END_X - HEADER_START_X)/2) - 60, HEADER_START_Y + PAD4, (HEADER_END_X)-160, HEADER_END_Y-1,
                       NULL, lcd0.backgroundColor);
    // display current mode
    if (sg_UI_STATE.mode == NORMAL_MODE) {
        LcdTextOutXY(((HEADER_END_X - HEADER_START_X)/2) - 60, HEADER_START_Y + PAD4, "NORMAL ");
    }
    else if (sg_UI_STATE.mode == SHUFFLE_MODE) {
        LcdTextOutXY(((HEADER_END_X-HEADER_START_X)/2) - 60, HEADER_START_Y + PAD4, "SHUFFLE");
    }
    else if (sg_UI_STATE.mode == REPEAT_MODE) {
        LcdTextOutXY(((HEADER_END_X-HEADER_START_X)/2) - 60, HEADER_START_Y + PAD4, "REPEAT ");
    }
    else {
        LcdTextOutXY(((HEADER_END_X-HEADER_START_X)/2) - 60, HEADER_START_Y + PAD4, "UNKNOWN");
    }
}

void uiDisplayBattery() {
    char battery_display_str[12];
    if (sg_UI_STATE.menu_state != INIT_SCREEN) {
        sprintf(battery_display_str, "BATT: %3u", batteryLevel);

        // clear current section on screen
        LcdFilledRectangle((HEADER_END_X)-80, HEADER_START_Y + PAD4, HEADER_END_X, HEADER_END_Y-1,
                        NULL, lcd0.backgroundColor);

        // display battery percentage
        LcdTextOutXY((HEADER_END_X)-80, HEADER_START_Y + PAD4, battery_display_str);
    }
}

void uiDisplayVolume() {
    char volume_display_str[8];
    if (sg_UI_STATE.menu_state != INIT_SCREEN) {
        sprintf(volume_display_str, "VOL: %2u", 25 - (cycVolume / 6));

        // clear current section on screen
        LcdFilledRectangle((HEADER_END_X)-160, HEADER_START_Y + PAD4, HEADER_END_X-80, HEADER_END_Y-1,
                        NULL, lcd0.backgroundColor);

        // display battery percentage
        LcdTextOutXY((HEADER_END_X)-160, HEADER_START_Y + PAD4, volume_display_str);
    }
}

void uiDisplayMenuItems() {
    int offset, i, length, list_item;
    char temp_name[MENU_ITEM_LENGTH];
    LcdClearMainWindow();

    for (i = 0; i < MENU_LENGTH; ++i) {
        list_item = (MENU_LENGTH * (sg_LIST_INDEX / MENU_LENGTH)) + i;
        if (strlen(sg_MENU_ITEMS[list_item]) > 0 && list_item < sg_LIST_LENGTH) {
            // strncpy(temp_name, sg_MENU_ITEMS[(sg_LIST_INDEX / MENU_LENGTH) + i], MENU_ITEM_LENGTH);
            // format name
            for (length = 0; length < MENU_ITEM_LENGTH; ++length) {
                switch (sg_MENU_ITEMS[list_item][length]){
                case '_':
                case '-':
                    temp_name[length] = ' ';
                    break;
                case '.':
                    temp_name[length] = '\0';
                    break;
                default:
                    temp_name[length] = sg_MENU_ITEMS[list_item][length];
                    break;
                }
                // stop early if possible
                if (temp_name[length] == '\0') {
                    break;
                }
            }

            // highlight selected item
            if (i == (sg_LIST_INDEX % MENU_LENGTH)) {
                lcd0.backgroundColor = COLOR_NAVY;
                lcd0.textColor = COLOR_WHITE;
            }
            else {
                lcd0.backgroundColor = lcd0.defaultBackgroundColor;
                lcd0.textColor = lcd0.defaultTextColor;
            }
            // draw the item
            offset = (i * LIST_ITEM_HEIGHT) + (i * 4) + 4;
            LcdDrawBox(LIST_ITEM_START_X, MAIN_WINDOW_START_Y+4+offset, LIST_ITEM_END_X, MAIN_WINDOW_START_Y+4+LIST_ITEM_HEIGHT+offset,
                2, COLOR_BLACK, lcd0.backgroundColor);
            LcdTextOutXY(LIST_ITEM_START_X + 4, MAIN_WINDOW_START_Y+4+11+offset, temp_name);
        }
    }
    lcd0.backgroundColor = lcd0.defaultBackgroundColor;
    lcd0.textColor = lcd0.defaultTextColor;
}

void uiLoadPlaylistNames() {
    int line_number;
    char c;
    int i, j;
    FILE* usb_playlist_list_file = NULL;  // file that contains a list of playlists, separated by newlines
    // display now playing and all songs as options
    strcpy(sg_MENU_ITEMS[0], "Now Playing");
    strcpy(sg_MENU_ITEMS[1], "All Music");
    sg_MENU_ITEMS[0][12] = '\0';
    sg_MENU_ITEMS[1][10] = '\0';

    // open the file
    usb_playlist_list_file = fopen("D:Playlists/__playlists.txt", "r");
    if (usb_playlist_list_file == NULL) {
        // could not open file, either it does not exit or we are out of memory
        printf("Couldn't open file: D:Playlists/__playlists.txt\n");
        return;
    }

    for (i = 2; i < MAX_MENU_ITEMS; ++i) {
        if (fgets(sg_MENU_ITEMS[i], MENU_ITEM_LENGTH, usb_playlist_list_file) == NULL) {
            // copy failed somehow, either out of items or file permissions got bad.
            // either way, set the rest to a null string

            // keep track of where it ended so that more menus aren't displayed
            sg_LIST_LENGTH = i;
            for (i; i < MAX_MENU_ITEMS; ++i) {
                for (j = 0; j < MENU_ITEM_LENGTH; ++j) {
                    sg_MENU_ITEMS[i][j] = '\0';
                }
            }
            break;
        }
    }

    // check if there's more to read later:
    // sg_EOF_REACHED = (fgets(playlist_filename, 50, usb_playlist_list_file) == NULL);
    fclose(usb_playlist_list_file);
}

void uiCursorUp() {
    if (sg_LIST_INDEX != 0) {
        --sg_LIST_INDEX;
        uiDisplayMenuItems();
    }
    printf("UP: List index = %d    Menu Index = %d\n", sg_LIST_INDEX, sg_LIST_INDEX % 6);
}

void uiCursorDown() {
    // if (!sg_EOF_REACHED || sg_MENU_ITEM_INDEX != 5) {
    if (sg_LIST_INDEX < sg_LIST_LENGTH - 1) {
        ++sg_LIST_INDEX;
        uiDisplayMenuItems();
    }
    printf("DOWN: List index = %d    Menu Index = %d\n", sg_LIST_INDEX, sg_LIST_INDEX % 6);
}

char* uiCursorSelect() {
    return sg_MENU_ITEMS[sg_LIST_INDEX];
}

void uiLoadMainMenu()
{
    LcdClearMainWindow();
    lcd0.textColor = COLOR_BLACK;

    sg_UI_STATE.menu_state = MAIN_MENU;

    sg_LIST_INDEX = 0;
        
    uiLoadHeader();
    
    // display items
    uiDisplayMenuItems();
}

void uiLoadNowPlaying()
{
    int i;
    u_int16 info_start_x = MAIN_WINDOW_START_X+ALBUM_ART_MAX_WIDTH+2+PAD4;
    u_int16 info_start_y = MAIN_WINDOW_START_Y+12+PAD4;

    sg_UI_STATE.menu_state = NOW_PLAYING;
    lcd0.textColor = COLOR_BLACK;

    // clear the now playing section
    LcdClearMainWindow();

    // draw box for album art
    LcdDrawBox(MAIN_WINDOW_START_X+10, MAIN_WINDOW_START_Y+10,
               MAIN_WINDOW_START_X+ALBUM_ART_MAX_WIDTH+2, MAIN_WINDOW_START_Y+ALBUM_ART_MAX_HEIGHT+2,
               2, COLOR_BLACK, COLOR_WHITE); // COLOR_LIME



    LcdTextOutXY(info_start_x, info_start_y, sg_SONG_NAME);
    LcdTextOutXY(info_start_x, info_start_y+15, sg_ARTIST_NAME);
    LcdTextOutXY(info_start_x, info_start_y+30, sg_ALBUM_NAME);

    // draw the playback bar
     LcdDrawBox(PLAYBACK_START_X+58, PLAYBACK_START_Y+PAD4, MAIN_WINDOW_END_X-57, MAIN_WINDOW_END_Y-10,
               2, COLOR_BLACK, lcd0.backgroundColor);
}

void uiDisplaySongPlaybackBar(u_int16 elapsed_time, u_int16 song_length) {
    char buffer[8];
    u_int16 progress_bar_end = ((elapsed_time * 200) / song_length) + PLAYBACK_START_X+60;
    // song length in seconds
    // clear current section of screen
    LcdFilledRectangle(PLAYBACK_START_X, PLAYBACK_START_Y, MAIN_WINDOW_END_X, MAIN_WINDOW_END_Y,
                       NULL, lcd0.backgroundColor);

    // draw the empty box
    LcdDrawBox(PLAYBACK_START_X+58, PLAYBACK_START_Y+PAD4, MAIN_WINDOW_END_X-57, MAIN_WINDOW_END_Y-10,
               2, COLOR_BLACK, lcd0.backgroundColor);

    // draw the elapsed / remaining times
    sprintf(buffer, "%3u:%02u", elapsed_time / 60, elapsed_time % 60);
    LcdTextOutXY(PLAYBACK_START_X+2, PLAYBACK_START_Y+8, buffer);

    song_length -= elapsed_time;
    sprintf(buffer, "%3u:%02u", song_length / 60, song_length % 60);
    LcdTextOutXY(MAIN_WINDOW_END_X-57+PAD4, PLAYBACK_START_Y+8, buffer);

    // fill the empty box to the elapsed point
    LcdFilledRectangle(PLAYBACK_START_X+60, PLAYBACK_START_Y+PAD4+2,
                       progress_bar_end, MAIN_WINDOW_END_Y-12, 0, COLOR_NAVY);  // MAIN_WINDOW_END_X-59

    printf("X1: %d, Y1: %d\nX2: %d, Y2: %d\n", PLAYBACK_START_X+60, PLAYBACK_START_Y+PAD4+2,
                                               progress_bar_end, MAIN_WINDOW_END_Y-12);  // MAIN_WINDOW_END_X-59

}

void uiUpdatePercentComplete(u_int16 percent_complete) {
    u_int16 current_position, new_position;
    if (sg_UI_STATE.menu_state == NOW_PLAYING) {
        if (percent_complete > 0 || sg_PLAYBACK_PERCENT_COMPLETE > 0) {
            if (percent_complete > 100) {
                percent_complete = 100;
            }
            // song already in progress, update the display
            current_position = ((sg_PLAYBACK_PERCENT_COMPLETE * 2) + PLAYBACK_START_X+60);
            new_position = ((percent_complete * 2) + PLAYBACK_START_X+60);
            if (new_position > current_position) {
                // song advancing forwards, draw the bar
                LcdFilledRectangle(current_position, PLAYBACK_START_Y+PAD4+2,
                                   new_position, MAIN_WINDOW_END_Y-12, 0, COLOR_NAVY);
            }
            else if (new_position < current_position) {
                // song went backwards, clear some of the bar
                LcdFilledRectangle(new_position, PLAYBACK_START_Y+PAD4+2,
                                   current_position, MAIN_WINDOW_END_Y-12, 0, lcd0.backgroundColor);
            }
        }
    }
    sg_PLAYBACK_PERCENT_COMPLETE = percent_complete;
}

void uiUpdatePlaybackTime(u_int16 new_time) {
    char buffer[8];
    sg_PLAYBACK_TIME = new_time;
    LcdFilledRectangle(PLAYBACK_START_X+2, PLAYBACK_START_Y+8, PLAYBACK_START_X+55,
                       MAIN_WINDOW_END_Y, NULL, lcd0.backgroundColor);
    sprintf(buffer, "%3u:%02u", new_time / 60, new_time % 60);
    LcdTextOutXY(PLAYBACK_START_X+2, PLAYBACK_START_Y+8, buffer);
}

void uiShowPlayPause(u_int16 isPaused) {
	if (isPaused) {
		// show paused
		LcdFilledRectangle(((HEADER_END_X-HEADER_START_X)/2) - 40, PLAYBACK_START_Y-16,
						   ((HEADER_END_X-HEADER_START_X)/2) + 40, PLAYBACK_START_Y, NULL, lcd0.backgroundColor);

		LcdTextOutXY(((HEADER_END_X-HEADER_START_X)/2) - 40, PLAYBACK_START_Y - 16, "PAUSED");
	}
	else {
		LcdFilledRectangle(((HEADER_END_X-HEADER_START_X)/2) - 40, PLAYBACK_START_Y-16,
						   ((HEADER_END_X-HEADER_START_X)/2) + 40, PLAYBACK_START_Y, NULL, lcd0.backgroundColor);

		LcdTextOutXY(((HEADER_END_X-HEADER_START_X)/2) - 40, PLAYBACK_START_Y - 16, "PLAYING");
	}
}

int getUIState(void) {
    if (sg_UI_STATE.menu_state == MAIN_MENU || sg_UI_STATE.menu_state == SONG_MENU) {
        return 1;
    }
    return 0;
}

// {
//     LcdClearScreen();
//     // Change font to red so it stands out
//     lcd0.textColor = COLOR_RED;
//     LcdTextOutXY(10, 20, "A critical error has occured!");
//     LcdTextOutXY(10, 40, "Please try restarting the system");
//     LcdTextOutXY(10, 60, "If the error persists, contact UML");
//     lcd0.textColor = COLOR_WHITE;
// }
