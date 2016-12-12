///////////////////////////////////////////////////////////////
// spiprocess3bandeqwin32.cpp : Defines the entry point for the application.
//
//3 band equaliser
//
//by stephane.poirier@oifii.org
//
//ref.
//http://www.musicdsp.org/archive.php?classid=3#258
//spinote: the id may not point to the proper archive, use "3 band equaliser"
///////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "spiprocess3bandeqwin32.h"
#include "FreeImage.h"
#include <shellapi.h> //for CommandLineToArgW()
#include <mmsystem.h> //for timeSetEvent()
#include <stdio.h> //for swprintf()
#include <assert.h>
#include "spiwavsetlib.h"

#include "portaudio.h"

#ifdef WIN32
#if PA_USE_ASIO
#include "pa_asio.h"
#endif
#endif

#include <string>
#include <map>
using namespace std;

#define SAMPLE_RATE  (44100)
//#define FRAMES_PER_BUFFER (64) 
#define FRAMES_PER_BUFFER (640)
//#define FRAMES_PER_BUFFER (2048) 
//#define NUM_CHANNELS    (1)
#define NUM_CHANNELS    (2)

// Select sample format. 
#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif


map<string,int> global_inputdevicemap;
map<string,int> global_outputdevicemap;

PaStream* global_stream;
PaStreamParameters global_inputParameters;
PaStreamParameters global_outputParameters;
PaError global_err;
string global_audioinputdevicename="";
string global_audiooutputdevicename="";
int global_inputAudioChannelSelectors[2];
int global_outputAudioChannelSelectors[2];
PaAsioStreamInfo global_asioInputInfo;
PaAsioStreamInfo global_asioOutputInfo;

FILE* pFILE= NULL;

// Global Variables:

#define MAX_LOADSTRING 100
FIBITMAP* global_dib;
HFONT global_hFont;
HWND global_hwnd=NULL;
MMRESULT global_timer=0;
//#define MAX_GLOBALTEXT	4096
//WCHAR global_text[MAX_GLOBALTEXT+1];
//int global_delay_ms=5000; //default to 5 seconds delay
float global_lowfreq_hz=880.0f; //default to 880.0 Hz
float global_hifreq_hz=5000.0f; //default to 5000.0 Hz
float global_duration_sec=180;
float global_lowgain=1.5f; //boost bass by 50%
float global_midgain=0.75f; //cut mid by 25%
float global_higain=1.0f; //leave high band alone
int global_x=100;
int global_y=200;
int global_xwidth=400;
int global_yheight=400;
BYTE global_alpha=200;
int global_fontheight=24;
int global_fontwidth=-1; //will be computed within WM_PAINT handler
int global_staticalignment = 0; //0 for left, 1 for center and 2 for right
int global_staticheight=-1; //will be computed within WM_SIZE handler
int global_staticwidth=-1; //will be computed within WM_SIZE handler 
//COLORREF global_statictextcolor=RGB(0xFF, 0xFF, 0xFF); //white
COLORREF global_statictextcolor=RGB(0xFF, 0x00, 0x00); //red
//spi, begin
int global_imageheight=-1; //will be computed within WM_SIZE handler
int global_imagewidth=-1; //will be computed within WM_SIZE handler 
//spi, end
int global_titlebardisplay=1; //0 for off, 1 for on
int global_acceleratoractive=0; //0 for off, 1 for on
int global_menubardisplay=0; //0 for off, 1 for on

DWORD global_startstamp_ms;
//FILE* global_pFILE=NULL;
string global_line;
std::ifstream global_ifstream;

#define IDC_MAIN_EDIT	100
#define IDC_MAIN_STATIC	101

HINSTANCE hInst;								// current instance
//TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
//TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szTitle[1024]={L"spiprocess3bandeqwin32title"};					// The title bar text
TCHAR szWindowClass[1024]={L"spiprocess3bandeqwin32class"};				// the main window class name

//new parameters
int global_statictextcolor_red=255;
int global_statictextcolor_green=0;
int global_statictextcolor_blue=0;
string global_begin="begin.ahk";
string global_end="end.ahk";


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

bool global_abort=false;

/*
float CubicAmplifier( float input );
static int fuzzCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData );

//Non-linear amplifier with soft distortion curve.
float CubicAmplifier( float input )
{
    float output, temp;
    if( input < 0.0 )
    {
        temp = input + 1.0f;
        output = (temp * temp * temp) - 1.0f;
    }
    else
    {
        temp = input - 1.0f;
        output = (temp * temp * temp) + 1.0f;
    }

    return output;
}
#define FUZZ(x) CubicAmplifier(CubicAmplifier(CubicAmplifier(CubicAmplifier(x))))
*/

