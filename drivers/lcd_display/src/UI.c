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

#include "UI.h"

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
#define SONG_MENU1    2
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
static u_int16 sg_BATTERY_PERCENTAGE;

// menu items
#define MENU_LENGTH 6
#define LIST_ITEM_NOW_PLAYING "Now Playing"
#define LIST_ITEM_ALL_MUSIC "All Music"
#define MENU_ITEM_LENGTH 40
static char* sg_MENU_ITEMS[MENU_LENGTH];
static u_int16 sg_MENU_ITEM_INDEX;
static u_int16 sg_LIST_INDEX;
static u_int16 sg_EOF_REACHED;


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

void resetSong() {
    strcpy(sg_SONG_NAME, "Unknown Song");
    strcpy(sg_ALBUM_NAME, "Unknown Album");
    strcpy(sg_ARTIST_NAME, "Unknown Artist");
}

void UIMetadataDecodeCallBack(s_int16 index, u_int16 message, u_int32 value) {
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


ioresult UI_init(void) {
    int i;
    LcdInit(0);

    LcdDrawBox((lcd0.width/2)-65, (lcd0.height/2)-25, (lcd0.width/2)+65,
               (lcd0.height/2)+25, 2, COLOR_BLACK, lcd0.backgroundColor);
    lcd0.textColor = COLOR_BLACK;
    LcdTextOutXY((lcd0.width/2)-60, (lcd0.height/2), "AMP3 Booting...");

    sg_UI_STATE.menu_state = INIT_SCREEN;
    sg_UI_STATE.mode = NORMAL_MODE;
    sg_UI_STATE.paused = TRUE;

    resetSong();
    // todo: replace with acutal battery info
    sg_BATTERY_PERCENTAGE = 100;

    sg_PLAYBACK_PERCENT_COMPLETE = 0;
    sg_PLAYBACK_TIME = 0;
    sg_LIST_INDEX = 0;
    sg_MENU_ITEM_INDEX = 0;

    // allocate 40 characters for each string
    for (i = 0; i < MENU_LENGTH; ++i) {
        sg_MENU_ITEMS[i] = (char*) malloc(MENU_ITEM_LENGTH * sizeof(char));
    }
    strncpy(sg_MENU_ITEMS[0], LIST_ITEM_NOW_PLAYING, MENU_ITEM_LENGTH);
    for (i = 1; i < MENU_LENGTH; ++i) {
        strncpy(sg_MENU_ITEMS[i], "\0\0\0\0", MENU_ITEM_LENGTH);
    }
    sg_EOF_REACHED = 0;
}

void loadHeader()
{
    // clear the header area
    LcdFilledRectangle(HEADER_START_X, HEADER_START_Y, HEADER_END_X, HEADER_END_Y, NULL, lcd0.backgroundColor);
    // add data to header
    lcd0.textColor = COLOR_BLACK;

    // display current mode
    if (sg_UI_STATE.mode == NORMAL_MODE) {
        LcdTextOutXY(HEADER_START_X + PAD4, HEADER_START_Y + PAD4, "NORMAL ");
    }
    else if (sg_UI_STATE.mode == SHUFFLE_MODE) {
        LcdTextOutXY(HEADER_START_X + PAD4, HEADER_START_Y + PAD4, "SHUFFLE");
    }
    else if (sg_UI_STATE.mode == REPEAT_MODE) {
        LcdTextOutXY(HEADER_START_X + PAD4, HEADER_START_Y + PAD4, "REPEAT ");
    }
    else {
        LcdTextOutXY(HEADER_START_X + PAD4, HEADER_START_Y + PAD4, "UNKNOWN");
    }

    // display title
    LcdTextOutXY(((HEADER_END_X-HEADER_START_X)/2) - 40, HEADER_START_Y + PAD4, "AMP3 Player");

    // display battery percentage
    displayBatteryPercentage(sg_BATTERY_PERCENTAGE);

    // draw border at bottom
    LcdFilledRectangle(HEADER_START_X, HEADER_END_Y, HEADER_END_X, HEADER_END_Y+1, NULL, COLOR_BLACK);
}

void displayBatteryPercentage(u_int16 battery_level) {
    char battery_display_str[12];
    sprintf(battery_display_str, "BATT: %3u%%", battery_level);

    // clear current section on screen
    LcdFilledRectangle((HEADER_END_X)-80, HEADER_START_Y + PAD4, HEADER_END_X, HEADER_END_Y-1,
                       NULL, lcd0.backgroundColor);

    // display battery percentage
    LcdTextOutXY((HEADER_END_X)-80, HEADER_START_Y + PAD4, battery_display_str);
}


void display_menu_items() {
    int offset, i, length;
    char temp_name[50];
    LcdClearMainWindow();

    for (i = 0; i < MENU_LENGTH; ++i) {
        if (sg_MENU_ITEMS[i] != NULL && sg_MENU_ITEMS[i][0] != '\0') {
            strncpy(temp_name, sg_MENU_ITEMS[i], 50);
            // format name
            for (length = 0; length < MENU_ITEM_LENGTH; ++length) {
                switch (temp_name[length]){
                case '\0': break;
                case '_':
                case '-':
                    temp_name[length] = ' ';
                    break;
                case '.':
                    temp_name[length] = '\0';
                    break;
                }
                if (temp_name[length] == '\0') {
                    break;
                }
            }

            if (length == MENU_ITEM_LENGTH) {
                temp_name[length-1] = '\0';
            }

            // highlight selected item
            lcd0.backgroundColor = (i == sg_MENU_ITEM_INDEX) ? COLOR_NAVY : lcd0.defaultBackgroundColor;
            lcd0.textColor = (i == sg_MENU_ITEM_INDEX) ? COLOR_WHITE : lcd0.defaultTextColor;
            offset = (i * LIST_ITEM_HEIGHT) + (i * 4) + 4;
            LcdDrawBox(LIST_ITEM_START_X, MAIN_WINDOW_START_Y+4+offset, LIST_ITEM_END_X, MAIN_WINDOW_START_Y+4+LIST_ITEM_HEIGHT+offset, 
                2, COLOR_BLACK, lcd0.backgroundColor);
            LcdTextOutXY(LIST_ITEM_START_X + 4, MAIN_WINDOW_START_Y+4+11+offset, temp_name);
        }
    }
    lcd0.backgroundColor = lcd0.defaultBackgroundColor;
    lcd0.textColor = lcd0.defaultTextColor;
}

void load_playlist_names() {
    char playlist_filename[50];
    int line_number;
    char c;
    int i = 0;
    FILE* usb_playlist_list_file = NULL;  // file that contains a list of playlists, separated by newlines
    printf("LOAD: List index = %d    Menu Index = %d\n", sg_LIST_INDEX, sg_MENU_ITEM_INDEX);
    if (sg_LIST_INDEX < 6) {
        // display now playing and all songs as options
        sg_MENU_ITEMS[0] = LIST_ITEM_NOW_PLAYING;
        sg_MENU_ITEMS[1] = LIST_ITEM_ALL_MUSIC;
        i = 2;
    }

    // open the file
    usb_playlist_list_file = fopen("D:Playlists/__playlists.txt", "r");
    if (usb_playlist_list_file == NULL) {
        // could not open file, either it does not exit or we are out of memory
        printf("Couldn't open file: D:Playlists/__playlists.txt");
        return;
    }

    printf("Opened file at %p\n", usb_playlist_list_file);
    printf("Size: %d\n", usb_playlist_list_file->fileSize);
    usb_playlist_list_file->Identify(usb_playlist_list_file, playlist_filename, 50);
    printf("Decorative info: %s\n", playlist_filename);
    printf("Location: %d\n", usb_playlist_list_file->pos);
    fseek(usb_playlist_list_file, 0, SEEK_SET);
    printf("Location: %d\n", usb_playlist_list_file->pos);
    // navigate to correct line in file
    line_number = ((u_int16)(sg_LIST_INDEX / 6)) * 6 - 2;

    printf("Line Number: %d\n", line_number);
    // while (line_number > 0) {
    //     // printf("Location: %d\n", usb_playlist_list_file->pos);
    //     // printf("read: %p\n", fgets(playlist_filename, 50, usb_playlist_list_file));
    //     // printf("Location: %d\n", usb_playlist_list_file->pos);

    //     printf("Skipping line: <%s>\n", playlist_filename);
    //     --line_number;
    // }
    c = fgetc(usb_playlist_list_file);
    while (c != EOF) {
        printf("%c", c);
        c = fgetc(usb_playlist_list_file);
    }
    fseek(usb_playlist_list_file, 0, SEEK_SET);

    while (i < 6) {
        // read line from file
        if (fgets(playlist_filename, 50, usb_playlist_list_file) != NULL) {
            // add playlist filename to menu items
            strncpy(sg_MENU_ITEMS[i], playlist_filename, MENU_ITEM_LENGTH);
        }
        else {
            printf("Read an empty line at item %d!\n", i);
            // if (sg_MENU_ITEM_INDEX == 0 && sg_LIST_INDEX >= 6) {
            //     sg_LIST_INDEX -= 1;
            //     sg_MENU_ITEM_INDEX = sg_LIST_INDEX % 6;
            //     fclose(usb_playlist_list_file);
            //     load_playlist_names();
            //     return;
            // }
            // else {
            strncpy(sg_MENU_ITEMS[i], "\0\0\0\0", MENU_ITEM_LENGTH);
            // }
        }
        i++;
    }
    // check if there's more to read later:
    // sg_EOF_REACHED = (fgets(playlist_filename, 50, usb_playlist_list_file) == NULL);
    printf("Closing file\n");
    fclose(usb_playlist_list_file);
    printf("File closed\n\n\n\n\n\n");
}

void uiCursorUp() {
    if (sg_LIST_INDEX != 0) {
        --sg_LIST_INDEX;
        sg_MENU_ITEM_INDEX = sg_LIST_INDEX % 6;
        load_playlist_names();
        display_menu_items();
    }
    printf("UP: List index = %d    Menu Index = %d\n", sg_LIST_INDEX, sg_MENU_ITEM_INDEX);
}

void uiCursorDown() {
    // if (!sg_EOF_REACHED || sg_MENU_ITEM_INDEX != 5) {
    ++sg_LIST_INDEX;
    sg_MENU_ITEM_INDEX = sg_LIST_INDEX % 6;
    load_playlist_names();
    display_menu_items();
    // }
    printf("DOWN: List index = %d    Menu Index = %d\n", sg_LIST_INDEX, sg_MENU_ITEM_INDEX);
}

void loadMainMenu()
{
    // monitorVoltage();
    LcdClearMainWindow();
    lcd0.textColor = COLOR_BLACK;

    sg_UI_STATE.menu_state = MAIN_MENU;
    
    // reload items
    // sg_MENU_ITEMS[0] = "Now Playing";

    sg_LIST_INDEX = 0;
    sg_MENU_ITEM_INDEX = 0;

    // load playlist names
    load_playlist_names();

    // display items
    display_menu_items();
}

void loadNowPlaying()
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

void displaySongPlaybackBar(u_int16 elapsed_time, u_int16 song_length) {
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

void updatePercentComplete(u_int16 percent_complete) {
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

void updatePlaybackTime(u_int16 new_time) {
    char buffer[8];
    sg_PLAYBACK_TIME = new_time;
    LcdFilledRectangle(PLAYBACK_START_X+2, PLAYBACK_START_Y+8, PLAYBACK_START_X+55,
                       MAIN_WINDOW_END_Y, NULL, lcd0.backgroundColor);
    sprintf(buffer, "%3u:%02u", new_time / 60, new_time % 60);
    LcdTextOutXY(PLAYBACK_START_X+2, PLAYBACK_START_Y+8, buffer);
}

void UIShowPlayPause(u_int16 isPaused) {
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


// void hideSongPlaybackBar() {
//     LcdFilledRectangle(PLAYBACK_START_X, PLAYBACK_START_Y, MAIN_WINDOW_END_X, MAIN_WINDOW_END_Y,
//                        NULL, lcd0.backgroundColor);
// }























void loadCriticalErrorMenu()
{
    LcdClearScreen();
    // Change font to red so it stands out
    lcd0.textColor = COLOR_RED;
    LcdTextOutXY(10, 20, "A critical error has occured!");
    LcdTextOutXY(10, 40, "Please try restarting the system");
    LcdTextOutXY(10, 60, "If the error persists, contact UML");
    lcd0.textColor = COLOR_WHITE;
}















































































// Used across multiple menus by specifying the title being drawn
// void drawCommonMenuItems(char *title, u_int16 menu)
// {
// 	LcdInit(1);
// 	monitorVoltage();
// 	LcdTextOutXY(1,1, title);
// 	// Change font to red so it stands out
// 	lcd0.textColor = RED;
// 	//Draw common buttons and title to screen
// 	LcdTextOutXY(200,20, "BACK");

// 	// The info menu doesn't have options for navigation, so lets hide them
// 	if (menu != INFO_MENU)
// 	{
// 		LcdTextOutXY(10,20, "UP");
// 		LcdTextOutXY(10,310, "DOWN");
// 		LcdTextOutXY(190,310, "SELECT");
// 		// Done with red, change font to yellow for the box
// 		lcd0.textColor = YELLOW;
// 		// Draw the box around the first entry
// 		LcdTextOutXY(10, 30,  "--------------------------------");
// 		LcdTextOutXY(7, 40,  "|");
// 		LcdTextOutXY(230, 40,  "|");
// 		LcdTextOutXY(10, 50,  "--------------------------------");
// 		// Change the color back to white for any other function that draws
// 		lcd0.textColor = WHITE;
// 		pageNum = 0;
// 		arrowSelection = 0;
// 	}

// 	currentMenu = menu;
// }

// void drawMenuListOptions()
// {
// 	u_int16 i;
// 	char *entry;

// 	// Clear all options on the screen
// 	for (i = 0; i < 9; i++)
// 	{
// 	    LcdTextOutXY(20, 40 + (i * 30), "                              ");
// 	}

// 	// point to start of file
// 	fseek(filehandle, 0, SEEK_SET);

// 	// Skip lines that we're not interested in right now
// 	for (i = 0; i < pageNum * 9; i++)
// 	{
// 	    fgets(buffer, sizeof(buffer), filehandle);
// 	    //printf("skipped buffer: %s", buffer);
// 	}

// 	topOfList = ftell(filehandle);
// 	//fgets(buffer, sizeof(buffer), filehandle);
// 	//printf("Top of List buffer : %s", buffer);
// 	//fseek(filehandle, topOfList, SEEK_SET);

// 	// Grab the first 10 lines of data. 9 will be displayed, and the 10th will
// 	// be used to decide if reaching the end of the list should allow a new
// 	// list to be generated or not
// 	for (i = 0; i <= 9; i++)
// 	{
// 	    // First 9 to draw
// 	    if (i < 9)
// 	    {
// 	        // If the line exists, draw the line to the screen
// 	        if (fgets(buffer, sizeof(buffer), filehandle))
// 	        {
// 	        	 // strtok reads a buffer until a certain character is found as a delimitter
// 	        	 // in our case, "|" will be our delimitter
// 	        	 // read the first entry into the table, which will be the file name
// 	        	 // FILENAME | PATH | INDEX
// 	            entry = strtok(buffer, "|");
// 	            LcdTextOutXY(20, 40 + (i * 30), entry);
// 	        }
// 	        // The line didn't exist, mark the end of file line to know our
// 	        // box movement boundaries. No reason checking additional lines,
// 	        // so break from the loop now
// 	        else
// 	        {
// 	            endOfFile = i-1;
// 	            break;
// 	        }
// 	    }
// 	    // Last check to determine if there is another list after
// 	    else
// 	    {
// 	        // Not drawing to screen, just checking if it exists. If it doesn't,
// 	        // set end of file appropriately
// 	        if (!fgets(buffer, sizeof(buffer), filehandle))
// 	        {
// 	            endOfFile = i-1;
// 	        }
// 	        // There is another menu, so set end of file to value that will not stop
// 	        // us from navigating to it
// 	        else
// 	        {
// 	        	endOfFile = -1;
// 			 }
// 	    }
// 	}
// 	// move the pointer back to the top of the list for easier callback on selection
// 	fseek(filehandle, topOfList, SEEK_SET);
// 	fgets(buffer, sizeof(buffer), filehandle);
// 	printf("Top of List buffer : %s", buffer);
// 	fseek(filehandle, topOfList, SEEK_SET);
// }

// void loadArtistsMenu()
// {
// 	drawCommonMenuItems("ARTISTS MENU", ARTISTS_MENU);

// 	if (filehandle)
// 	{
// 		fclose(filehandle);
// 	}

// 	filehandle = fopen("S:LOOKUP/songs.txt", "rb");

// 	// If file was succesfully opened to be read
// 	if (filehandle)
// 	{
// 		drawMenuListOptions();
// 	}
// 	else
// 	{
// 		printf("failed to open file \n");
// 	}
// }

// void loadPlaylistsMenu()
// {
// 	drawCommonMenuItems("PLAYLIST MENU", PLAYLISTS_MENU);

// 	if (filehandle)
// 	{
// 		fclose(filehandle);
// 	}

// 	filehandle = fopen("S:LOOKUP/songs.txt", "rb");

// 	// If file was succesfully opened to be read
// 	if (filehandle)
// 	{
// 		drawMenuListOptions();
// 	}
// 	else
// 	{
// 		printf("failed to open file \n");
// 	}
// }

// void loadSongsMenu()
// {
// 	drawCommonMenuItems("SONGS MENU", SONGS_MENU);

// 	if (filehandle)
// 	{
// 		fclose(filehandle);
// 	}
// 	filehandle = fopen("S:LOOKUP/songs.txt", "rb");

// 	// If file was succesfully opened to be read
// 	if (filehandle)
// 	{
// 		drawMenuListOptions();
// 	}
// 	else
// 	{
// 		printf("failed to open file \n");
// 	}
// }

// void loadScanningMenu()
// {
// 	LcdInit(1);
// 	// Change font to red so it stands out
// 	lcd0.textColor = RED;
// 	LcdTextOutXY(10, 100, "Scanning disk for songs...");
// 	lcd0.textColor = WHITE;
// 	currentMenu = LOAD_MENU;
// }

// void loadInfoMenu()
// {
// 	drawCommonMenuItems("INFO MENU", INFO_MENU);

// 	LcdTextOutXY(10, 100, (char *) metadata->title);
// 	LcdTextOutXY(10, 130, (char *) metadata->artist);
// 	LcdTextOutXY(10, 160, (char *) metadata->album);

// 	strcpy(currentTitle, (char *) metadata->title);
// }

// void moveBoxSelection(u_int16 direction)
// {
// 	u_int16 i;

// 	// Clear the current box from the screen by just drawing over it with blank spaces
// 	LcdTextOutXY(10, 30 + (arrowSelection * 30),  "                                ");
// 	LcdTextOutXY(7, 40 + (arrowSelection * 30),  " ");
// 	LcdTextOutXY(230, 40 + (arrowSelection * 30),  " ");
// 	LcdTextOutXY(10, 50 + (arrowSelection * 30),  "                                ");

// 	// User wants to move the selection up
// 	if (direction == UP)
// 	{
// 		// If the user is already at the top of the list, determine if there is a previous
// 		// list to navigate to or not
// 		if (arrowSelection == 0)
// 		{
// 			// list page number is not 0, so there is a previous list to navigate to
// 			if (!pageNum == 0)
// 			{
// 				// decrement list since we're navigating back
// 				pageNum--;
// 				// move the box to the last option on the previous list
// 				arrowSelection = 8;
// 				// redraw the options to the screen
// 				drawMenuListOptions();
// 			}
// 		}
// 		// We're not on the first option, so we can just decrement the counter by one and be done
// 		else
// 		{
// 			arrowSelection--;
// 		}
// 	}
// 	// User wants to move the selection down
// 	else
// 	{
// 		// If end of file was set, then we don't want to allow the user to move beyond the last
// 		// populated option
// 		if (arrowSelection != endOfFile)
// 		{
// 			// Moving down on last option requires a redraw of the screen options
// 			if (arrowSelection == 8)
// 			{
// 				// increment page number since we're moving to a new page
// 				pageNum++;
// 				// reset the box to the top of the list
// 				arrowSelection = 0;

// 				drawMenuListOptions();
// 			}
// 			else
// 			{
// 				arrowSelection++;
// 			}
// 		}
// 	}

// 	// Draw the box at the new determined position
// 	lcd0.textColor = YELLOW;
// 	LcdTextOutXY(10, 30 + (arrowSelection * 30),  "--------------------------------");
// 	LcdTextOutXY(7, 40 + (arrowSelection * 30),  "|");
// 	LcdTextOutXY(230, 40 + (arrowSelection * 30),  "|");
// 	LcdTextOutXY(10, 50 + (arrowSelection * 30),  "--------------------------------");
// 	lcd0.textColor = WHITE;
// }

// char *retrieveSongLocation(u_int16 *index, u_int16 offset)
// {
// 	u_int16 i;
// 	char *charIndex;

// 	// Skip lines that we're not interested in right now
// 	for (i = 0; i < arrowSelection + offset; i++)
// 	{
// 		// We want to make sure that the line that is being read actually exists
// 		// This should always be the case when a user selects an entry, but when
// 		// the next song is chosen automatically when a song ends, that might not be the case
// 		// so, check if it exists and if it doesn't, we need to point back to the top of the
// 		// list and then start reading lines
// 	    if (!fgets(buffer, sizeof(buffer), filehandle))
// 	    {
// 	    	fseek(filehandle, 0, SEEK_SET);
// 	    	fgets(buffer, sizeof(buffer), filehandle);
// 		 }
// 	    //printf("Discarded buffers : %s \n", buffer);
// 	}

// 	// Now fill the buffer with the line of interest
// 	fgets(buffer, sizeof(buffer), filehandle);
// 	//printf("Used buffer : %s \n", buffer);

// 	// We're done grabbing the line of interest, move pointer back to top of list
// 	// for most convenience on future selections
// 	fseek(filehandle, topOfList, SEEK_SET);

// 	// strtok will split the buffer string into the fields that we are interested in
// 	// The first field is the title, which we don't need for locating the song on the disk
// 	// so we're going to have it skipped by calling the function and not using it
// 	strtok(buffer, "|");
// 	//printf("Title : %s \n", buffer);

// 	// Now the next column is the index number. It'll be returned as a char, so we need to subtract
// 	// '0' from it to convert it to the appropriate integer
// 	charIndex = strtok(NULL, "|");
// 	//printf("Index char : %s \n", charIndex);
// 	*index = atoi(charIndex);
// 	//printf("Index int : %i \n", index);

// 	// If a song is selected and something is currently playing, we want to set newSongSelected as
// 	// an interrupt. Otherwise, we want to start running the system
// 	if (running)
// 	{
// 		newSongSelected = 1;
// 	}
// 	else
// 	{
// 		running = 1;
// 	}

// 	// The last field we need is the path. This will just be returned and used by the calling function
// 	// directly
// 	return strtok(NULL, "|");
// }

// void loadNoDriveMenu()
// {
// 	LcdInit(1);
// 	// Change font to red so it stands out
// 	lcd0.textColor = RED;
// 	LcdTextOutXY(1, 70, "No Drive Detected!");
// 	LcdTextOutXY(1, 90, "Please try restarting the system");
// 	LcdTextOutXY(1, 110, "Make sure the drive is plugged in");
// 	LcdTextOutXY(1, 130, "and is formatted as FAT");
// 	LcdTextOutXY(1, 150, "If the error persists, contact UML");
// 	lcd0.textColor = WHITE;
// 	currentMenu = ERR_MENU;
// }

// u_int16 navigateMenus()
// {
// 	switch (currentMenu)
// 	{
// 		case MAIN_MENU :
// 			if (GpioReadPin(BUTTON10))
// 			{
// 				loadPlaylistsMenu();
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON10)) {}
// 				printf("BUTTON10 \n");
// 				return 1;
// 			}
// 			else if (GpioReadPin(BUTTON11))
// 			{
// 				loadSongsMenu();
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON11)) {}
// 				printf("BUTTON11 \n");
// 				return 1;
// 			}
// 			else if (GpioReadPin(BUTTON12))
// 			{
// 				loadArtistsMenu();
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON12)) {}
// 				printf("BUTTON12 \n");
// 				return 1;
// 			}
// 			else if (GpioReadPin(BUTTON13))
// 			{
// 				loadInfoMenu();
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON13)) {}
// 				printf("BUTTON13 \n");
// 				return 1;
// 			}
// 			break;
// 		case INFO_MENU :
// 			if (lastTimeSeconds != audioDecoder->cs.playTimeSeconds)
// 			{
// 				lastTimeSeconds = (int) audioDecoder->cs.playTimeSeconds;
// 				if (lastTimeSeconds < 99*60)
// 				{

