//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound sample loader
//////////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <stdlib.h>

#include "DebugInterface.h"
#include "alsound_local.h"

#include "al/alut.h"

static CEqSoundThreadedLoader s_threadedSoundLoader;

CEqSoundThreadedLoader* g_pThreadedSoundLoader = &s_threadedSoundLoader;

// WAVE FILE LOADER

typedef struct                                  /* WAV File-header */
{
	ALubyte  Id[4];
	ALsizei  Size;
	ALubyte  Type[4];
} wavhdr_t;

typedef struct                                  /* WAV Fmt-header */
{
	ALushort Format;                              
	ALushort Channels;
	ALuint   SamplesPerSec;
	ALuint   BytesPerSec;
	ALushort BlockAlign;
	ALushort BitsPerSample;
} wavfmthdr_t;

typedef struct									/* WAV FmtEx-header */
{
	ALushort Size;
	ALushort SamplesPerBlock;
} wavexhdr_t;

typedef struct                                  /* WAV Smpl-header */
{
	ALuint   Manufacturer;
	ALuint   Product;
	ALuint   SamplePeriod;                          
	ALuint   Note;                                  
	ALuint   FineTune;                              
	ALuint   SMPTEFormat;
	ALuint   SMPTEOffest;
	ALuint   Loops;
	ALuint   SamplerData;

	struct
	{
		ALuint Identifier;
		ALuint Type;
		ALuint Start;
		ALuint End;
		ALuint Fraction;
		ALuint Count;
	}Loop[1];

}wavsamplehdr_t;

typedef struct                                  /* WAV Chunk-header */
{
	ALuint Id;
	ALuint Size;
} wavchunkhdr_t;

typedef struct 
{
	ALuint Name; 
	ALuint Position;
	ALuint fccChunk;
	ALuint ChunkStart;
	ALuint BlockStart; 
	ALuint SampleOffset;
} wavcuehdr_t;

#define WAV_DATA_ID			(('a'<<24)+('t'<<16)+('a'<<8)+'d')
#define WAV_FMT_ID			((' '<<24)+('t'<<16)+('m'<<8)+'f')
#define WAV_CUE_ID			((' '<<24)+('e'<<16)+('u'<<8)+'c')
#define WAV_SAMPLE_ID		(('l'<<24)+('p'<<16)+('m'<<8)+'s')

void LoadWavFromBufferEX(ubyte *memory, ALenum *format, ALvoid **data,ALsizei *size,ALsizei *freq, ALboolean* loop)
{
	wavchunkhdr_t ChunkHdr;
	wavexhdr_t FmtExHdr;
	wavhdr_t FileHdr;
	wavsamplehdr_t SmplHdr;
	wavfmthdr_t FmtHdr;

	ubyte *Stream;
	
	*format = AL_FORMAT_MONO16;
	*data = NULL;
	*size = 0;
	*freq = 22050;
	*loop = AL_FALSE;

	if (memory)
	{		
		Stream = memory;
		if (Stream)
		{
		    memcpy(&FileHdr,Stream,sizeof(wavhdr_t));
		    Stream+=sizeof(wavhdr_t);
			FileHdr.Size=((FileHdr.Size+1)&~1)-4;

			while ((FileHdr.Size!=0) && (memcpy(&ChunkHdr, Stream, sizeof(wavchunkhdr_t))))
			{
				Stream += sizeof(wavchunkhdr_t);
			    
				if (ChunkHdr.Id == WAV_FMT_ID)
				{
					memcpy(&FmtHdr,Stream,sizeof(wavfmthdr_t));

					if (FmtHdr.Format==0x0001)
					{
						*format=(FmtHdr.Channels==1?
								(FmtHdr.BitsPerSample==8?AL_FORMAT_MONO8:AL_FORMAT_MONO16):
								(FmtHdr.BitsPerSample==8?AL_FORMAT_STEREO8:AL_FORMAT_STEREO16));

						*freq = FmtHdr.SamplesPerSec;
						Stream += ChunkHdr.Size;
					} 
					else
					{
						memcpy(&FmtExHdr,Stream,sizeof(wavexhdr_t));
						Stream+=ChunkHdr.Size;
					}
				}
				else if (ChunkHdr.Id == WAV_DATA_ID)
				{
					if (FmtHdr.Format==0x0001)
					{
						*size = ChunkHdr.Size;
						*data = malloc(ChunkHdr.Size + 31);

						if (*data) 
						{
							memcpy(*data,Stream,ChunkHdr.Size);
						    memset(((char *)*data)+ChunkHdr.Size,0,31);
							Stream += ChunkHdr.Size;
						}
					}
					else if (FmtHdr.Format==0x0011)
					{
						//IMA ADPCM
					}
					else if (FmtHdr.Format==0x0055)
					{
						//MP3 WAVE
					}
				}
				else if (ChunkHdr.Id == WAV_SAMPLE_ID)
				{
				   	memcpy(&SmplHdr,Stream,sizeof(wavsamplehdr_t));
					Stream += ChunkHdr.Size;
				}
				else if (ChunkHdr.Id == WAV_CUE_ID)
				{
					wavcuehdr_t *cueData = (wavcuehdr_t*)Stream;
					*loop = (cueData->SampleOffset != 0xFFFF);
					Stream += ChunkHdr.Size;
				}

				else Stream += ChunkHdr.Size;
				Stream += ChunkHdr.Size&1;
				FileHdr.Size -= (((ChunkHdr.Size+1)&~1)+8);
			}
		}
	}
}


