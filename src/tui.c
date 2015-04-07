
#include <menu.h>	/* Gives you menuing capabilities */
#include <stdlib.h>	/* Needed for calloc() */
#include <string.h>	/* Needed for strlen() and friends */

#include "tui.h"

#define WHITEONRED 1
#define WHITEONBLUE 2
#define WHITEONBLACK 3
#define BLACKONWHITE 4
#define REDONWHITE 5


void wCenterTitle(WINDOW *pwin, const char * title)
	{
	int x, maxy, maxx, stringsize;
	getmaxyx(pwin, maxy, maxx);
	stringsize = 4 + strlen(title);
	x = (maxx - stringsize)/2;
	mvwaddch(pwin, 0, x, ACS_RTEE);
	waddch(pwin, ' ');
	waddstr(pwin, title);
	waddch(pwin, ' ');
	waddch(pwin, ACS_LTEE);
	}

void wclrscr(WINDOW * pwin)
	{
	int y, x, maxy, maxx;
	getmaxyx(pwin, maxy, maxx);
	for(y=0; y < maxy; y++)
		for(x=0; x < maxx; x++)
			mvwaddch(pwin, y, x, ' ');
	}

bool initColors()
	{
	if(has_colors())
		{
		start_color();
		init_pair(WHITEONRED, COLOR_WHITE, COLOR_RED);
		init_pair(WHITEONBLUE, COLOR_WHITE, COLOR_BLUE);
		init_pair(REDONWHITE, COLOR_RED, COLOR_WHITE);
		return(true);
		}
	else
		return(false);
	}


int runMenu(
		WINDOW *wParent,
		int height,
		int width,
		int y,
		int x,
		char *choices[]
		)
	{
	int c;			/* key pressed */	
	ITEM **my_items;	/* list of items on this menu */
	MENU *my_menu;		/* the menu structure */

	WINDOW *wUI;		/* window on which the user
					interacts with the menu */
	WINDOW *wBorder;	/* window containing the wUI window
					and the border and title */

        int n_choices;		/* number of items on menu */
        int ssChoice;		/* subscript to run around the choices array */
	int my_choice = -1;	/* the zero based numeric user choice */

	/* CALCULATE NUMBER OF MENU CHOICES */
	for(n_choices=0; choices[n_choices]; n_choices++);

	/* ALLOCATE ITEM ARRAY AND INDIVIDUAL ITEMS */
        my_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));
        for(ssChoice = 0; ssChoice < n_choices; ++ssChoice)
                my_items[ssChoice] = new_item(choices[ssChoice], NULL);
	my_items[n_choices] = (ITEM *)NULL;

	/* CREATE THE MENU STRUCTURE */
	my_menu = new_menu((ITEM **)my_items);

	/* PUT > TO THE LEFT OF HIGHLIGHTED ITEM */
	set_menu_mark(my_menu, "> ");

	/* SET UP WINDOW FOR MENU'S BORDER */
	wBorder = newwin(height, width, y, x);
	wattrset(wBorder, COLOR_PAIR(WHITEONRED) | WA_BOLD);
	wclrscr(wBorder); 
	box(wBorder, 0, 0);
	wCenterTitle(wBorder, "Choose one");

	/* SET UP WINDOW FOR THE MENU'S USER INTERFACE */
	wUI = derwin(wBorder, height-2, width-2, 2, 2);

	/* ASSOCIATE THESE WINDOWS WITH THE MENU */
	set_menu_win(my_menu, wBorder);
	set_menu_sub(my_menu, wUI);
	set_menu_format(my_menu, 12, 1);

	/* MATCH MENU'S COLORS TO THAT OF ITS WINDOWS */
	set_menu_fore(my_menu, COLOR_PAIR(REDONWHITE));
	set_menu_back(my_menu, COLOR_PAIR(WHITEONRED) | WA_BOLD);

	/* SET UP AN ENVIRONMENT CONDUCIVE TO MENUING */
	keypad(wUI, TRUE);	/* enable detection of function keys */
	noecho();		/* user keystrokes don't echo */
	curs_set(0);		/* make cursor invisible */

	/* DISPLAY THE MENU */
	post_menu(my_menu);

	/* REFRESH THE BORDER WINDOW PRIOR TO ACCEPTING USER INTERACTION */
	touchwin(wBorder);
	wrefresh(wBorder);  

	/* HANDLE USER KEYSTROKES */
	while(my_choice == -1)
		{
		touchwin(wUI);	/* refresh prior to getch() */
		wrefresh(wUI); 	/* refresh prior to getch() */
		c = getch();
		switch(c)
			{
			case KEY_DOWN:
				menu_driver(my_menu, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				menu_driver(my_menu, REQ_UP_ITEM);
				break;
			case 10:	/* Enter */
				my_choice = item_index(current_item(my_menu));

				/* RESET CURSOR IN CASE MORE SELECTION IS NECESSARY */
				pos_menu_cursor(my_menu);
				break;
			}
		}	

	/* FREE ALL ALLOCATED MENU AND ITEM RESOURCES */
	unpost_menu(my_menu);
        for(ssChoice = 0; ssChoice < n_choices; ++ssChoice)
                free_item(my_items[ssChoice]);
	free_menu(my_menu);

	/* DESTROY MENU WINDOW AND BORDER WINDOWS */
	delwin(wUI);
	delwin(wBorder);

	/* UNDO MENU SPECIFIC ENVIRONMENT */
	curs_set(1);			/* make cursor visible again */
	
	/* REPAINT THE CALLING SCREEN IN PREPARATION FOR RETURN */
	touchwin(wParent);
	wrefresh(wParent);

	/* RETURN THE ZERO BASED NUMERIC USER CHOICE */
	return(my_choice);
	}