// 				}
// 			}
// 			if (GpioReadPin(BUTTON12))
// 			{
// 				loadMainMenu();
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON12)) {}
// 				printf("BUTTON12 \n");
// 				return 1;
// 			}
// 			break;
// 		case ARTISTS_MENU :
// 			if (GpioReadPin(BUTTON10))
// 			{
// 				moveBoxSelection(DOWN);
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON10)) {}
// 				printf("BUTTON10 \n");
// 				return 1;
// 			}

// 			if (GpioReadPin(BUTTON11))
// 			{
// 				moveBoxSelection(UP);
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON11)) {}
// 				printf("BUTTON11 \n");
// 				return 1;
// 			}

// 			if (GpioReadPin(BUTTON12))
// 			{
// 				loadMainMenu();
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON12)) {}
// 				printf("BUTTON12 \n");
// 				return 1;
// 			}
// 			break;

// 		case SONGS_MENU :
// 			if (GpioReadPin(BUTTON10))
// 			{
// 				moveBoxSelection(DOWN);
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON10)) {}
// 				printf("BUTTON10 \n");
// 				return 1;
// 			}

// 			else if (GpioReadPin(BUTTON11))
// 			{
// 				moveBoxSelection(UP);
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON11)) {}
// 				printf("BUTTON11 \n");
// 				return 1;
// 			}

// 			else if (GpioReadPin(BUTTON12))
// 			{
// 				loadMainMenu();
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON12)) {}
// 				printf("BUTTON12 \n");
// 				return 1;
// 			}

// 			else if (GpioReadPin(BUTTON13))
// 			{
// 				path = retrieveSongLocation(&index, 0);
// 				printf("%s - %i \n", path, index);
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON13)) {}
// 				printf("BUTTON13 \n");
// 				return 1;
// 			}
// 			break;

// 		case PLAYLISTS_MENU :
// 			if (GpioReadPin(BUTTON12))
// 			{
// 				loadMainMenu();
// 				anyButtonPressed = 1;
// 				idleMode = 0;
// 				while (GpioReadPin(BUTTON12)) {}
// 				printf("BUTTON12 \n");
// 				return 1;
// 			}
// 			break;

// 		default :
// 			break;
// 	}

// 	return 0;
// }