DkSoundSampleLocal::DkSoundSampleLocal()
{
	m_bLooping = false;
	m_bStreaming = false;
	m_bIsLoaded = false;
	m_szName = "null.wav";
	m_nChannels = 1;
}

DkSoundSampleLocal::~DkSoundSampleLocal()
{
	alDeleteBuffers(1, &m_nALBuffer);
}

int DkSoundSampleLocal::GetFlags()
{
	return m_nFlags;
}

void DkSoundSampleLocal::Load(const char *name, bool streaming, bool looping, int nFlags)
{
	//Copy all
	m_szName = name;
	m_bStreaming = streaming;
	m_bLooping = looping;
	m_nFlags = nFlags;

	//create single buffer
	alGenBuffers(1, &m_nALBuffer);

	if(!streaming)
	{
		EqString ext = m_szName.Path_Extract_Ext();
		
		if(!stricmp(ext.GetData(),"wav"))
			LoadWav( name, m_nALBuffer );
		else if(!stricmp(ext.GetData(),"ogg"))
			LoadOgg( name, m_nALBuffer );
		else
			MsgError("DkSoundSample::Load '%s' failed, extension not supported\n", name);
	}
}

void DkSoundSampleLocal::LoadWav(const char *name, unsigned int buffer)
{
	//return;

	//Load a wav
	ALenum format;
	ALvoid *data;
	ALsizei size;
	ALsizei freq;
	ALboolean loop;

	DKFILE* file = GetFileSystem()->Open((_Es(SOUND_DEFAULT_PATH) + name).GetData(), "rb");

	if(!file)
		return;

	int fSize = file->GetSize();

	ubyte* fileBuffer = (ubyte*)malloc( fSize );
	GetFileSystem()->Read(fileBuffer, 1, fSize, file);
	GetFileSystem()->Close(file);

	LoadWavFromBufferEX( fileBuffer, &format, &data, &size, &freq, &loop );

	if((format == AL_FORMAT_MONO8) || (format == AL_FORMAT_MONO16))
		m_nChannels = 1;
	else if((format == AL_FORMAT_STEREO8) || (format == AL_FORMAT_STEREO16))
		m_nChannels = 2;

	// load buffer data into
	alBufferData( buffer, format, data, size, freq );

	m_bLooping = (loop > 0);

	free( data );

	// free hunk
	free( fileBuffer );
}

size_t eqogg_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	IFile* pFile = (IFile*)datasource;

	return pFile->Read(ptr, nmemb, size);
}

int	eqogg_seek(void *datasource, ogg_int64_t offset, int whence)
{
	IFile* pFile = (IFile*)datasource;

	// let's do some fuckin' undocumented features of ogg

	int returnVal;

	switch(whence)
	{
		case SEEK_SET:
		case SEEK_CUR:
		case SEEK_END:
			returnVal = pFile->Seek(offset, (VirtStreamSeek_e)whence);
			break;
		default: //Bad value
			return -1;
	}

	if(returnVal == 0)
		return 0;
	else
		return -1; //Could not do a seek. Device not capable of seeking. (Should never encounter this case)
}

long eqogg_tell(void *datasource)
{
	IFile* pFile = (IFile*)datasource;

	return pFile->Tell();
}

int eqogg_close(void *datasource)
{
	return 1;
}

void DkSoundSampleLocal::LoadOgg(const char *name, unsigned int buffer)
{
	ALenum format;
	ALsizei freq;

	// Open for binary reading
	DKFILE* pFile = GetFileSystem()->Open((_Es(SOUND_DEFAULT_PATH) + name).GetData(), "rb");
	if(!pFile)
		return;

	vorbis_info *pInfo;
	OggVorbis_File oggFile;

	ov_callbacks cb;

	cb.read_func = eqogg_read;
	cb.close_func = eqogg_close;
	cb.seek_func = eqogg_seek;
	cb.tell_func = eqogg_tell;

	int ovResult = ov_open_callbacks(pFile, &oggFile, NULL, 0, cb);

	if(ovResult < 0)
	{
		GetFileSystem()->Close(pFile);
		MsgError("Can't open sound '%s', is not an ogg file (%d)\n", name, ovResult);
		return;
	}

	// Get some information about the OGG file
	pInfo = ov_info(&oggFile, -1);

	// Check the number of channels... always use 16-bit samples
	if (pInfo->channels == 1)
		format = AL_FORMAT_MONO16;
	else
		format = AL_FORMAT_STEREO16;

	m_nChannels = pInfo->channels;

	// The frequency of the sampling rate
	freq = pInfo->rate;

	int nSamples = (uint) ov_pcm_total(&oggFile, -1);
	int nChannels = pInfo->channels;

	int size = nSamples * nChannels * sizeof(short);
	short* soundbuffer = (short*)PPAlloc(size);

	int samplePos = 0;

	while (samplePos < size)
	{
		char *dest = ((char *) soundbuffer) + samplePos;
		int readBytes = ov_read(&oggFile, dest, size - samplePos, 0, 2, 1, NULL);

		if (readBytes <= 0)
			break;

		samplePos += readBytes;
	}

	// Upload sound data to buffer
	alBufferData(buffer, format, soundbuffer, size, freq);

	ov_clear(&oggFile);

	GetFileSystem()->Close(pFile);

	PPFree(soundbuffer);
}
