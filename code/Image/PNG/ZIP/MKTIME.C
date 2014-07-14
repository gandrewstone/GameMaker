/* free mktime function
   Copyright 1988, 1989 by David MacKenzie <djm@ai.mit.edu>
   and Michael Haertel <mike@ai.mit.edu>
   Unlimited distribution permitted provided this copyright notice is
   retained and any functional modifications are prominently identified.  */

/* Note: This version of mktime is ignorant of the tzfile; it does not
   return correct results during the few hours around when daylight savings
   time goes in to or out of effect.  It also does not allow or adjust
   for invalid values in any of the fields, contrary to the ANSI C
   specification. */

#ifdef MKTIME_MISSING
#include <sys/types.h>
#include <time.h>

time_t mkgmtime ();

/* Return the equivalent in seconds past 12:00:00 a.m. Jan 1, 1970 GMT
   of the local time and date in the exploded time structure `tm',
   and set `tm->tm_yday', `tm->tm_wday', and `tm->tm_isdst'.
   Return -1 if any of the other fields in `tm' has an invalid value. */

time_t
mktime (tm)
     struct tm *tm;
{
  struct tm save_tm;            /* Copy of contents of `*tm'. */
  struct tm *ltm;               /* Local time. */
  time_t then;                  /* The time to return. */

  then = mkgmtime (tm);
  if (then == -1)
    return -1;

  /* In case `tm' points to the static area used by localtime,
     save its contents and restore them later. */
  save_tm = *tm;
  /* Correct for the timezone and any daylight savings time.
     If a change to or from daylight savings time occurs between when
     it is the time in `tm' locally and when it is that time in Greenwich,
     the change to or from dst is ignored, but that is a rare case. */
  then += then - mkgmtime (localtime (&then));

  ltm = localtime (&then);
  save_tm.tm_yday = ltm->tm_yday;
  save_tm.tm_wday = ltm->tm_wday;
  save_tm.tm_isdst = ltm->tm_isdst;
  *tm = save_tm;

  return then;
}

/* Nonzero if `y' is a leap year, else zero. */
#define leap(y) (((y) % 4 == 0 && (y) % 100 != 0) || (y) % 400 == 0)

/* Number of leap years from 1970 to `y' (not including `y' itself). */
#define nleap(y) (((y) - 1969) / 4 - ((y) - 1901) / 100 + ((y) - 1601) / 400)

/* Number of days in each month of the year. */
static char monlens[] =
{
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/* Return the equivalent in seconds past 12:00:00 a.m. Jan 1, 1970 GMT
   of the Greenwich Mean time and date in the exploded time structure `tm',
   and set `tm->tm_yday', `tm->tm_wday', and `tm->tm_isdst'.
   Return -1 if any of the other fields in `tm' has an invalid value. */

time_t
mkgmtime (tm)
     struct tm *tm;
{
  int years, months, days, hours, minutes, seconds;

  years = tm->tm_year + 1900;   /* year - 1900 -> year */
  months = tm->tm_mon;          /* 0..11 */
  days = tm->tm_mday - 1;       /* 1..31 -> 0..30 */
  hours = tm->tm_hour;          /* 0..23 */
  minutes = tm->tm_min;         /* 0..59 */
  seconds = tm->tm_sec;         /* 0..61 in ANSI C. */

  if (years < 1970
      || months < 0 || months > 11
      || days < 0
      || days > monlens[months] + (months == 1 && leap (years)) - 1
      || hours < 0 || hours > 23
      || minutes < 0 || minutes > 59
      || seconds < 0 || seconds > 61)
  return -1;

  /* Set `days' to the number of days into the year. */
  if (months > 1 && leap (years))
    ++days;
  while (months-- > 0)
    days += monlens[months];
  tm->tm_yday = days;

  /* Now set `days' to the number of days since Jan 1, 1970. */
  days += 365 * (years - 1970) + nleap (years);
  tm->tm_wday = (days + 4) % 7; /* Jan 1, 1970 was Thursday. */
  tm->tm_isdst = 0;

  return 86400 * days + 3600 * hours + 60 * minutes + seconds;
}
#endif
