/** gccDppConsole.cpp

*/

// gccDppConsole.cpp : Defines the entry point for the console application.
#include <iostream>
using namespace std; 
#include "ConsoleHelper.h"
#include "stringex.h"

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#define CLEAR_TERM "cls"
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#define _getch getchar
#define CLEAR_TERM "clear"
#endif

CConsoleHelper chdpp;					// DPP communications functions
bool bRunSpectrumTest = false;			// run spectrum test
bool bRunConfigurationTest = false;		// run configuration test
bool bHaveStatusResponse = false;		// have status response
bool bHaveConfigFromHW = false;			// have configuration from hardware

// connect to default dpp
//		CConsoleHelper::LibUsb_Connect_Default_DPP	// LibUsb connect to default DPP
void ConnectToDefaultDPP()
{
	cout << endl;
	cout << "Running DPP LibUsb tests from console..." << endl;
	cout << endl;
	cout << "\tConnecting to default LibUsb device..." << endl;
	if (chdpp.LibUsb_Connect_Default_DPP()) {
		cout << "\t\tLibUsb DPP device connected." << endl;
		cout << "\t\tLibUsb DPP devices present: "  << chdpp.LibUsb_NumDevices << endl;
	} else {
		cout << "\t\tLibUsb DPP device not connected." << endl;
		cout << "\t\tNo LibUsb DPP device present." << endl;
	}
}

// Get DPP Status
//		CConsoleHelper::LibUsb_isConnected						// check if DPP is connected
//		CConsoleHelper::LibUsb_SendCommand(XMTPT_SEND_STATUS)	// request status
//		CConsoleHelper::LibUsb_ReceiveData()					// parse the status
//		CConsoleHelper::DppStatusString							// display status string
void GetDppStatus()
{
	if (chdpp.LibUsb_isConnected) { // send and receive status
		cout << endl;
		cout << "\tRequesting Status..." << endl;
		if (chdpp.LibUsb_SendCommand(XMTPT_SEND_STATUS)) {	// request status
			cout << "\t\tStatus sent." << endl;
			cout << "\t\tReceiving status..." << endl;
			if (chdpp.LibUsb_ReceiveData()) {
				cout << "\t\t\tStatus received..." << endl;
				cout << chdpp.DppStatusString << endl;
				bRunSpectrumTest = true;
				bHaveStatusResponse = true;
				bRunConfigurationTest = true;
			} else {
				cout << "\t\tError receiving status." << endl;
			}
		} else {
			cout << "\t\tError sending status." << endl;
		}
	}
}

// Read Full DPP Configuration From Hardware			// request status before sending/receiving configurations
//		CONFIG_OPTIONS									// holds configuration command options
//		CConsoleHelper::CreateConfigOptions				// creates configuration options from last status read
//      CConsoleHelper::ClearConfigReadFormatFlags();	// clear configuration format flags, for cfg readback
//      CConsoleHelper::CfgReadBack = true;				// requesting general readback format
//		CConsoleHelper::LibUsb_SendCommand_Config		// send command with options
//		CConsoleHelper::LibUsb_ReceiveData()			// parse the configuration
//		CConsoleHelper::HwCfgReady						// config is ready
void ReadDppConfigurationFromHardware(bool bDisplayCfg)
{
	CONFIG_OPTIONS CfgOptions;
	if (bHaveStatusResponse && bRunConfigurationTest) {
		//test configuration functions
		// Set options for XMTPT_FULL_READ_CONFIG_PACKET
		chdpp.CreateConfigOptions(&CfgOptions, "", chdpp.DP5Stat, false);
		cout << endl;
		cout << "\tRequesting Full Configuration..." << endl;
		chdpp.ClearConfigReadFormatFlags();	// clear all flags, set flags only for specific readback properties
		//chdpp.DisplayCfg = false;	// DisplayCfg format overrides general readback format
		chdpp.CfgReadBack = true;	// requesting general readback format
		if (chdpp.LibUsb_SendCommand_Config(XMTPT_FULL_READ_CONFIG_PACKET, CfgOptions)) {	// request full configuration
			if (chdpp.LibUsb_ReceiveData()) {
				if (chdpp.HwCfgReady) {		// config is ready
					bHaveConfigFromHW = true;
					if (bDisplayCfg) {
						cout << "\t\t\tConfiguration Length: " << (unsigned int)chdpp.HwCfgDP5.length() << endl;
						cout << "\t================================================================" << endl;
						cout << chdpp.HwCfgDP5 << endl;
						cout << "\t================================================================" << endl;
						cout << "\t\t\tScroll up to see configuration settings." << endl;
						cout << "\t================================================================" << endl;
					} else {
						cout << "\t\tFull configuration received." << endl;
					}
				}
			}
		}
	}
}