/*
// delayline.c 
//const int M = SAMPLE_RATE*global_delay_ms/1000; 
//static float D[M];           // initialized to zero

int M = SAMPLE_RATE*global_delay_ms/1000;
static float D[SAMPLE_RATE*60];           // initialized to zero
static long ptr=0;            // read-write offset

float delayline(float x)
{
	float y = D[ptr];          // read operation 
	D[ptr++] = x;               // write operation
	if (ptr >= M) { ptr -= M; } // wrap ptr if needed
	//    ptr %= M;                   // modulo-operator syntax
	return y;
}
*/
/*
//lowpass filter
float a0 = 0.0f;
float a1 = 0.0f;
float b1 = 0.0f;
*/
/*
void SetLPF(float fCut, float fSampling)
{
	a0 = fCut/(fSampling+fCut);
	a1 = a0;
	b1 = (fSampling-fCut)/(fSampling+fCut);
}
*/
/*
void SetLPF(float fCut, float fSampling)
{
	float w = 2.0 * fSampling;
	fCut *= 2.0f *3.14159265359;
	float Norm = 1.0/(fCut+w);
	a0 = fCut * Norm;
	a1 = a0;
	b1 = (w - fCut) * Norm;
}
*/
#define M_PI 3.14159265358979323846
//----------------------------------------------------------------------------
//
//                                3 Band EQ :)
//
// EQ.C - Main Source file for 3 band EQ
//
// (c) Neil C / Etanza Systems / 2K6
//
// Shouts / Loves / Moans = etanza at lycos dot co dot uk 
//
// This work is hereby placed in the public domain for all purposes, including
// use in commercial applications.
//
// The author assumes NO RESPONSIBILITY for any problems caused by the use of
// this software.
//
//----------------------------------------------------------------------------

// NOTES :
//
// - Original filter code by Paul Kellet (musicdsp.pdf)
//
// - Uses 4 first order filters in series, should give 24dB per octave
//
// - Now with P4 Denormal fix :)


//----------------------------------------------------------------------------

// ----------
//| Includes |
// ----------

#include <math.h>
//#include "eq.h"


// -----------
//| Constants |
// -----------

static double vsa = (1.0 / 4294967295.0);   // Very small amount (Denormal Fix)


// ---------------
//| Initialise EQ |
// ---------------

// Recommended frequencies are ...
//
//  lowfreq  = 880  Hz
//  highfreq = 5000 Hz
//
// Set mixfreq to whatever rate your system is using (eg 48Khz)

void init_3band_state(EQSTATE* es, int lowfreq, int highfreq, int mixfreq)
{
  // Clear state 

  memset(es,0,sizeof(EQSTATE));

  // Set Low/Mid/High gains to unity

  es->lg = 1.0;
  es->mg = 1.0;
  es->hg = 1.0;

  // Calculate filter cutoff frequencies

  es->lf = 2 * sin(M_PI * ((double)lowfreq / (double)mixfreq)); 
  es->hf = 2 * sin(M_PI * ((double)highfreq / (double)mixfreq));
}


// ---------------
//| EQ one sample |
// ---------------

// - sample can be any range you like :)
//
// Note that the output will depend on the gain settings for each band 
// (especially the bass) so may require clipping before output, but you 
// knew that anyway :)

double do_3band(EQSTATE* es, double sample)
{
  // Locals

  double  l,m,h;      // Low / Mid / High - Sample Values

  // Filter #1 (lowpass)

  es->f1p0  += (es->lf * (sample   - es->f1p0)) + vsa;
  es->f1p1  += (es->lf * (es->f1p0 - es->f1p1));
  es->f1p2  += (es->lf * (es->f1p1 - es->f1p2));
  es->f1p3  += (es->lf * (es->f1p2 - es->f1p3));

  l          = es->f1p3;

  // Filter #2 (highpass)
  
  es->f2p0  += (es->hf * (sample   - es->f2p0)) + vsa;
  es->f2p1  += (es->hf * (es->f2p0 - es->f2p1));
  es->f2p2  += (es->hf * (es->f2p1 - es->f2p2));
  es->f2p3  += (es->hf * (es->f2p2 - es->f2p3));

  h          = es->sdm3 - es->f2p3;

  // Calculate midrange (signal - (low + high))

  m          = es->sdm3 - (h + l);
  //m          = sample - (h + l);

  // Scale, Combine and store

  l         *= es->lg;
  m         *= es->mg;
  h         *= es->hg;

  // Shuffle history buffer 

  es->sdm3   = es->sdm2;
  es->sdm2   = es->sdm1;
  es->sdm1   = sample;                

  // Return result

  return(l + m + h);
}


