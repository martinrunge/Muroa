/*
 * CFakeMediaCollection.cpp
 *
 *  Created on: 21 Dec 2010
 *      Author: martin
 */

#include "CFakeMediaCollection.h"

#include "../../libmuroa++/CMediaItem.h"
#include <sstream>

#include "boost/filesystem.hpp"   // includes all needed Boost.Filesystem declarations
#include <iostream>               // for std::cout
#include <cstring>

#include <taglib/fileref.h>
#include <taglib/tag.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}


using namespace boost::filesystem;
using namespace std;
using namespace muroa;

CFakeMediaCollection::CFakeMediaCollection(string indir) : m_rootdir(indir) {

	/* register all the codecs */
	av_register_all();

}

CFakeMediaCollection::~CFakeMediaCollection() {
}


std::vector<CMediaItem*>* CFakeMediaCollection::collectionWithoutFiles( int size ) {
	vector<CMediaItem*>* collection = new vector<CMediaItem*>;

	for(int i = 0; i < size; i++ ) {
		CMediaItem* item = new CMediaItem();

		ostringstream ss;
		ss << "Test Album " << i;
		item->setAlbum( ss.str() );

		ss.str("");
		ss << "Test Artist " << i;
		item->setArtist(ss.str());

		ss.str("");
		ss << "Test Title " << i;
		item->setTitle(ss.str());

		item->setYear( 1990 + i % 20 );
		item->setDuration( 240 );

		collection->push_back(item);
	}

	return collection;
}

int CFakeMediaCollection::mp3Ccollection(int numArtists, int numAlbumsPerArtist, int numTitlesPerAlbum) {
	const string tmplname = "template.mp3";

	path mp3_path( m_rootdir );
	path tmp_file = mp3_path;
	tmp_file /= tmplname;
	createMp3File(tmp_file.string());

	ostringstream oss_artist, oss_album, oss_title;

	for(int artistNo = 0; artistNo < numArtists; artistNo++) {
		oss_artist.str(string());
		oss_artist << "Test Artist " << artistNo;
		path artist_path = mp3_path;
		artist_path /= oss_artist.str();

		for(int albumNo = 0; albumNo < numAlbumsPerArtist; albumNo++) {
			oss_album.str(string());
			oss_album << "Test Album " << albumNo;
			path album_path = artist_path;
			album_path /= oss_album.str();
			if ( !exists(album_path)) {
				create_directories(album_path);
				cerr << "mkpath: " << album_path <<endl;
			}

			for(int titleNo = 0; titleNo < numTitlesPerAlbum; titleNo++) {
				oss_title.str(string());
				oss_title <<  "Test Title " << titleNo;

				ostringstream oss_fn;
				oss_fn.str(string());
				oss_fn << titleNo << " " << oss_title.str() << ".mp3";

				path filename = album_path;
				filename /= oss_fn.str();

				copy(tmp_file, filename);
				// createMp3File(filename.string());

				char buffer[32];
				memset(buffer, 0, 32);

				FILE* fakefile = fopen(filename.string().c_str(), "wb");
				fwrite(buffer, 32, 32, fakefile);
				fclose(fakefile);

				TagLib::FileRef f( filename.string().c_str(), false);

				f.tag()->setArtist(oss_artist.str());
				f.tag()->setAlbum(oss_album.str());
				f.tag()->setTitle(oss_title.str());
				f.tag()->setYear(1234);

				f.save();
			}
		}
	}
}

/**
 * This method is taken from libavcodec/api-example.c
 */
void CFakeMediaCollection::createMp3File(std::string filename) {

	AVCodec *codec;
	AVCodecContext *c= NULL;
	int frame_size, i, j, out_size, outbuf_size;
	FILE *f;
	short *samples;
	float t, tincr;
	uint8_t *outbuf;

	cerr << "Audio encoding" << endl;

	/* find the MP2 encoder */
	codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
	if (!codec) {
		fprintf(stderr, "codec not found\n");
		return;
	}

	c= avcodec_alloc_context3(codec);

	/* put sample parameters */
	c->bit_rate = 64000;
	c->sample_rate = 44100;
	c->channels = 2;

	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		return;
	}

	/* the codec gives us the frame size, in samples */
	frame_size = c->frame_size;
	samples = (short int*)malloc(frame_size * 2 * c->channels);
	outbuf_size = 10000;
	outbuf = (uint8_t*)malloc(outbuf_size);

	f = fopen(filename.c_str(), "wb");
	if (!f) {
		cerr << "could not open" << filename << endl;
		return;
	}

	/* encode a single tone sound */
	t = 0;
	tincr = 2 * M_PI * 440.0 / c->sample_rate;
	for(i=0;i<200;i++) {
		for(j=0;j<frame_size;j++) {
			samples[2*j] = (int)(sin(t) * 10000);
			samples[2*j+1] = samples[2*j];
			t += tincr;
		}
		/* encode the samples */
		// out_size = avcodec_encode_audio2(c, outbuf, outbuf_size, samples);
		fwrite(outbuf, 1, out_size, f);
	}
	fclose(f);
	free(outbuf);
	free(samples);

	avcodec_close(c);
	av_free(c);

}

