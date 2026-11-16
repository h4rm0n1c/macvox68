#!/usr/bin/env python3
"""Host contract checks using the actual networking and speech source.

Minimal fake Toolbox/OT headers isolate receive-status and stop dispatch.
These checks do not emulate Open Transport or validate the 68k ABI.
"""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1] / "src"
with tempfile.TemporaryDirectory(prefix='macvox-contracts-') as tmp:
 p=Path(tmp)
 (p/'Types.h').write_text('''#include <stddef.h>
#include <stdint.h>
typedef int Boolean;
typedef char **Handle;
typedef long Size;
typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned long UInt32;
typedef int32_t OSStatus;
typedef int32_t OSErr;
typedef uint32_t OSType;
#define true 1
#define false 0
#define noErr 0
''')
 (p/'OpenTransport.h').write_text('''#include <Types.h>
typedef void *EndpointRef;
typedef void *OTConfigurationRef;
typedef uint32_t OTFlags;
typedef int32_t OTResult;
typedef struct {unsigned long maxlen,len; UInt8 *buf;} TNetbuf;
typedef struct {TNetbuf addr; unsigned long qlen;} TBind;
typedef struct {TNetbuf addr;} TCall;
typedef struct {int reason;} TDiscon;
#define kOTNoError 0
#define kOTNoDataErr -3150
#define kOTLookErr -3158
#define T_DISCONNECT 8
void InitOpenTransport(void);
OSStatus OTCloseProvider(EndpointRef);
OTConfigurationRef OTCreateConfiguration(const char *);
EndpointRef OTOpenEndpoint(OTConfigurationRef, long, void *, OSStatus *);
OSStatus OTSetNonBlocking(EndpointRef);
OSStatus OTBind(EndpointRef,TBind *,void *);
OSStatus OTRcvDisconnect(EndpointRef,TDiscon *);
OSStatus OTListen(EndpointRef,TCall *);
OSStatus OTAccept(EndpointRef,EndpointRef,TCall *);
OTResult OTRcv(EndpointRef,void *,long,OTFlags *);
OTResult OTLook(EndpointRef);
''')
 (p/'OpenTptInternet.h').write_text('''typedef struct {unsigned long host; unsigned short port;} InetAddress;
void OTInitInetAddress(InetAddress *,UInt16,UInt32);
''')
 (p/'network_test.c').write_text('''#include "network_backend_ot.c"
#include <assert.h>
static OTResult result, event;
static OTFlags returnedFlags;
static int closed, looked, disconnected, delivered;
void InitOpenTransport(void) {assert(0);}
OTConfigurationRef OTCreateConfiguration(const char *s) {assert(0); return NULL;}
EndpointRef OTOpenEndpoint(OTConfigurationRef c,long x,void *y,OSStatus *s) {assert(0); return NULL;}
OSStatus OTSetNonBlocking(EndpointRef ep) {assert(0); return 0;}
OSStatus OTBind(EndpointRef ep,TBind *b,void *x) {assert(0); return 0;}
OSStatus OTListen(EndpointRef ep,TCall *c) {assert(0); return 0;}
OSStatus OTAccept(EndpointRef a,EndpointRef b,TCall *c) {assert(0); return 0;}
void OTInitInetAddress(InetAddress *a,UInt16 p,UInt32 h) {assert(0);}
OSStatus OTCloseProvider(EndpointRef ep) {++closed; return 0;}
OSStatus OTRcvDisconnect(EndpointRef ep,TDiscon *d) {++disconnected; return 0;}
OTResult OTLook(EndpointRef ep) {++looked; return event;}
OTResult OTRcv(EndpointRef ep,void *buf,long count,OTFlags *flags) {
 *flags=returnedFlags;
 if(result>0) memcpy(buf,"abc",3);
 return result;
}
static void data(const char *s,Size n) {assert(n==3); assert(!strcmp(s,"abc")); ++delivered;}
static void check(OTResult r,OTFlags f,int expectClose,int expectLook) {
 NetworkClient c={(void *)1,true}; result=r; returnedFlags=f;
 closed=looked=disconnected=delivered=0;
 network_poll_client(&c);
 assert(closed==expectClose); assert(looked==expectLook);
 assert(delivered==(r>0)); assert(c.active==!expectClose);
}
int main(void) {
 NetworkCallbacks cb={data,NULL}; gCallbacks=&cb;
 check(3,1,0,0); /* A successful read with T_MORE must not close. */
 check(kOTNoDataErr,0,0,0);
 check(-999,0,1,0); /* Negative status must not be mistaken for flags. */
 event=T_DISCONNECT; check(kOTLookErr,0,1,1); assert(disconnected==1);
 event=0; check(kOTLookErr,0,0,1);
 check(0,0,0,0);
 return 0;
}
''')
 (p/'Gestalt.h').write_text('#include <Types.h>\nOSErr Gestalt(long,long *);\n')
 (p/'Memory.h').write_text("""#include <string.h>
#include <Types.h>
typedef char *Ptr;
#define paramErr -50
#define unimpErr -4
#define memFullErr -108
Ptr NewPtr(long);
void DisposePtr(Ptr);
Handle NewHandle(long);
long GetHandleSize(Handle);
void DisposeHandle(Handle);
void HLock(Handle);
void HUnlock(Handle);
#define BlockMoveData(s,d,n) memcpy(d,s,n)
""")
 (p/'Speech.h').write_text("""#include <Types.h>
typedef int32_t Fixed;
typedef void *SpeechChannel;
typedef struct {char startDelimiter[2],endDelimiter[2];} DelimiterInfo;
typedef struct {Boolean outputBusy,outputPaused;long inputBytesLeft;short phonemeCode;} SpeechStatusInfo;
#define soStatus 1
#define soCommandDelimiter 2
#define soVolume 3
#define soInputMode 4
#define soSynthType 5
#define modeText 0x54455854
#define modePhonemes 0x50484f4e
 typedef struct {long synthManufacturer;} SpeechVersionInfo;
OSErr TextToPhonemes(SpeechChannel,const void *,long,Handle,long *);
OSErr GetSpeechRate(SpeechChannel,Fixed *);
OSErr GetSpeechPitch(SpeechChannel,Fixed *);
OSErr SetSpeechRate(SpeechChannel,Fixed);
OSErr SetSpeechPitch(SpeechChannel,Fixed);
OSErr NewSpeechChannel(void *,SpeechChannel *);
OSErr SetSpeechInfo(SpeechChannel,long,const void *);
OSErr GetSpeechInfo(SpeechChannel,long,void *);
OSErr SpeakText(SpeechChannel,const void *,long);
OSErr StopSpeech(SpeechChannel);
OSErr DisposeSpeechChannel(SpeechChannel);
""")
 (p/'speech_test.c').write_text(r'''#include "speech.c"
#include <assert.h>
#include <stdlib.h>
static int present=1,allocs,busy,allocationFail;
static OSErr statusError,stopError,speakError,newError,setError,disposeError;
static const void *borrowed;
static int commands, activeCommands, nativeProvider, conversions;
static OSType inputMode;
static OSErr conversionError, modeError;
static Fixed volumeValue, rateValue, pitchValue;
static OSErr rateError, pitchError, defaultError;
OSErr GetSpeechRate(SpeechChannel c,Fixed *v) {*v=180*65536;return defaultError;}
OSErr GetSpeechPitch(SpeechChannel c,Fixed *v) {*v=45*65536;return defaultError;}
OSErr SetSpeechRate(SpeechChannel c,Fixed v) {rateValue=v;return rateError;}
OSErr SetSpeechPitch(SpeechChannel c,Fixed v) {pitchValue=v;return pitchError;}
static char observed[4096];
OSErr Gestalt(long s,long *r) {*r=present;return noErr;}
Ptr NewPtr(long n) {if(allocationFail)return NULL;++allocs;return malloc(n);}
void DisposePtr(Ptr p) {assert(p);assert(p!=borrowed||!busy);--allocs;free(p);}
Handle NewHandle(long n) {Handle h=malloc(sizeof(*h));*h=malloc(n);return h;}
void DisposeHandle(Handle h) {free(*h);free(h);}
long GetHandleSize(Handle h) {return 16384;}
void HLock(Handle h) {}
void HUnlock(Handle h) {}
OSErr NewSpeechChannel(void *v,SpeechChannel *c) {if(newError)return newError;*c=(void *)1;return 0;}
OSErr SetSpeechInfo(SpeechChannel c,long selector,const void *v) {
 if(selector==soInputMode){inputMode=*(const OSType*)v;return modeError;}
 if(selector==soVolume){volumeValue=*(const Fixed*)v;return setError;}
 const DelimiterInfo *d=v;assert(selector==soCommandDelimiter);
 activeCommands=d->startDelimiter[0]=='[';
 assert(d->startDelimiter[0]==(activeCommands?'[':0)&&d->startDelimiter[1]==(activeCommands?'[':0)&&d->endDelimiter[0]==(activeCommands?']':0)&&d->endDelimiter[1]==(activeCommands?']':0));return setError;
}
OSErr GetSpeechInfo(SpeechChannel c,long selector,void *v) {
 assert(c);
 if(selector==soSynthType){((SpeechVersionInfo*)v)->synthManufacturer=nativeProvider?'mtk3':0;return 0;}
 assert(selector==soStatus);((SpeechStatusInfo *)v)->outputBusy=busy;return statusError;
}
OSErr TextToPhonemes(SpeechChannel c,const void *text,long n,Handle phon,long *count) {
 assert(c&&n>0&&!activeCommands&&inputMode==modeText);++conversions;
 if(conversionError)return conversionError;
 memcpy(*phon,"_t1EHst.%",9);*count=9;return 0;
}
OSErr SpeakText(SpeechChannel c,const void *p,long n) {assert(c&&p&&n>0);assert(activeCommands==commands&&inputMode==modeText);assert(n<sizeof(observed));memcpy(observed,p,n);observed[n]=0;borrowed=p;busy=1;return speakError;}
OSErr StopSpeech(SpeechChannel c) {assert(c);if(!stopError)busy=0;return stopError;}
OSErr DisposeSpeechChannel(SpeechChannel c) {assert(c);if(!disposeError)busy=0;return disposeError;}
int main(void) {
 char input[1024];memset(input,'A',sizeof(input));
 present=0;speech_init();assert(speech_speak_text(input,1)==unimpErr);assert(!allocs);
 present=1;speech_init();assert(speech_speak_text(NULL,1)==paramErr);assert(speech_speak_text(input,-1)==paramErr);
 assert(speech_speak_text(input,sizeof(input))==0);assert(allocs==1&&borrowed!=input);input[0]='Z';assert(((const char*)borrowed)[0]=='A');
 statusError=-1;assert(speech_is_busy()&&allocs==1);statusError=0;
 stopError=-2;assert(speech_stop()==-2&&allocs==1);assert(speech_speak_text(input,4)==-2&&allocs==1);
 stopError=0;assert(speech_stop()==0&&!allocs);
 assert(speech_speak_text(input,1024)==0);busy=0;assert(!speech_is_busy()&&!allocs); /* completion */
 speakError=-3;assert(speech_speak_text(input,1024)==-3&&!allocs);speakError=0;
 allocationFail=1;assert(speech_speak_text(input,1024)==memFullErr&&!allocs);allocationFail=0;
 assert(speech_speak_text(input,1024)==0);disposeError=-4;assert(speech_shutdown()==-4&&allocs==1);disposeError=0;assert(speech_shutdown()==0&&!allocs&&!gChannel);
 newError=-5;assert(speech_speak_text(input,1)==-5&&!gChannel&&!allocs);newError=0;
 setError=-6;assert(speech_speak_text(input,1)==-6&&!gChannel&&!allocs);setError=0;
 assert(speech_set_settings(-1,100,0)==paramErr);
 assert(speech_set_settings(100,0,0)==paramErr);
 assert(speech_set_settings(100,100,13)==paramErr);
 assert(speech_set_settings(25,150,7)==0);
 assert(speech_speak_text(input,1)==0);
 assert(volumeValue==16384&&rateValue==270*65536&&pitchValue==52*65536);
 assert(speech_stop()==0);
 /* Percentages always use original voice defaults, never compound. */
 assert(speech_speak_text(input,1)==0&&rateValue==270*65536&&pitchValue==52*65536);
 assert(speech_stop()==0);
 rateError=-7;assert(speech_speak_text(input,1)==-7&&!allocs);rateError=0;
 pitchError=-8;assert(speech_speak_text(input,1)==-8&&!allocs);pitchError=0;
 assert(speech_shutdown()==0);
 defaultError=-9;assert(speech_speak_text(input,1)==-9&&!gChannel&&!allocs);defaultError=0;
 assert(speech_set_settings(100,100,0)==0);
 commands=1;assert(speech_speak_vox("Hello there.",12)==0);
 assert(strstr(observed,"[[vers 1]]")&&strstr(observed,"[[slnc 500]]")&&!strstr(observed,"[[slnc 5000]]"));
 assert(allocs==1);assert(speech_stop()==0&&!allocs);
 assert(speech_speak_vox("  ",2)==0&&!allocs);assert(speech_speak_vox(NULL,1)==paramErr);
 assert(speech_speak_vox("x",32768)==paramErr);assert(speech_speak_vox("x\0x",3)==paramErr);
 nativeProvider=1;commands=1;conversions=0;
 assert(speech_speak_vox("Good morning and welcome to the Black Mesa Transit System.",57)==0);
 assert(conversions==8&&strstr(observed,"[[inpt PHON]] _t1EHst.% [[inpt TEXT]] [[slnc 100]]"));
 assert(strstr(observed,"[[slnc 500]]"));assert(speech_stop()==0&&!allocs);
 conversions=0;assert(speech_speak_vox("The time is 8:47 AM.",19)==0);
 assert(conversions==5);assert(speech_stop()==0&&!allocs); /* AM is one spoken beat. */
 conversionError=-22;assert(speech_speak_vox("Hello.",6)==-22&&!allocs);conversionError=0;
 modeError=-23;assert(speech_speak_vox("Hello.",6)==-23&&!allocs);modeError=0;
 /* Stop before trailing TEXT was parsed: Clean must reset PHON directly. */
 inputMode=modePhonemes;
 commands=0;assert(speech_speak_text(input,1024)==0&&inputMode==modeText);
 assert(speech_stop()==0&&!allocs);
 commands=0;assert(speech_speak_text(input,1024)==0);assert(speech_speak_text(NULL,0)==0&&!allocs);assert(speech_shutdown()==0&&!gChannel);
 return 0;
}
''')
 for name in ('network','speech'):
  subprocess.run(['cc','-g','-O1','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined','-Wl,--gc-sections','-Werror=incompatible-pointer-types','-I',str(p),'-I',str(root),str(p/(name+'_test.c')), *([str(root/'vox_parser.c')] if name=='speech' else []), '-o',str(p/name)],check=True)
  subprocess.run([str(p/name)],check=True)
 print('network receive: 6 scenarios passed; owned speech: copy, completion, stop, quit and failure ownership passed')