//----------------------------------------------------------------------------

EQSTATE eq;
static int eq3bandCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData );

static int gNumNoInputs = 0;
// This routine will be called by the PortAudio engine when audio is needed.
// It may be called at interrupt level on some machines so don't do anything
// that could mess up the system like calling malloc() or free().
//
static int eq3bandCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
    SAMPLE *out = (SAMPLE*)outputBuffer;
    const SAMPLE *in = (const SAMPLE*)inputBuffer;
    unsigned int i;
    (void) timeInfo; // Prevent unused variable warnings.
    (void) statusFlags;
    (void) userData;

	if(global_abort==true) return paAbort;

	if( inputBuffer == NULL )
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            *out++ = 0;  // left - silent
            *out++ = 0;  // right - silent 
        }
        gNumNoInputs += 1;
    }
    else
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
			/*
            *out++ = FUZZ(*in++);  // left - distorted 
            *out++ = FUZZ(*in++);  // right - distorted
			*/
            /*
			*out++ = *in++;          // left - clean 
            *out++ = *in++;          // right - clean 
			*/
			
            *out++ = do_3band(&eq, *in++);          // left - EQed 
            *out++ = do_3band(&eq, *in++);          // right - EQed
			
        }
    }
    
    return paContinue;
}





bool SelectAudioInputDevice()
{
	const PaDeviceInfo* deviceInfo;
    int numDevices = Pa_GetDeviceCount();
    for( int i=0; i<numDevices; i++ )
    {
        deviceInfo = Pa_GetDeviceInfo( i );
		string devicenamestring = deviceInfo->name;
		global_inputdevicemap.insert(pair<string,int>(devicenamestring,i));
		if(pFILE) fprintf(pFILE,"id=%d, name=%s\n", i, devicenamestring.c_str());
	}

	int deviceid = Pa_GetDefaultInputDevice(); // default input device 
	map<string,int>::iterator it;
	it = global_inputdevicemap.find(global_audioinputdevicename);
	if(it!=global_inputdevicemap.end())
	{
		deviceid = (*it).second;
		//printf("%s maps to %d\n", global_audiodevicename.c_str(), deviceid);
		deviceInfo = Pa_GetDeviceInfo(deviceid);
		//assert(inputAudioChannelSelectors[0]<deviceInfo->maxInputChannels);
		//assert(inputAudioChannelSelectors[1]<deviceInfo->maxInputChannels);
	}
	else
	{
		//Pa_Terminate();
		//return -1;
		//printf("error, audio device not found, will use default\n");
		//MessageBox(win,"error, audio device not found, will use default\n",0,0);
		deviceid = Pa_GetDefaultInputDevice();
	}


	global_inputParameters.device = deviceid; 
	if (global_inputParameters.device == paNoDevice) 
	{
		//MessageBox(win,"error, no default input device.\n",0,0);
		return false;
	}
	//global_inputParameters.channelCount = 2;
	global_inputParameters.channelCount = NUM_CHANNELS;
	global_inputParameters.sampleFormat =  PA_SAMPLE_TYPE;
	global_inputParameters.suggestedLatency = Pa_GetDeviceInfo( global_inputParameters.device )->defaultLowOutputLatency;
	//inputParameters.hostApiSpecificStreamInfo = NULL;

	//Use an ASIO specific structure. WARNING - this is not portable. 
	//PaAsioStreamInfo asioInputInfo;
	global_asioInputInfo.size = sizeof(PaAsioStreamInfo);
	global_asioInputInfo.hostApiType = paASIO;
	global_asioInputInfo.version = 1;
	global_asioInputInfo.flags = paAsioUseChannelSelectors;
	global_asioInputInfo.channelSelectors = global_inputAudioChannelSelectors;
	if(deviceid==Pa_GetDefaultInputDevice())
	{
		global_inputParameters.hostApiSpecificStreamInfo = NULL;
	}
	else if(Pa_GetHostApiInfo(Pa_GetDeviceInfo(deviceid)->hostApi)->type == paASIO) 
	{
		global_inputParameters.hostApiSpecificStreamInfo = &global_asioInputInfo;
	}
	else if(Pa_GetHostApiInfo(Pa_GetDeviceInfo(deviceid)->hostApi)->type == paWDMKS) 
	{
		global_inputParameters.hostApiSpecificStreamInfo = NULL;
	}
	else
	{
		//assert(false);
		global_inputParameters.hostApiSpecificStreamInfo = NULL;
	}
	return true;
}

