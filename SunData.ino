// Sunrise, solar noon and sunrise data and calculation of whether recording should start
// Times are in NZST and each month has 31 days of data as the max possible.

/**
Class which handles time calculations for knowing how long to sleep before the next recording time.

D. Q. McDonald   August 2025
*/

#include "RecorderTime.h"



typedef struct
{
  uint8_t month;         // month of data
  uint8_t day;           // day of data
  uint8_t sunrise_hour;  // sunrise hour
  uint8_t sunrise_min;   // sunrise min
  uint8_t sunset_hour;   // sunset hour
  uint8_t sunset_min;    // sunset minute

} Sun_data_s;

Sun_data_s sun_data[] = {
  { 1, 1, 4, 51, 20, 13 },
  { 1, 2, 4, 52, 20, 13 },
  { 1, 3, 4, 52, 20, 13 },
  { 1, 4, 4, 53, 20, 13 },
  { 1, 5, 4, 54, 20, 13 },
  { 1, 6, 4, 55, 20, 13 },
  { 1, 7, 4, 56, 20, 13 },
  { 1, 8, 4, 57, 20, 12 },
  { 1, 9, 4, 59, 20, 12 },
  { 1, 10, 5, 0, 20, 12 },
  { 1, 11, 5, 1, 20, 12 },
  { 1, 12, 5, 2, 20, 11 },
  { 1, 13, 5, 3, 20, 11 },
  { 1, 14, 5, 4, 20, 10 },
  { 1, 15, 5, 6, 20, 10 },
  { 1, 16, 5, 7, 20, 9 },
  { 1, 17, 5, 8, 20, 9 },
  { 1, 18, 5, 9, 20, 8 },
  { 1, 19, 5, 11, 20, 8 },
  { 1, 20, 5, 12, 20, 7 },
  { 1, 21, 5, 13, 20, 6 },
  { 1, 22, 5, 15, 20, 5 },
  { 1, 23, 5, 16, 20, 5 },
  { 1, 24, 5, 17, 20, 4 },
  { 1, 25, 5, 19, 20, 3 },
  { 1, 26, 5, 20, 20, 2 },
  { 1, 27, 5, 21, 20, 1 },
  { 1, 28, 5, 23, 20, 0 },
  { 1, 29, 5, 24, 19, 59 },
  { 1, 30, 5, 25, 19, 58 },
  { 1, 31, 5, 27, 19, 57 },
  { 2, 1, 5, 28, 19, 56 },
  { 2, 2, 5, 30, 19, 55 },
  { 2, 3, 5, 31, 19, 54 },
  { 2, 4, 5, 32, 19, 52 },
  { 2, 5, 5, 34, 19, 51 },
  { 2, 6, 5, 35, 19, 50 },
  { 2, 7, 5, 37, 19, 49 },
  { 2, 8, 5, 38, 19, 47 },
  { 2, 9, 5, 39, 19, 46 },
  { 2, 10, 5, 41, 19, 45 },
  { 2, 11, 5, 42, 19, 43 },
  { 2, 12, 5, 44, 19, 42 },
  { 2, 13, 5, 45, 19, 41 },
  { 2, 14, 5, 46, 19, 39 },
  { 2, 15, 5, 48, 19, 38 },
  { 2, 16, 5, 49, 19, 36 },
  { 2, 17, 5, 50, 19, 35 },
  { 2, 18, 5, 52, 19, 33 },
  { 2, 19, 5, 53, 19, 32 },
  { 2, 20, 5, 55, 19, 30 },
  { 2, 21, 5, 56, 19, 29 },
  { 2, 22, 5, 57, 19, 27 },
  { 2, 23, 5, 59, 19, 25 },
  { 2, 24, 6, 0, 19, 24 },
  { 2, 25, 6, 1, 19, 22 },
  { 2, 26, 6, 3, 19, 21 },
  { 2, 27, 6, 4, 19, 19 },
  { 2, 28, 6, 5, 19, 17 },
  { 2, 29, 6, 7, 19, 16 },
  { 2, 30, 6, 7, 19, 16 },
  { 2, 31, 6, 7, 19, 16 },
  { 3, 1, 6, 7, 19, 16 },
  { 3, 2, 6, 8, 19, 14 },
  { 3, 3, 6, 9, 19, 12 },
  { 3, 4, 6, 10, 19, 11 },
  { 3, 5, 6, 12, 19, 9 },
  { 3, 6, 6, 13, 19, 7 },
  { 3, 7, 6, 14, 19, 5 },
  { 3, 8, 6, 15, 19, 4 },
  { 3, 9, 6, 17, 19, 2 },
  { 3, 10, 6, 18, 19, 0 },
  { 3, 11, 6, 19, 18, 58 },
  { 3, 12, 6, 21, 18, 57 },
  { 3, 13, 6, 22, 18, 55 },
  { 3, 14, 6, 23, 18, 53 },
  { 3, 15, 6, 24, 18, 51 },
  { 3, 16, 6, 25, 18, 49 },
  { 3, 17, 6, 27, 18, 48 },
  { 3, 18, 6, 28, 18, 46 },
  { 3, 19, 6, 29, 18, 44 },
  { 3, 20, 6, 30, 18, 42 },
  { 3, 21, 6, 32, 18, 40 },
  { 3, 22, 6, 33, 18, 39 },
  { 3, 23, 6, 34, 18, 37 },
  { 3, 24, 6, 35, 18, 35 },
  { 3, 25, 6, 36, 18, 33 },
  { 3, 26, 6, 38, 18, 31 },
  { 3, 27, 6, 39, 18, 30 },
  { 3, 28, 6, 40, 18, 28 },
  { 3, 29, 6, 41, 18, 26 },
  { 3, 30, 6, 42, 18, 24 },
  { 3, 31, 6, 44, 18, 22 },
  { 4, 1, 6, 45, 18, 21 },
  { 4, 2, 6, 46, 18, 19 },
  { 4, 3, 6, 47, 18, 17 },
  { 4, 4, 6, 48, 18, 15 },
  { 4, 5, 6, 49, 18, 14 },
  { 4, 6, 6, 51, 18, 12 },
  { 4, 7, 6, 52, 18, 10 },
  { 4, 8, 6, 53, 18, 8 },
  { 4, 9, 6, 54, 18, 7 },
  { 4, 10, 6, 55, 18, 5 },
  { 4, 11, 6, 57, 18, 3 },
  { 4, 12, 6, 58, 18, 1 },
  { 4, 13, 6, 59, 18, 0 },
  { 4, 14, 7, 0, 17, 58 },
  { 4, 15, 7, 1, 17, 56 },
  { 4, 16, 7, 2, 17, 55 },
  { 4, 17, 7, 4, 17, 53 },
  { 4, 18, 7, 5, 17, 51 },
  { 4, 19, 7, 6, 17, 50 },
  { 4, 20, 7, 7, 17, 48 },
  { 4, 21, 7, 8, 17, 47 },
  { 4, 22, 7, 9, 17, 45 },
  { 4, 23, 7, 11, 17, 44 },
  { 4, 24, 7, 12, 17, 42 },
  { 4, 25, 7, 13, 17, 41 },
  { 4, 26, 7, 14, 17, 39 },
  { 4, 27, 7, 15, 17, 38 },
  { 4, 28, 7, 16, 17, 36 },
  { 4, 29, 7, 18, 17, 35 },
  { 4, 30, 7, 19, 17, 33 },
  { 4, 31, 7, 20, 17, 32 },
  { 5, 1, 7, 20, 17, 32 },
  { 5, 2, 7, 21, 17, 30 },
  { 5, 3, 7, 22, 17, 29 },
  { 5, 4, 7, 23, 17, 28 },
  { 5, 5, 7, 24, 17, 26 },
  { 5, 6, 7, 26, 17, 25 },
  { 5, 7, 7, 27, 17, 24 },
  { 5, 8, 7, 28, 17, 23 },
  { 5, 9, 7, 29, 17, 21 },
  { 5, 10, 7, 30, 17, 20 },
  { 5, 11, 7, 31, 17, 19 },
  { 5, 12, 7, 32, 17, 18 },
  { 5, 13, 7, 33, 17, 17 },
  { 5, 14, 7, 34, 17, 16 },
  { 5, 15, 7, 35, 17, 15 },
  { 5, 16, 7, 36, 17, 13 },
  { 5, 17, 7, 37, 17, 12 },
  { 5, 18, 7, 39, 17, 12 },
  { 5, 19, 7, 40, 17, 11 },
  { 5, 20, 7, 41, 17, 10 },
  { 5, 21, 7, 42, 17, 9 },
  { 5, 22, 7, 43, 17, 8 },
  { 5, 23, 7, 44, 17, 7 },
  { 5, 24, 7, 45, 17, 6 },
  { 5, 25, 7, 45, 17, 6 },
  { 5, 26, 7, 46, 17, 5 },
  { 5, 27, 7, 47, 17, 4 },
  { 5, 28, 7, 48, 17, 3 },
  { 5, 29, 7, 49, 17, 3 },
  { 5, 30, 7, 50, 17, 2 },
  { 5, 31, 7, 51, 17, 2 },
  { 6, 1, 7, 52, 17, 1 },
  { 6, 2, 7, 52, 17, 1 },
  { 6, 3, 7, 53, 17, 0 },
  { 6, 4, 7, 54, 17, 0 },
  { 6, 5, 7, 55, 16, 59 },
  { 6, 6, 7, 55, 16, 59 },
  { 6, 7, 7, 56, 16, 59 },
  { 6, 8, 7, 57, 16, 58 },
  { 6, 9, 7, 57, 16, 58 },
  { 6, 10, 7, 58, 16, 58 },
  { 6, 11, 7, 58, 16, 58 },
  { 6, 12, 7, 59, 16, 58 },
  { 6, 13, 7, 59, 16, 58 },
  { 6, 14, 8, 0, 16, 58 },
  { 6, 15, 8, 0, 16, 58 },
  { 6, 16, 8, 1, 16, 58 },
  { 6, 17, 8, 1, 16, 58 },
  { 6, 18, 8, 2, 16, 58 },
  { 6, 19, 8, 2, 16, 58 },
  { 6, 20, 8, 2, 16, 58 },
  { 6, 21, 8, 2, 16, 58 },
  { 6, 22, 8, 3, 16, 58 },
  { 6, 23, 8, 3, 16, 59 },
  { 6, 24, 8, 3, 16, 59 },
  { 6, 25, 8, 3, 16, 59 },
  { 6, 26, 8, 3, 17, 0 },
  { 6, 27, 8, 3, 17, 0 },
  { 6, 28, 8, 3, 17, 0 },
  { 6, 29, 8, 3, 17, 1 },
  { 6, 30, 8, 3, 17, 1 },
  { 6, 31, 8, 3, 17, 2 },
  { 7, 1, 8, 3, 17, 2 },
  { 7, 2, 8, 3, 17, 2 },
  { 7, 3, 8, 3, 17, 3 },
  { 7, 4, 8, 3, 17, 3 },
  { 7, 5, 8, 2, 17, 4 },
  { 7, 6, 8, 2, 17, 5 },
  { 7, 7, 8, 2, 17, 5 },
  { 7, 8, 8, 1, 17, 6 },
  { 7, 9, 8, 1, 17, 7 },
  { 7, 10, 8, 1, 17, 7 },
  { 7, 11, 8, 0, 17, 8 },
  { 7, 12, 8, 0, 17, 9 },
  { 7, 13, 7, 59, 17, 10 },
  { 7, 14, 7, 58, 17, 10 },
  { 7, 15, 7, 58, 17, 11 },
  { 7, 16, 7, 57, 17, 12 },
  { 7, 17, 7, 57, 17, 13 },
  { 7, 18, 7, 56, 17, 14 },
  { 7, 19, 7, 55, 17, 15 },
  { 7, 20, 7, 54, 17, 16 },
  { 7, 21, 7, 54, 17, 16 },
  { 7, 22, 7, 53, 17, 17 },
  { 7, 23, 7, 52, 17, 18 },
  { 7, 24, 7, 51, 17, 19 },
  { 7, 25, 7, 50, 17, 20 },
  { 7, 26, 7, 49, 17, 21 },
  { 7, 27, 7, 48, 17, 22 },
  { 7, 28, 7, 47, 17, 23 },
  { 7, 29, 7, 46, 17, 24 },
  { 7, 30, 7, 45, 17, 25 },
  { 7, 31, 7, 44, 17, 26 },
  { 8, 1, 7, 43, 17, 27 },
  { 8, 2, 7, 41, 17, 28 },
  { 8, 3, 7, 40, 17, 29 },
  { 8, 4, 7, 39, 17, 30 },
  { 8, 5, 7, 38, 17, 32 },
  { 8, 6, 7, 37, 17, 33 },
  { 8, 7, 7, 35, 17, 34 },
  { 8, 8, 7, 34, 17, 35 },
  { 8, 9, 7, 33, 17, 36 },
  { 8, 10, 7, 31, 17, 37 },
  { 8, 11, 7, 30, 17, 38 },
  { 8, 12, 7, 28, 17, 39 },
  { 8, 13, 7, 27, 17, 40 },
  { 8, 14, 7, 26, 17, 41 },
  { 8, 15, 7, 24, 17, 42 },
  { 8, 16, 7, 23, 17, 43 },
  { 8, 17, 7, 21, 17, 45 },
  { 8, 18, 7, 20, 17, 46 },
  { 8, 19, 7, 18, 17, 47 },
  { 8, 20, 7, 17, 17, 48 },
  { 8, 21, 7, 15, 17, 49 },
  { 8, 22, 7, 13, 17, 50 },
  { 8, 23, 7, 12, 17, 51 },
  { 8, 24, 7, 10, 17, 52 },
  { 8, 25, 7, 9, 17, 53 },
  { 8, 26, 7, 7, 17, 54 },
  { 8, 27, 7, 5, 17, 56 },
  { 8, 28, 7, 4, 17, 57 },
  { 8, 29, 7, 2, 17, 58 },
  { 8, 30, 7, 0, 17, 59 },
  { 8, 31, 6, 58, 18, 0 },
  { 9, 1, 6, 57, 18, 1 },
  { 9, 2, 6, 55, 18, 2 },
  { 9, 3, 6, 53, 18, 3 },
  { 9, 4, 6, 51, 18, 4 },
  { 9, 5, 6, 50, 18, 5 },
  { 9, 6, 6, 48, 18, 7 },
  { 9, 7, 6, 46, 18, 8 },
  { 9, 8, 6, 44, 18, 9 },
  { 9, 9, 6, 43, 18, 10 },
  { 9, 10, 6, 41, 18, 11 },
  { 9, 11, 6, 39, 18, 12 },
  { 9, 12, 6, 37, 18, 13 },
  { 9, 13, 6, 35, 18, 14 },
  { 9, 14, 6, 34, 18, 15 },
  { 9, 15, 6, 32, 18, 16 },
  { 9, 16, 6, 30, 18, 18 },
  { 9, 17, 6, 28, 18, 19 },
  { 9, 18, 6, 26, 18, 20 },
  { 9, 19, 6, 24, 18, 21 },
  { 9, 20, 6, 23, 18, 22 },
  { 9, 21, 6, 21, 18, 23 },
  { 9, 22, 6, 19, 18, 24 },
  { 9, 23, 6, 17, 18, 25 },
  { 9, 24, 6, 15, 18, 27 },
  { 9, 25, 6, 13, 18, 28 },
  { 9, 26, 6, 12, 18, 29 },
  { 9, 27, 6, 10, 18, 30 },
  { 9, 28, 6, 8, 18, 31 },
  { 9, 29, 6, 6, 18, 32 },
  { 9, 30, 6, 4, 18, 33 },
  { 9, 31, 6, 2, 18, 35 },
  { 10, 1, 6, 2, 18, 35 },
  { 10, 2, 6, 1, 18, 36 },
  { 10, 3, 5, 59, 18, 37 },
  { 10, 4, 5, 57, 18, 38 },
  { 10, 5, 5, 55, 18, 39 },
  { 10, 6, 5, 53, 18, 40 },
  { 10, 7, 5, 52, 18, 42 },
  { 10, 8, 5, 50, 18, 43 },
  { 10, 9, 5, 48, 18, 44 },
  { 10, 10, 5, 46, 18, 45 },
  { 10, 11, 5, 45, 18, 46 },
  { 10, 12, 5, 43, 18, 48 },
  { 10, 13, 5, 41, 18, 49 },
  { 10, 14, 5, 39, 18, 50 },
  { 10, 15, 5, 38, 18, 51 },
  { 10, 16, 5, 36, 18, 53 },
  { 10, 17, 5, 34, 18, 54 },
  { 10, 18, 5, 33, 18, 55 },
  { 10, 19, 5, 31, 18, 56 },
  { 10, 20, 5, 29, 18, 58 },
  { 10, 21, 5, 28, 18, 59 },
  { 10, 22, 5, 26, 19, 0 },
  { 10, 23, 5, 25, 19, 1 },
  { 10, 24, 5, 23, 19, 3 },
  { 10, 25, 5, 21, 19, 4 },
  { 10, 26, 5, 20, 19, 5 },
  { 10, 27, 5, 18, 19, 7 },
  { 10, 28, 5, 17, 19, 8 },
  { 10, 29, 5, 15, 19, 9 },
  { 10, 30, 5, 14, 19, 11 },
  { 10, 31, 5, 12, 19, 12 },
  { 11, 1, 5, 11, 19, 13 },
  { 11, 2, 5, 10, 19, 15 },
  { 11, 3, 5, 8, 19, 16 },
  { 11, 4, 5, 7, 19, 17 },
  { 11, 5, 5, 6, 19, 19 },
  { 11, 6, 5, 4, 19, 20 },
  { 11, 7, 5, 3, 19, 21 },
  { 11, 8, 5, 2, 19, 23 },
  { 11, 9, 5, 1, 19, 24 },
  { 11, 10, 5, 0, 19, 25 },
  { 11, 11, 4, 58, 19, 27 },
  { 11, 12, 4, 57, 19, 28 },
  { 11, 13, 4, 56, 19, 29 },
  { 11, 14, 4, 55, 19, 31 },
  { 11, 15, 4, 54, 19, 32 },
  { 11, 16, 4, 53, 19, 33 },
  { 11, 17, 4, 52, 19, 35 },
  { 11, 18, 4, 51, 19, 36 },
  { 11, 19, 4, 50, 19, 37 },
  { 11, 20, 4, 50, 19, 39 },
  { 11, 21, 4, 49, 19, 40 },
  { 11, 22, 4, 48, 19, 41 },
  { 11, 23, 4, 47, 19, 42 },
  { 11, 24, 4, 46, 19, 44 },
  { 11, 25, 4, 46, 19, 45 },
  { 11, 26, 4, 45, 19, 46 },
  { 11, 27, 4, 45, 19, 47 },
  { 11, 28, 4, 44, 19, 49 },
  { 11, 29, 4, 44, 19, 50 },
  { 11, 30, 4, 43, 19, 51 },
  { 11, 31, 4, 43, 19, 52 },
  { 12, 1, 4, 43, 19, 52 },
  { 12, 2, 4, 42, 19, 53 },
  { 12, 3, 4, 42, 19, 54 },
  { 12, 4, 4, 42, 19, 55 },
  { 12, 5, 4, 41, 19, 56 },
  { 12, 6, 4, 41, 19, 57 },
  { 12, 7, 4, 41, 19, 58 },
  { 12, 8, 4, 41, 19, 59 },
  { 12, 9, 4, 41, 20, 0 },
  { 12, 10, 4, 41, 20, 1 },
  { 12, 11, 4, 41, 20, 2 },
  { 12, 12, 4, 41, 20, 3 },
  { 12, 13, 4, 41, 20, 4 },
  { 12, 14, 4, 41, 20, 5 },
  { 12, 15, 4, 41, 20, 6 },
  { 12, 16, 4, 41, 20, 6 },
  { 12, 17, 4, 42, 20, 7 },
  { 12, 18, 4, 42, 20, 8 },
  { 12, 19, 4, 42, 20, 8 },
  { 12, 20, 4, 43, 20, 9 },
  { 12, 21, 4, 43, 20, 9 },
  { 12, 22, 4, 44, 20, 10 },
  { 12, 23, 4, 44, 20, 10 },
  { 12, 24, 4, 45, 20, 11 },
  { 12, 25, 4, 45, 20, 11 },
  { 12, 26, 4, 46, 20, 12 },
  { 12, 27, 4, 47, 20, 12 },
  { 12, 28, 4, 47, 20, 12 },
  { 12, 29, 4, 48, 20, 12 },
  { 12, 30, 4, 49, 20, 13 },
  { 12, 31, 4, 50, 20, 13 },
};