// Display Preset Settings
//		CConsoleHelper::strPresetCmd	// preset mode
//		CConsoleHelper::strPresetVal	// preset setting
void DisplayPresets()
{
	if (bHaveConfigFromHW) {
		cout << "\t\t\tPreset Mode: " << chdpp.strPresetCmd << endl;
		cout << "\t\t\tPreset Settings: " << chdpp.strPresetVal << endl;
	}
}

// Display Preset Settings
//		CONFIG_OPTIONS								// holds configuration command options
//		CConsoleHelper::CreateConfigOptions			// creates configuration options from last status read
//		CConsoleHelper::HwCfgDP5Out					// preset setting
//		CConsoleHelper::LibUsb_SendCommand_Config	// send command with options
void SendPresetAcquisitionTime(string strPRET)
{
	CONFIG_OPTIONS CfgOptions;
	cout << "\tSetting Preset Acquisition Time..." << strPRET << endl;
	chdpp.CreateConfigOptions(&CfgOptions, "", chdpp.DP5Stat, false);
	CfgOptions.HwCfgDP5Out = strPRET;
	// send PresetAcquisitionTime string, bypass any filters, read back the mode and settings
	if (chdpp.LibUsb_SendCommand_Config(XMTPT_SEND_CONFIG_PACKET_EX, CfgOptions)) {
		ReadDppConfigurationFromHardware(false);	// read setting back
		DisplayPresets();							// display new presets
	} else {
		cout << "\t\tPreset Acquisition Time NOT SET" << strPRET << endl;
	}
}