bool SelectAudioOutputDevice()
{
	const PaDeviceInfo* deviceInfo;
    int numDevices = Pa_GetDeviceCount();
    for( int i=0; i<numDevices; i++ )
    {
        deviceInfo = Pa_GetDeviceInfo( i );
		string devicenamestring = deviceInfo->name;
		global_outputdevicemap.insert(pair<string,int>(devicenamestring,i));
		if(pFILE) fprintf(pFILE,"id=%d, name=%s\n", i, devicenamestring.c_str());
	}

	int deviceid = Pa_GetDefaultOutputDevice(); // default output device 
	map<string,int>::iterator it;
	it = global_outputdevicemap.find(global_audiooutputdevicename);
	if(it!=global_outputdevicemap.end())
	{
		deviceid = (*it).second;
		//printf("%s maps to %d\n", global_audiodevicename.c_str(), deviceid);
		deviceInfo = Pa_GetDeviceInfo(deviceid);
		//assert(inputAudioChannelSelectors[0]<deviceInfo->maxInputChannels);
		//assert(inputAudioChannelSelectors[1]<deviceInfo->maxInputChannels);
	}
	else
	{
		//Pa_Terminate();
		//return -1;
		//printf("error, audio device not found, will use default\n");
		//MessageBox(win,"error, audio device not found, will use default\n",0,0);
		deviceid = Pa_GetDefaultOutputDevice();
	}


	global_outputParameters.device = deviceid; 
	if (global_outputParameters.device == paNoDevice) 
	{
		//MessageBox(win,"error, no default output device.\n",0,0);
		return false;
	}
	//global_inputParameters.channelCount = 2;
	global_outputParameters.channelCount = NUM_CHANNELS;
	global_outputParameters.sampleFormat =  PA_SAMPLE_TYPE;
	global_outputParameters.suggestedLatency = Pa_GetDeviceInfo( global_outputParameters.device )->defaultLowOutputLatency;
	//outputParameters.hostApiSpecificStreamInfo = NULL;

	//Use an ASIO specific structure. WARNING - this is not portable. 
	//PaAsioStreamInfo asioInputInfo;
	global_asioOutputInfo.size = sizeof(PaAsioStreamInfo);
	global_asioOutputInfo.hostApiType = paASIO;
	global_asioOutputInfo.version = 1;
	global_asioOutputInfo.flags = paAsioUseChannelSelectors;
	global_asioOutputInfo.channelSelectors = global_outputAudioChannelSelectors;
	if(deviceid==Pa_GetDefaultOutputDevice())
	{
		global_outputParameters.hostApiSpecificStreamInfo = NULL;
	}
	else if(Pa_GetHostApiInfo(Pa_GetDeviceInfo(deviceid)->hostApi)->type == paASIO) 
	{
		global_outputParameters.hostApiSpecificStreamInfo = &global_asioOutputInfo;
	}
	else if(Pa_GetHostApiInfo(Pa_GetDeviceInfo(deviceid)->hostApi)->type == paWDMKS) 
	{
		global_outputParameters.hostApiSpecificStreamInfo = NULL;
	}
	else
	{
		//assert(false);
		global_outputParameters.hostApiSpecificStreamInfo = NULL;
	}
	return true;
}



// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte                  (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar                  (CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void CALLBACK StartGlobalProcess(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	WavSetLib_Initialize(global_hwnd, IDC_MAIN_STATIC, global_staticwidth, global_staticheight, global_fontwidth, global_fontheight, global_staticalignment);

	DWORD nowstamp_ms = GetTickCount();
	while( (global_duration_sec<0.0f) || ((nowstamp_ms-global_startstamp_ms)/1000.0f)<global_duration_sec )
	{
		Sleep((int)(1*1000));
		
		/*
		std::getline(global_ifstream, global_line);
		if(global_ifstream.eof()) 
		{
			global_ifstream.close();
			global_ifstream.open(global_filename.c_str(), ios_base::in);
			global_line="";
		}
		global_line += "\n";
		StatusAddText(global_line.c_str());
		*/

		/*
		eq.lg = 0.1;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.2;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.3;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.4;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.5;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.6;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.7;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.8;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.9;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 1.0;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));

		eq.lg = 0.9;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.8;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.7;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.6;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.5;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.4;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.3;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.2;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.1;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));

		eq.lg = 0.0;
		eq.mg = 0.1;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.2;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.3;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.4;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.5;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.6;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.7;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.8;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.9;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 1.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));

		eq.lg = 0.0;
		eq.mg = 0.9;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.8;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.7;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.6;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.5;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.4;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.3;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.2;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.1;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		eq.lg = 0.0;
		eq.mg = 0.0;
		eq.hg = 0.0;
		Sleep((int)(1*1000));
		*/

		nowstamp_ms = GetTickCount();
	}
	PostMessage(global_hwnd, WM_DESTROY, 0, 0);
}




