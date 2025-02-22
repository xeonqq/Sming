#include <SmingCore.h>
#include <NtpClientDemo.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
#define WIFI_SSID "PleaseEnterSSID" // Put your SSID and password here
#define WIFI_PWD "PleaseEnterPass"
#endif

namespace
{
#pragma GCC diagnostic ignored "-Wunused-function"
void onNtpReceive(NtpClient& client, time_t timestamp);

SimpleTimer printTimer;

// Option 1
// Use this option if you want to have full control of NtpTime client
// Default : no automatic NTP requests sent, no automatic update of SystemTime
// Default : NTP_DEFAULT_SERVER and NTP_DEFAULT_AUTO_UPDATE_INTERVAL from ntpclient.h

// NtpClient ntpClient(onNtpReceive);

// Option 2
// Use this option if you want control but other server/timeout options
// Default : use server as defined in call, no automatic update of SystemTime
// Default : automatic NTP request at myRequestInterval seconds
// Default : if myRequestInterval == 0 -> no automatic NTP request

// NtpClient ntpClient ("my_ntp_server", myRequestInterval, onNtpReceive);

// Option 3
// Use this option if you want to start with autorefresh and autosystemupdate
// No further callback from ntpClient
// NtpClient ntpClient("pool.ntp.org", 30);

// Option 4
// only create pointer and initialize on ConnectOK
// NtpClient *ntpClient;

// Callback example using defined class ntpClientDemo
NtpClientDemo* demo;

// CallBack example 1
// ntpClientDemo dm1 = ntpClientDemo();
// or use
// ntpClientDemo dm1;

void onPrintSystemTime()
{
	Serial << _F("Local Time: ") << SystemClock.getSystemTimeString(eTZ_Local) << _F(", UTC Time: ")
		   << SystemClock.getSystemTimeString(eTZ_UTC) << endl;
}

// Called when time has been received by NtpClient (option 1 or 2)
// Either after manual requestTime() or when
// and automatic request has been made.
void onNtpReceive(NtpClient& client, time_t timestamp)
{
	SystemClock.setTime(timestamp, eTZ_UTC);

	Serial << _F("Time synchronized: ") << SystemClock.getSystemTimeString() << endl;
}

// Will be called when WiFi station timeout was reached
void connectFail(const String& ssid, MacAddress bssid, WifiDisconnectReason reason)
{
	Serial.println(_F("I'm NOT CONNECTED!"));
}

void gotIP(IpAddress ip, IpAddress netmask, IpAddress gateway)
{
	// Set specific parameters if started by option 1 or 2
	// Set client to do automatic time requests every 60 seconds.
	// NOTE: you should have longer interval in a real world application
	// no need for query for time every 60 sec, should be at least 10 minutes or so.
	//	ntpClient.setAutoQueryInterval(60);
	//	ntpClient.setAutoQuery(true);
	//  ntpClient.setAutoUpdateSystemClock(true);
	// Request to update time now.
	// Otherwise the set interval will pass before time
	// is updated.
	//	ntpClient.requestTime();

	//  When using option 4 -> create client after connect OK
	//  ntpClient = new NtpClient("my_ntp_server", myrefreshinterval);

	//	When using Delegate Callback Option 2
	if(demo == nullptr) {
		demo = new NtpClientDemo();
	}
}

} // namespace

// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println(_F("Sming. Let's do smart things!"));

	// Station - WiFi client
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD); // Put your SSID and password here

	printTimer.initializeMs<1000>(onPrintSystemTime).start();

	WifiEvents.onStationDisconnect(connectFail);
	WifiEvents.onStationGotIP(gotIP);
}
