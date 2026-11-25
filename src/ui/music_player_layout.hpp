#ifndef __UI_MUSIC_PLAYER_LAYOUT_HPP__
#define __UI_MUSIC_PLAYER_LAYOUT_HPP__

#include <SDL2/SDL.h>

#include <string>

/*
 * The geometry and the texts of the music player, as pure functions of integers, thus a host test
 * checks them with no display. The proportions come from the board `tpl-lecteur` of the canvas,
 * drawn at 1024 x 768.
 */

/** The rectangles of the player, in the coordinates of the screen */
struct PlayerRects {
    /** The art takes the right of the screen, over the whole height. Its left edge fades out. */
    SDL_Rect art;
    /** The row of the format chip and of the counter at the top left */
    SDL_Rect topRow;
    /** The column of the artist, the title, the album and the chips, from `textTop` down */
    SDL_Rect textColumn;
    /** The row of the elapsed time, the bar and the remaining time */
    SDL_Rect progressRow;
};

/**
 * Lays out the player on a screen of `width` x `height`. `padding` is the margin of the pills,
 * `inset` the extra margin of the text from it, `pillHeight` the height of a pill row, and
 * `rowHeight` the height of the progress row.
 */
PlayerRects layoutPlayer(int width, int height, int padding, int inset, int pillHeight, int rowHeight);

/**
 * The room of the lyrics, between the bottom of the text column's last row at `contentBottom` and
 * the progress row at `progressTop`, `gap` away from both. The height is 0 when nothing fits.
 */
SDL_Rect layoutLyricsRect(const PlayerRects& rects, int contentBottom, int gap);

/**
 * The alpha of the art at the column `x` of an art `width` pixels wide, from 0 at the left edge
 * to 255 at `fadeWidth`, and 255 beyond. It rises smoothly, and never falls.
 */
int getArtFadeAlpha(int x, int width, int fadeWidth);

/** The width of the fade of an art `width` pixels wide */
int getArtFadeWidth(int width);

/** Returns the time as "m:ss", or "h:mm:ss" from one hour. A negative time counts as 0. */
std::string formatPlayerTime(int milliseconds);

/** Returns the time left as "-m:ss", and "-0:00" when nothing is left. */
std::string formatRemainingTime(int positionMs, int durationMs);

/** Returns the counter of the queue as "02 / 15". */
std::string formatTrackCounter(int number, int total);

#endif