// Acquire Spectrum
//		CConsoleHelper::LibUsb_SendCommand(XMTPT_DISABLE_MCA_MCS)		//disable for data/status clear
//		CConsoleHelper::LibUsb_SendCommand(XMTPT_SEND_CLEAR_SPECTRUM_STATUS)  //clear spectrum/status
//		CConsoleHelper::LibUsb_SendCommand(XMTPT_ENABLE_MCA_MCS);		// enabling MCA for spectrum acquisition
//		CConsoleHelper::LibUsb_SendCommand(XMTPT_SEND_SPECTRUM_STATUS)) // request spectrum+status
//		CConsoleHelper::LibUsb_ReceiveData()							// process spectrum and data
//		CConsoleHelper::ConsoleGraph()	(low resolution display)		// graph data on console with status
//		CConsoleHelper::LibUsb_SendCommand(XMTPT_DISABLE_MCA_MCS)		// disable mca after acquisition
void AcquireSpectrum()
{
	int MaxMCA = 11;
	bool bDisableMCA;

	//bRunSpectrumTest = false;		// disable test
	if (bRunSpectrumTest) {
		cout << "\tRunning spectrum test..." << endl;
		cout << "\t\tDisabling MCA for spectrum data/status clear." << endl;
		chdpp.LibUsb_SendCommand(XMTPT_DISABLE_MCA_MCS);
		Sleep(1000);
		cout << "\t\tClearing spectrum data/status." << endl;
		chdpp.LibUsb_SendCommand(XMTPT_SEND_CLEAR_SPECTRUM_STATUS);
		Sleep(1000);
		cout << "\t\tEnabling MCA for spectrum data acquisition with status ." << endl;
		chdpp.LibUsb_SendCommand(XMTPT_ENABLE_MCA_MCS);
		Sleep(1000);
		for(int idxSpectrum=0;idxSpectrum<MaxMCA;idxSpectrum++) {
			//cout << "\t\tAcquiring spectrum data set " << (idxSpectrum+1) << " of " << MaxMCA << endl;
			if (chdpp.LibUsb_SendCommand(XMTPT_SEND_SPECTRUM_STATUS)) {	// request spectrum+status
				if (chdpp.LibUsb_ReceiveData()) {
					bDisableMCA = true;				// we are aquiring data, disable mca when done
					system(CLEAR_TERM);
					chdpp.ConsoleGraph(chdpp.DP5Proto.SPECTRUM.DATA,chdpp.DP5Proto.SPECTRUM.CHANNELS,true,chdpp.DppStatusString);
					Sleep(2000);
				}
			} else {
				cout << "\t\tProblem acquiring spectrum." << endl;
				break;
			}
		}
		if (bDisableMCA) {
			////system("Pause");
			//cout << "\t\tSpectrum acquisition with status done. Disabling MCA." << endl;
			chdpp.LibUsb_SendCommand(XMTPT_DISABLE_MCA_MCS);
			Sleep(1000);
		}
	}
}

// Read Configuration File
//		CConsoleHelper::SndCmd.GetDP5CfgStr("PX5_Console_Test.txt");
void ReadConfigFile()
{
	std::string strCfg;
	strCfg = chdpp.SndCmd.GetDP5CfgStr("PX5_Console_Test.txt");
	cout << "\t\t\tConfiguration Length: " << (unsigned int)strCfg.length() << endl;
	cout << "\t================================================================" << endl;
	cout << strCfg << endl;
	cout << "\t================================================================" << endl;
}

// Close Connection
//		CConsoleHelper::LibUsb_isConnected			// LibUsb DPP connection indicator
//		CConsoleHelper::LibUsb_Close_Connection()	// close connection
void CloseConnection()
{
	if (chdpp.LibUsb_isConnected) { // send and receive status
		cout << endl;
		cout << "\tClosing connection to default LibUsb device..." << endl;
		chdpp.LibUsb_Close_Connection();
		cout << "\t\tDPP device connection closed." << endl;
	}
}

int main(int argc, char* argv[])
{
	system(CLEAR_TERM);
	ConnectToDefaultDPP();
	cout << "Press the Enter key to continue . . .";
	_getch();

	system(CLEAR_TERM);
	GetDppStatus();
	cout << "Press the Enter key to continue . . .";
	_getch();

	system(CLEAR_TERM);
	ReadDppConfigurationFromHardware(true);
	cout << "Press the Enter key to continue . . .";
	_getch(); 

	system(CLEAR_TERM);
	DisplayPresets();
	cout << "Press the Enter key to continue . . .";
	_getch(); 

	system(CLEAR_TERM);
	SendPresetAcquisitionTime("PRET=20;");
	cout << "Press the Enter key to continue . . .";
	_getch(); 

	system(CLEAR_TERM);
	AcquireSpectrum();
	cout << "Press the Enter key to continue . . .";
	_getch(); 

	system(CLEAR_TERM);
	SendPresetAcquisitionTime("PRET=OFF;");
	cout << "Press the Enter key to continue . . .";
	_getch(); 

	system(CLEAR_TERM);
	ReadConfigFile();
	cout << "Press the Enter key to continue . . .";
	_getch(); 

	system(CLEAR_TERM);
	CloseConnection();
	cout << "Press the Enter key to continue . . .";
	_getch(); 

	return 0;
}





















