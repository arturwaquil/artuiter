#include "../include/ClientUI.hpp"

#include <iostream>
#include <list>
#include <string>

#include <ncurses.h>

#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

bool ncurses_started = false;

ClientUI::ClientUI()
{
    _start_curses();
}

ClientUI::~ClientUI()
{
    _end_curses();
}

void ClientUI::init()
{
    line_width = 70;
    command_pos = std::make_pair(20,5);
    feed_pos = std::make_pair(15,5);

    basic_screen();

}

std::string ClientUI::read_command()
{
    _move(command_pos);
    
    char ch;
    std::string input;

    timeout(100);   // Time (ms) that getch() waits before returning ERR
    
    // Read input from user until break condition (newline or ctrl+D)
    while (true)
    {
        ch = getch();

        // Because timeout is positive, getch() is non-blocking, and returns
        // ERR if no input is read. We want to continue waiting for input.
        if (ch == ERR) continue;

        // Break on newline or ctrl+D
        // TODO: ctrl+D must halt program
        if (ch == '\n' || ch == CTRL('d'))
        {
            if (input.empty()) continue;
            else break;
        }

        // Handle backspace
        else if (ch == 127)
        {
            if (!input.empty())
            {
                input.pop_back();
                addch('\b');
                addch(' ');
                int y, x;
                getyx(stdscr, y, x);
                move(y, x-1);
            }
        }

        // Add char to input string
        else
        {
            input.push_back(ch);
            addch(ch);
        }
    }

    // Clear command-input part of the screen
    print(command_pos, "");

    return input;
}

int ClientUI::update_feed(std::string update)
{
    // Insert update in list, ensure max size of 10
    last_ten_updates.push_front(update);
    if (last_ten_updates.size() > 10) last_ten_updates.pop_back();

    // Print last ten updates
    // TODO: this if-else part will depend on the max length of the tweet
    // TODO: ensure no overflow at the top of the feed box
    int row = 15;
    for (std::string u : last_ten_updates)
    {
        int len = u.size();

        if (len <= 70) 
        {
            print(row, 5, u);
            row--;
        }
        else if (len <= 140)
        {
            print(row-1, 5, u.substr(0,70));
            print(row, 5, u.substr(70));
            row -= 2;
        }
        else //if (len <= 210)
        {
            print(row-2, 5, u.substr(0,70));
            print(row-1, 5, u.substr(70,70));
            print(row, 5, u.substr(140));
            row -= 3;
        }

    }
    
    return 0;
}

void ClientUI::_start_curses()
{
    if (ncurses_started)
    {
        refresh();
    }
    else
    {
        initscr();
        noecho();   // TODO: ???
        // atexit(_end_curses);
        ncurses_started = true;
    }

}

void ClientUI::_end_curses()
{
    if (ncurses_started && !isendwin())
    {
        endwin();
    }
}

void ClientUI::basic_screen()
{
    // Save previous position
    int old_y, old_x;
    getyx(stdscr, old_y, old_x);

    clear();
    
    move( 0,0); addstr("                                                                                ");
    move( 1,0); addstr("                              Welcome to Artuiter!                              ");
    move( 2,0); addstr("                                                                                ");
    move( 3,0); addstr("                                                                                ");
    move( 4,0); addstr("     YOUR FEED                                                                  ");
    move( 5,0); addstr("   **************************************************************************   ");
    move( 6,0); addstr("   *                                                                        *   ");
    move( 7,0); addstr("   *                                                                        *   ");
    move( 8,0); addstr("   *                                                                        *   ");
    move( 9,0); addstr("   *                                                                        *   ");
    move(10,0); addstr("   *                                                                        *   ");
    move(11,0); addstr("   *                                                                        *   ");
    move(12,0); addstr("   *                                                                        *   ");
    move(13,0); addstr("   *                                                                        *   ");
    move(14,0); addstr("   *                                                                        *   ");
    move(15,0); addstr("   *                                                                        *   ");
    move(16,0); addstr("   **************************************************************************   ");
    move(17,0); addstr("                                                                                ");
    move(18,0); addstr("     SEND COMMAND                                                               ");
    move(19,0); addstr("   **************************************************************************   ");
    move(20,0); addstr("   *                                                                        *   ");
    move(21,0); addstr("   **************************************************************************   ");
    move(22,0); addstr("                                                                                ");
    move(23,0); addstr("                                                                                ");

    // Move back to previous position
    move(old_y, old_x);
}

void ClientUI::print(int row, int col, std::string s)
{
    // Save previous position
    int old_y, old_x;
    getyx(stdscr, old_y, old_x);

    // Fill the end of the line based on the basic screen
    std::string rest = std::string(" *   ");
    rest.insert(0, 70-s.size(), ' ');

    // Print string in desired position
    move(row, col);
    addstr((s + rest).c_str());

    // Move back to previous position
    move(old_y, old_x);
}

void ClientUI::print(position pos, std::string s)
{
    print(pos.first, pos.second, s);
}

void ClientUI::_move(position pos)
{
    move(pos.first, pos.second);
}