#define DAYS_PER_MONTH 31  // Every day has

const char* SUNRISE_PREFIX = "SR";
const char* DAY_PREFIX = "DA";
const char* SUNSET_PREFIX = "SS";

// Constants for how long after sunrise and how long before sunset we should do a random recording time
const int MINUTES_BEFORE_SUNSET = 90;
const int MINUTES_AFTER_SUNRISE = 90;


bool checkNextEvent(int sunrise_offset, int sunset_offset, int* sleep_hour, int* sleep_minute, const char** prefix,
                    bool is_sunrise) {
  // Check for the next recording event. If we are at it then will return true to indicate recording should begin
  // Otherwise return false and set the sleeping time in sleep_hour and sleep_minute.
  // Prefix will return a pointer to a two letter code for the filename based on what event recording will be done for
  // If "is_sunrise" is true will calculate sleep period to wake sometime at random between 1.5 hr after sunrise
  // and 1.5 hours before sunset.


#ifdef DEBUG
  Serial.print("checkForEvent(  ");
  Serial.print("sunrise_offset = ");
  Serial.print(sunrise_offset);
  Serial.print("; sunset offset = ");
  Serial.print(sunset_offset);
  Serial.print("; is_sunrise = ");
  Serial.print(is_sunrise);
  Serial.println(" )");
  Log.trace(F("checkForEvent(sunrise_offset=%d,sunset_offset=%d, is_sunrise = %d\n"), sunrise_offset, sunset_offset, is_sunrise);
#endif


  // Get current date and time
  int current_month = month();
  int current_day = day();
  int current_hour = hour();
  int current_minute = minute();


#ifdef DEBUG
  Serial.print("  checkNextEvent() current month:");
  Serial.println(current_month);
  Serial.print("  checkNextEvent() current day:");
  Serial.println(current_day);
  Serial.print("  checkNextEvent() current hour:");
  Serial.println(current_hour);
  Serial.print("  checkNextEvent() current minute:");
  Serial.println(current_minute);
  Log.trace(F("checkForEvent() month = %d, day = %d, hour = %d, minute=%d\n"), current_month,
            current_day, current_hour, current_minute);
#endif

  RecorderTime current_time(current_hour, current_minute);


  //Compute the index into the list of event times
  int idx = (current_month - 1) * DAYS_PER_MONTH + current_day - 1;
#ifdef DEBUG
  Serial.print("  checkNextEvent() index:");
  Serial.println(idx);
  Log.trace(F("checkForEvent() index = %d\n"), idx);
#endif


  // Create objects for sunrise and sunset
  RecorderTime sunrise((int)sun_data[idx].sunrise_hour, (int)sun_data[idx].sunrise_min);

#ifdef DEBUG
  Serial.print("  checkNextEvent() sunrise hour (without offset): ");
  Serial.println(sunrise.hour());
  Serial.print("  checkNextEvent() sunrise minute (without offset): ");
  Serial.println(sunrise.minute());
  Log.trace(F("checkForEvent() sunrise(without offset): %d:%d\n"), sunrise.hour(), sunrise.minute());
#endif


  RecorderTime sunset((int)sun_data[idx].sunset_hour, (int)sun_data[idx].sunset_min);
#ifdef DEBUG
  Serial.print("  checkNextEvent() sunset hour (without offset): ");
  Serial.println(sunset.hour());
  Serial.print("  checkNextEvent() sunset minute (without offset): ");
  Serial.println(sunset.minute());
  Log.trace(F("checkForEvent() sunset(without offset): %d:%d\n"), sunset.hour(), sunset.minute());
#endif


  // Also next sunrise
  int next_idx = idx + 1;

  // If at end of data use the first of the year
  if (next_idx >= int(sizeof(sun_data) / sizeof(Sun_data_s))) {
    next_idx = 0;
  }
  RecorderTime next_sunrise((int)sun_data[next_idx].sunrise_hour, (int)sun_data[next_idx].sunrise_min);
#ifdef DEBUG
  Serial.print("  checkNextEvent() next sunrise hour (without offset): ");
  Serial.println(next_sunrise.hour());
  Serial.print("  checkNextEvent() next sunrise (without offset): ");
  Serial.println(next_sunrise.minute());
  Log.trace(F("checkForEvent() next day sunrise (without offset): %d:%d\n"), next_sunrise.hour(), next_sunrise.minute());
#endif

  if (is_sunrise) {
    // We are at sunrise and have done the recordings. Figure out a time of day between 1.5 hr after sunrise and 1.5 before
    // sunset.
#ifdef DEBUG
    Serial.println("  checkNextEvent() At sunrise - calculating random sleep time ");
    Log.trace(F("  checkNextEvent() At sunrise - calculating random sleep time "));
#endif

    int start = sunrise.in_minutes();
    int end = sunset.in_minutes();
    // offset by 90 minutes

    start += MINUTES_AFTER_SUNRISE;
    end -= MINUTES_BEFORE_SUNSET;

#ifdef DEBUG
    Serial.print("  checkNextEvent() start minutes: ");
    Serial.println(start);
    Serial.print("  checkNextEvent() end minutes ");
    Serial.println(end);
    Log.trace(F("checkForEvent() start, end minutes: %d:%d\n"), start, end);
#endif
    int day_length = end - start;

    int random_minutes = random(MINUTES_AFTER_SUNRISE, day_length);
    uint16_t h, m, s;

    secondsToHMS(random_minutes * SEC_PER_MINUTE, h, m, s);

#ifdef DEBUG
    Serial.print("  checkNextEvent() random_minutes ");
    Serial.println(random_minutes);
    Log.trace(F("checkForEvent() random_minutes %d\n"), random_minutes);
#endif

    secondsToHMS(random_minutes * SEC_PER_MINUTE, h, m, s);  // Convert back to hours and minutes

    *prefix = SUNSET_PREFIX;
    *sleep_hour = h;
    *sleep_minute = m;
    return false;


  } else {

    // Not currently sunrise so figure out how long to the next event.
    // Start by applying offsets:

    sunrise.applyOffset(sunrise_offset);

#ifdef DEBUG
    Serial.print("  checkNextEvent() sunrise hour (with offset): ");
    Serial.println(sunrise.hour());
    Serial.print("  checkNextEvent() sunrise minute (with offset): ");
    Serial.println(sunrise.minute());
    Log.trace(F("checkForEvent() sunrise(with offset): %d:%d\n"), sunrise.hour(), sunrise.minute());

#endif

    sunset.applyOffset(sunset_offset);
#ifdef DEBUG
    Serial.print("  checkNextEvent() sunset hour (with offset): ");
    Serial.println(sunset.hour());
    Serial.print("  checkNextEvent() sunset minute (with offset): ");
    Serial.println(sunset.minute());
    Log.trace(F("checkForEvent() sunset(with offset): %d:%d\n"), sunset.hour(), sunset.minute());
#endif

    next_sunrise.applyOffset(sunrise_offset);
#ifdef DEBUG
    Serial.print("  checkNextEvent() next sunrise hour (with offset): ");
    Serial.println(next_sunrise.hour());
    Serial.print("  checkNextEvent() next sunrise (with offset): ");
    Serial.println(next_sunrise.minute());
    Log.trace(F("checkForEvent() next day sunrise (with offset): %d:%d\n"), next_sunrise.hour(), next_sunrise.minute());
#endif


    // Now start comparing the possible events:
    // Sunrise
    if (current_time.before(sunrise)) {
#ifdef DEBUG
      Serial.println("checkNextEvent() before sunrise");
      Log.trace(F("checkForEvent() before sunrise\n"));

#endif
      *prefix = SUNRISE_PREFIX;
      return current_time.sleepOrRecord(sunrise, sleep_hour, sleep_minute);
    }

    if (current_time.before(sunset)) {

#ifdef DEBUG
      Serial.println("checkNextEvent() before sunset ");
      Log.trace(F("checkForEvent() before sunset\n"));

#endif
      *prefix = SUNSET_PREFIX;
      return current_time.sleepOrRecord(sunset, sleep_hour, sleep_minute);
    }

// Finally if we get this far we must be after sunset:
#ifdef DEBUG
    Serial.println("checkNextEvent() before next day sunrise ");
    Log.trace(F("checkForEvent() before next day\n"));

#endif
    *prefix = SUNRISE_PREFIX;
    return current_time.sleepOrRecord(next_sunrise, sleep_hour, sleep_minute);
  }
}
