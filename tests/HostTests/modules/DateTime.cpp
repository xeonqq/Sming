#include <HostTests.h>
#include <DateTime.h>
#include <FlashString/Array.hpp>

#include "DateTimeData.h"

class DateTimeTest : public TestGroup
{
public:
	DateTimeTest() : TestGroup(_F("DateTime"))
	{
	}

	void execute() override
	{
		Serial << _F("time_t is ") << sizeof(time_t) * 8 << _F(" bits") << endl;

		TEST_CASE("fromHttpDate()")
		{
			checkHttpDates(VALID_HTTP_DATE);
		}

		TEST_CASE("fromISO8601 (32-bit)")
		{
			checkIsoTimes(VALID_ISO_DATETIME, false);
		}

		TEST_CASE("fromISO8601 (time only)")
		{
			checkIsoTimes(VALID_ISO_TIME, true);
		}

		if(sizeof(time_t) == 8) {
			TEST_CASE("fromISO8601 (64-bit)")
			{
				checkIsoTimes(VALID_ISO_DATETIME64, false);
			}
		}

		// dt = testDate;
		// debug_d("\"%s\"", dt.toFullDateTimeString().c_str());

		TEST_CASE("setTime")
		{
			checkSetTime(VALID_HTTP_DATE);
			checkSetTime(VALID_ISO_DATETIME);
			if(sizeof(time_t) == 8) {
				checkSetTime(VALID_ISO_DATETIME64);
			}
		}

		TEST_CASE("setTime speed check")
		{
			OneShotFastUs timer;
			unsigned count = checkSetTime(VALID_HTTP_DATE);
			count += checkSetTime(VALID_ISO_DATETIME);
			auto elapsed = timer.elapsedTime();
			Serial << "Checked " << count << " dates in " << elapsed.toString();
			if(count != 0) {
				Serial << ", " << elapsed / count << " per date";
			}
			Serial << endl;
		}

		TEST_CASE("getMonthDays")
		{
			for(auto year : {1980, 1981}) {
				unsigned yearDays{0};
				for(unsigned month = dtJanuary; month <= dtDecember; ++month) {
					auto days = DateTime::getMonthDays(month, year);
					yearDays += days;
					Serial << DateTime::getIsoMonthName(month) << ' ' << year << " : " << days << endl;
				}
				REQUIRE_EQ(yearDays, DateTime::getDaysInYear(year));
			}
		}

		TEST_CASE("getDayName")
		{
			for(unsigned day = dtSunday; day <= dtSaturday; ++day) {
				Serial << day << ": " << DateTime::getIsoDayName(day) << ", " << DateTime::getLocaleDayName(day)
					   << endl;
			}
		}

		TEST_CASE("getMonthName")
		{
			for(unsigned month = dtJanuary; month <= dtDecember; ++month) {
				Serial << month << ": " << DateTime::getIsoMonthName(month) << ", "
					   << DateTime::getLocaleMonthName(month) << endl;
			}
		}
	}

	void checkHttpDates(const FSTR::Array<TestDate>& dates)
	{
		for(auto date : VALID_HTTP_DATE) {
			DateTime dt;
			String s(*date.stringToParse);
			Serial << s << endl;
			REQUIRE(dt.fromHttpDate(s));
			REQUIRE_EQ(date.unixTimestamp, dt.toUnixTime());

			dt.setTime(date.unixTimestamp);
			REQUIRE_EQ(date.unixTimestamp, dt.toUnixTime());

			REQUIRE_EQ(String(*date.expectedString), dt.toHTTPDate());
			Serial.println();
		}
	}

	void checkIsoTimes(const FSTR::Array<TestDate>& dates, bool timeOnly)
	{
		for(auto date : dates) {
			DateTime dt;
			String s(*date.stringToParse);
			Serial << s << ", " << String(time_t(date.unixTimestamp), HEX) << endl;
			REQUIRE(dt.fromISO8601(s));
			Serial << dt.toISO8601() << endl;
			REQUIRE_EQ(date.unixTimestamp, dt.toUnixTime());
			REQUIRE_EQ(date.milliseconds, dt.Milliseconds);

			if(timeOnly) {
				REQUIRE_EQ(String(*date.expectedString), dt.format("%T"));
			} else {
				REQUIRE_EQ(String(*date.expectedString), dt.toISO8601());
			}

			dt.setTime(date.unixTimestamp);
			REQUIRE_EQ(date.unixTimestamp, dt.toUnixTime());

			Serial.println();
		}
	}

	// Return number of dates checked
	unsigned checkSetTime(const FSTR::Array<TestDate>& dates, bool silent = false)
	{
		unsigned checkCount{0};

		for(auto date : dates) {
			const DateTime refDate(date.unixTimestamp);

			if(!silent) {
				Serial << "RefDate " << refDate.toFullDateTimeString() << endl;
			}

			auto check = [&](int secOffset, int minOffset, int hourOffset, int dayOffset) {
				++checkCount;
				const int sec = secOffset + refDate.Second;
				const int min = minOffset + refDate.Minute;
				const int hour = hourOffset + refDate.Hour;
				const int day = dayOffset + refDate.Day;
				DateTime dt = DateTime ::toUnixTime(sec, min, hour, day, refDate.Month, refDate.Year);
				time_t refTime = DateTime::toUnixTime(0, 0, 0, 1, refDate.Month, refDate.Year);
				refTime += sec;
				refTime += int64_t(min) * 60;
				refTime += int64_t(hour) * 60 * 60;
				refTime += int64_t(day - 1) * 24 * 60 * 60;
				time_t calcTime = dt.toUnixTime();
				if(calcTime == refTime) {
					return;
				}

				char buf[100];
				m_snprintf(buf, sizeof(buf), " (%ds, %dm %dh, %dd)", secOffset, minOffset, hourOffset, dayOffset);
				Serial << _F("Check ") << DateTime(refTime).toFullDateTimeString() << buf << endl;
				Serial << _F("Got   ") << dt.toFullDateTimeString() << ", diff " << refTime - calcTime << endl;
				REQUIRE_EQ(refTime, calcTime);
			};

			for(int offset = -10000; offset < 10000; offset += 555) {
				check(offset, 0, 0, 0);
				check(0, offset, 0, 0);
				check(0, 0, offset, 0);
				check(0, 0, 0, offset);
			}
		};

		return checkCount;
	}
};

void REGISTER_TEST(DateTime)
{
	registerGroup<DateTimeTest>();
}