PCHAR*
    CommandLineToArgvA(
        PCHAR CmdLine,
        int* _argc
        )
    {
        PCHAR* argv;
        PCHAR  _argv;
        ULONG   len;
        ULONG   argc;
        CHAR   a;
        ULONG   i, j;

        BOOLEAN  in_QM;
        BOOLEAN  in_TEXT;
        BOOLEAN  in_SPACE;

        len = strlen(CmdLine);
        i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

        argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
            i + (len+2)*sizeof(CHAR));

        _argv = (PCHAR)(((PUCHAR)argv)+i);

        argc = 0;
        argv[argc] = _argv;
        in_QM = FALSE;
        in_TEXT = FALSE;
        in_SPACE = TRUE;
        i = 0;
        j = 0;

        while( a = CmdLine[i] ) {
            if(in_QM) {
                if(a == '\"') {
                    in_QM = FALSE;
                } else {
                    _argv[j] = a;
                    j++;
                }
            } else {
                switch(a) {
                case '\"':
                    in_QM = TRUE;
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    in_SPACE = FALSE;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if(in_TEXT) {
                        _argv[j] = '\0';
                        j++;
                    }
                    in_TEXT = FALSE;
                    in_SPACE = TRUE;
                    break;
                default:
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
                }
            }
            i++;
        }
        _argv[j] = '\0';
        argv[argc] = NULL;

        (*_argc) = argc;
        return argv;
    }

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	global_startstamp_ms = GetTickCount();

	//LPWSTR *szArgList;
	LPSTR *szArgList;
	int nArgs;
	int i;

	//szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	szArgList = CommandLineToArgvA(GetCommandLineA(), &nArgs);
	if( NULL == szArgList )
	{
		//wprintf(L"CommandLineToArgvW failed\n");
		return FALSE;
	}
	LPWSTR *szArgListW;
	int nArgsW;
	szArgListW = CommandLineToArgvW(GetCommandLineW(), &nArgsW);
	if( NULL == szArgListW )
	{
		//wprintf(L"CommandLineToArgvW failed\n");
		return FALSE;
	}

	global_audioinputdevicename="E-MU ASIO"; //"Wave (2- E-MU E-DSP Audio Proce"
	if(nArgs>1)
	{
		//global_filename = szArgList[1];
		global_audioinputdevicename = szArgList[1]; 
	}
	global_inputAudioChannelSelectors[0] = 0; // on emu patchmix ASIO device channel 1 (left)
	global_inputAudioChannelSelectors[1] = 1; // on emu patchmix ASIO device channel 2 (right)
	//global_inputAudioChannelSelectors[0] = 2; // on emu patchmix ASIO device channel 3 (left)
	//global_inputAudioChannelSelectors[1] = 3; // on emu patchmix ASIO device channel 4 (right)
	//global_inputAudioChannelSelectors[0] = 8; // on emu patchmix ASIO device channel 9 (left)
	//global_inputAudioChannelSelectors[1] = 9; // on emu patchmix ASIO device channel 10 (right)
	//global_inputAudioChannelSelectors[0] = 10; // on emu patchmix ASIO device channel 11 (left)
	//global_inputAudioChannelSelectors[1] = 11; // on emu patchmix ASIO device channel 12 (right)
	if(nArgs>2)
	{
		global_inputAudioChannelSelectors[0]=atoi((LPCSTR)(szArgList[2])); //0 for first asio channel (left) or 2, 4, 6, etc.
	}
	if(nArgs>3)
	{
		global_inputAudioChannelSelectors[1]=atoi((LPCSTR)(szArgList[3])); //1 for second asio channel (right) or 3, 5, 7, etc.
	}
	global_audiooutputdevicename="E-MU ASIO"; //"Wave (2- E-MU E-DSP Audio Proce"
	if(nArgs>4)
	{
		//global_filename = szArgList[1];
		global_audiooutputdevicename = szArgList[4]; 
	}
	global_outputAudioChannelSelectors[0] = 0; // on emu patchmix ASIO device channel 1 (left)
	global_outputAudioChannelSelectors[1] = 1; // on emu patchmix ASIO device channel 2 (right)
	//global_outputAudioChannelSelectors[0] = 2; // on emu patchmix ASIO device channel 3 (left)
	//global_outputAudioChannelSelectors[1] = 3; // on emu patchmix ASIO device channel 4 (right)
	//global_outputAudioChannelSelectors[0] = 8; // on emu patchmix ASIO device channel 9 (left)
	//global_outputAudioChannelSelectors[1] = 9; // on emu patchmix ASIO device channel 10 (right)
	//global_outputAudioChannelSelectors[0] = 10; // on emu patchmix ASIO device channel 11 (left)
	//global_outputAudioChannelSelectors[1] = 11; // on emu patchmix ASIO device channel 12 (right)
	if(nArgs>5)
	{
		global_outputAudioChannelSelectors[0]=atoi((LPCSTR)(szArgList[5])); //0 for first asio channel (left) or 2, 4, 6, etc.
	}
	if(nArgs>6)
	{
		global_outputAudioChannelSelectors[1]=atoi((LPCSTR)(szArgList[6])); //1 for second asio channel (right) or 3, 5, 7, etc.
	}

	
	if(nArgs>7)
	{
		global_lowfreq_hz = atof(szArgList[7]); 
	}
	if(nArgs>8)
	{
		global_hifreq_hz = atof(szArgList[8]); 
	}
	if(nArgs>9)
	{
		global_lowgain = atof(szArgList[9]); 
	}
	if(nArgs>10)
	{
		global_midgain = atof(szArgList[10]); 
	}
	if(nArgs>11)
	{
		global_higain = atof(szArgList[11]); 
	}


	if(nArgs>12)
	{
		global_duration_sec = atof(szArgList[12]);
	}


	if(nArgs>13)
	{
		global_x = atoi(szArgList[13]);
	}
	if(nArgs>14)
	{
		global_y = atoi(szArgList[14]);
	}
	if(nArgs>15)
	{
		global_xwidth = atoi(szArgList[15]);
	}
	if(nArgs>16)
	{
		global_yheight = atoi(szArgList[16]);
	}
	if(nArgs>17)
	{
		global_alpha = atoi(szArgList[17]);
	}
	if(nArgs>18)
	{
		global_titlebardisplay = atoi(szArgList[18]);
	}
	if(nArgs>19)
	{
		global_menubardisplay = atoi(szArgList[19]);
	}
	if(nArgs>20)
	{
		global_acceleratoractive = atoi(szArgList[20]);
	}
	if(nArgs>21)
	{
		global_fontheight = atoi(szArgList[21]);
	}

	//new parameters
	if(nArgs>22)
	{
		global_statictextcolor_red = atoi(szArgList[22]);
	}
	if(nArgs>23)
	{
		global_statictextcolor_green = atoi(szArgList[23]);
	}
	if(nArgs>24)
	{
		global_statictextcolor_blue = atoi(szArgList[24]);
	}
	if(nArgs>25)
	{
		wcscpy(szWindowClass, szArgListW[25]); 
	}
	if(nArgs>26)
	{
		wcscpy(szTitle, szArgListW[26]); 
	}
	if(nArgs>27)
	{
		global_begin = szArgList[27]; 
	}
	if(nArgs>28)
	{
		global_end = szArgList[28]; 
	}
	global_statictextcolor=RGB(global_statictextcolor_red, global_statictextcolor_green, global_statictextcolor_blue);
	LocalFree(szArgList);
	LocalFree(szArgListW);

	int nShowCmd = false;
	//ShellExecuteA(NULL, "open", "begin.bat", "", NULL, nShowCmd);
	ShellExecuteA(NULL, "open", global_begin.c_str(), "", NULL, nCmdShow);


	pFILE = fopen("devices.txt","w");
	///////////////////////
	//initialize port audio
	///////////////////////
    global_err = Pa_Initialize();
    if( global_err != paNoError )
	{
		//MessageBox(0,"portaudio initialization failed",0,MB_ICONERROR);
		if(pFILE) fprintf(pFILE, "portaudio initialization failed.\n");
		fclose(pFILE);
		return 1;
	}

	////////////////////////
	//audio device selection
	////////////////////////
	SelectAudioInputDevice();
	SelectAudioOutputDevice();

	////////////////
	//init 3 band eq
	////////////////
	init_3band_state(&eq,global_lowfreq_hz,global_hifreq_hz,SAMPLE_RATE);
	eq.lg = global_lowgain;
	eq.mg = global_midgain;
	eq.hg = global_higain;

	//////////////
    //setup stream  
	//////////////
    global_err = Pa_OpenStream(
        &global_stream,
        &global_inputParameters,
        &global_outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        0, //paClipOff,      // we won't output out of range samples so don't bother clipping them
        eq3bandCallback,  
        NULL ); //no callback userData
    if( global_err != paNoError ) 
	{
		char errorbuf[2048];
        sprintf(errorbuf, "Unable to open stream: %s\n", Pa_GetErrorText(global_err));
		//MessageBox(0,errorbuf,0,MB_ICONERROR);
		if(pFILE) fprintf(pFILE, "%s\n", errorbuf);
		fclose(pFILE);
        return 1;
    }

	//////////////
    //start stream  
	//////////////
    global_err = Pa_StartStream( global_stream );
    if( global_err != paNoError ) 
	{
		char errorbuf[2048];
        sprintf(errorbuf, "Unable to start stream: %s\n", Pa_GetErrorText(global_err));
		//MessageBox(0,errorbuf,0,MB_ICONERROR);
		if(pFILE) fprintf(pFILE, "%s\n", errorbuf);
        fclose(pFILE);
		return 1;
    }

	fclose(pFILE);
	pFILE=NULL;


	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_SPIWAVWIN32, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	if(global_acceleratoractive)
	{
		hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPIWAVWIN32));
	}
	else
	{
		hAccelTable = NULL;
	}
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	//wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPIWAVWIN32));
	wcex.hIcon			= (HICON)LoadImage(NULL, L"background_32x32x16.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);

	if(global_menubardisplay)
	{
		wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SPIWAVWIN32); //original with menu
	}
	else
	{
		wcex.lpszMenuName = NULL; //no menu
	}
	wcex.lpszClassName	= szWindowClass;
	//wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	wcex.hIconSm		= (HICON)LoadImage(NULL, L"background_16x16x16.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	global_dib = FreeImage_Load(FIF_JPEG, "background.jpg", JPEG_DEFAULT);

	//global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"SYSTEM_FIXED_FONT");
	global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"Segoe Script");

	if(global_titlebardisplay)
	{
		hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, //original with WS_CAPTION etc.
			global_x, global_y, global_xwidth, global_yheight, NULL, NULL, hInstance, NULL);
	}
	else
	{
		hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP | WS_VISIBLE, //no WS_CAPTION etc.
			global_x, global_y, global_xwidth, global_yheight, NULL, NULL, hInstance, NULL);
	}
	if (!hWnd)
	{
		return FALSE;
	}
	global_hwnd = hWnd;

	SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hWnd, 0, global_alpha, LWA_ALPHA);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	global_timer=timeSetEvent(500,25,(LPTIMECALLBACK)&StartGlobalProcess,0,TIME_ONESHOT);
	return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HGDIOBJ hOldBrush;
	HGDIOBJ hOldPen;
	int iOldMixMode;
	COLORREF crOldBkColor;
	COLORREF crOldTextColor;
	int iOldBkMode;
	HFONT hOldFont, hFont;
	TEXTMETRIC myTEXTMETRIC;

	switch (message)
	{
	case WM_CREATE:
		{
			/*
			HWND hStatic = CreateWindowEx(WS_EX_TRANSPARENT, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,  
				0, 100, 100, 100, hWnd, (HMENU)IDC_MAIN_STATIC, GetModuleHandle(NULL), NULL);
			*/
			HWND hStatic = CreateWindowEx(WS_EX_TRANSPARENT, L"STATIC", L"", WS_CHILD | WS_VISIBLE | global_staticalignment,  
				0, 100, 100, 100, hWnd, (HMENU)IDC_MAIN_STATIC, GetModuleHandle(NULL), NULL);
			if(hStatic == NULL)
				MessageBox(hWnd, L"Could not create static text.", L"Error", MB_OK | MB_ICONERROR);
			SendMessage(hStatic, WM_SETFONT, (WPARAM)global_hFont, MAKELPARAM(FALSE, 0));
		}
		break;
	case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);
			HWND hStatic = GetDlgItem(hWnd, IDC_MAIN_STATIC);
			global_staticwidth = rcClient.right - 0;
			//global_staticheight = rcClient.bottom-(rcClient.bottom/2);
			global_staticheight = rcClient.bottom-0;
			//spi, begin
			global_imagewidth = rcClient.right - 0;
			global_imageheight = rcClient.bottom - 0; 
			WavSetLib_Initialize(global_hwnd, IDC_MAIN_STATIC, global_staticwidth, global_staticheight, global_fontwidth, global_fontheight, global_staticalignment);
			//spi, end
			//SetWindowPos(hStatic, NULL, 0, rcClient.bottom/2, global_staticwidth, global_staticheight, SWP_NOZORDER);
			SetWindowPos(hStatic, NULL, 0, 0, global_staticwidth, global_staticheight, SWP_NOZORDER);
		}
		break;
	case WM_CTLCOLOREDIT:
		{
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetTextColor((HDC)wParam, RGB(0xFF, 0xFF, 0xFF));
			return (INT_PTR)::GetStockObject(NULL_PEN);
		}
		break;
	case WM_CTLCOLORSTATIC:
		{
			SetBkMode((HDC)wParam, TRANSPARENT);
			//SetTextColor((HDC)wParam, RGB(0xFF, 0xFF, 0xFF));
			SetTextColor((HDC)wParam, global_statictextcolor);
			return (INT_PTR)::GetStockObject(NULL_PEN);
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		SetStretchBltMode(hdc, COLORONCOLOR);
		//spi, begin
		/*
		StretchDIBits(hdc, 0, 0, global_xwidth, global_yheight,
						0, 0, FreeImage_GetWidth(global_dib), FreeImage_GetHeight(global_dib),
						FreeImage_GetBits(global_dib), FreeImage_GetInfo(global_dib), DIB_RGB_COLORS, SRCCOPY);
		*/
		StretchDIBits(hdc, 0, 0, global_imagewidth, global_imageheight,
						0, 0, FreeImage_GetWidth(global_dib), FreeImage_GetHeight(global_dib),
						FreeImage_GetBits(global_dib), FreeImage_GetInfo(global_dib), DIB_RGB_COLORS, SRCCOPY);
		//spi, end
		hOldBrush = SelectObject(hdc, (HBRUSH)GetStockObject(GRAY_BRUSH));
		hOldPen = SelectObject(hdc, (HPEN)GetStockObject(WHITE_PEN));
		//iOldMixMode = SetROP2(hdc, R2_MASKPEN);
		iOldMixMode = SetROP2(hdc, R2_MERGEPEN);
		//Rectangle(hdc, 100, 100, 200, 200);

		crOldBkColor = SetBkColor(hdc, RGB(0xFF, 0x00, 0x00));
		crOldTextColor = SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
		iOldBkMode = SetBkMode(hdc, TRANSPARENT);
		//hFont=CreateFontW(70,0,0,0,FW_BOLD,0,0,0,0,0,0,2,0,L"SYSTEM_FIXED_FONT");
		//hOldFont=(HFONT)SelectObject(hdc,global_hFont);
		hOldFont=(HFONT)SelectObject(hdc,global_hFont);
		GetTextMetrics(hdc, &myTEXTMETRIC);
		global_fontwidth = myTEXTMETRIC.tmAveCharWidth;
		//TextOutW(hdc, 100, 100, L"test string", 11);

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		SetROP2(hdc, iOldMixMode);
		SetBkColor(hdc, crOldBkColor);
		SetTextColor(hdc, crOldTextColor);
		SetBkMode(hdc, iOldBkMode);
		SelectObject(hdc,hOldFont);
		//DeleteObject(hFont);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		{
			if (global_timer) timeKillEvent(global_timer);
			global_abort=true;
			WavSetLib_Terminate();
			FreeImage_Unload(global_dib);
			DeleteObject(global_hFont);
			//if(global_pFILE) fclose(global_pFILE);
			global_ifstream.close();

			//int nShowCmd = false;
			//ShellExecuteA(NULL, "open", "end.bat", "", NULL, nShowCmd);
			ShellExecuteA(NULL, "open", global_end.c_str(), "", NULL, 0);
			PostQuitMessage(0);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
