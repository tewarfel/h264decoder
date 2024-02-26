extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
}

#include <iostream>

#include "h264decoder.hpp"

typedef unsigned char ubyte;

/* For backward compatibility with release 9 or so of libav */
#if (LIBAVCODEC_VERSION_MAJOR <= 54) 
#  define av_frame_alloc avcodec_alloc_frame
#  define av_frame_free  avcodec_free_frame  
#endif


H264Decoder::H264Decoder()
  : pkt_{std::make_unique<AVPacket>()}
{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  avcodec_register_all();
#endif

  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!codec)
    throw H264InitFailure("cannot find decoder");
  
  context = avcodec_alloc_context3(codec);
  if (!context)
    throw H264InitFailure("cannot allocate context");


//  context->pix_fmt = AV_PIX_FMT_YUV420P;
//  context->color_range = AVCOL_RANGE_MPEG; 

  printf("Declaring context for 1280x720 image, YUV420P\n");
  int err = avcodec_open2(context, codec, nullptr);
  if (err < 0)
    throw H264InitFailure("cannot open context");

  parser = av_parser_init(AV_CODEC_ID_H264);
  if (!parser)
    throw H264InitFailure("cannot init parser");
  
  frame = av_frame_alloc();
  if (!frame)
    throw H264InitFailure("cannot allocate frame");

  if (!pkt_)
    throw H264InitFailure("cannot allocate packet");
  av_init_packet(pkt_.get());
}


H264Decoder::~H264Decoder()
{
  av_parser_close(parser);
  avcodec_close(context);
  av_free(context);
  av_frame_free(&frame);
}


ParseResult H264Decoder::parse(const ubyte* in_data, ptrdiff_t in_size)
{
  auto nread = av_parser_parse2(parser, context, &pkt_->data, &pkt_->size, 
                                in_data, in_size, 
                                0, 0, AV_NOPTS_VALUE);
  // We might have a frame to decode. But what exactly is a packet? 
  //Is it guaranteed to contain a complete frame?
  if (pkt_->size)
  {
    const auto frame = decode_frame();
    return ParseResult{nread, frame};
  }
  return ParseResult{nread, nullptr};
}


const AVFrame* H264Decoder::decode_frame()
{
#if (LIBAVCODEC_VERSION_MAJOR > 56)
  int ret = avcodec_send_packet(context, pkt_.get());
  if (!ret) {
    ret = avcodec_receive_frame(context, frame);
    if (!ret)
      return frame;
  }
  return nullptr;
#else
  int got_picture = 0;
  int nread = avcodec_decode_video2(context, frame, &got_picture, pkt);
  if (nread < 0 || got_picture == 0)
    return nullptr;
  return *frame;
#endif
}


ConverterRGB24::ConverterRGB24()
{
  framergb = av_frame_alloc();
  if (!framergb)
    throw H264DecodeFailure("cannot allocate frame");
  context = nullptr;
}


ConverterRGB24::~ConverterRGB24()
{
  sws_freeContext(context);
  av_frame_free(&framergb);
}



const AVFrame& ConverterRGB24::convert(const AVFrame &frame, ubyte* out_rgb)
{
  int w = frame.width;
  int h = frame.height;

  // TEW - added to fix the "deprecated format" error msg
  int pix_fmt = (frame.format != AV_PIX_FMT_YUVJ420P ? frame.format : AV_PIX_FMT_YUV420P);
  
  context = sws_getCachedContext(context, 
                                 w, h, (AVPixelFormat)pix_fmt, 
                                 w, h, AV_PIX_FMT_BGR24, SWS_BILINEAR,
                                 nullptr, nullptr, nullptr);
  if (!context)
    throw H264DecodeFailure("cannot allocate context");
  
   // TEW - added to fix the "deprecated format" error msg
  {
    int *coefficients1;
    int *coefficients2;
    int full_range1;
    int full_range2;
    int brightness, contrast, saturation;

    if ( sws_getColorspaceDetails( context, &coefficients1, &full_range1, &coefficients2, &full_range2,
                                  &brightness, &contrast, &saturation ) != -1 ) {
        full_range1 = AVCOL_RANGE_MPEG;
        sws_setColorspaceDetails( context, coefficients1, full_range1, coefficients2, full_range2,
                                  brightness, contrast, saturation );
    }
  }

  // Setup framergb with out_rgb as external buffer. Also say that we want BGR24 output.
  av_image_fill_arrays(framergb->data, framergb->linesize, out_rgb, AV_PIX_FMT_BGR24, w, h, 1);

  // Do the conversion.
  sws_scale(context, frame.data, frame.linesize, 0, h,
            framergb->data, framergb->linesize);
  framergb->width = w;
  framergb->height = h;
  return *framergb;
}


/*
Determine required size of framebuffer.

avpicture_get_size is used in http://dranger.com/ffmpeg/tutorial01.html 
to do this. However, avpicture_get_size returns the size of a compact 
representation, without padding bytes. Since we use av_image_fill_arrays to
fill the buffer we should also use it to determine the required size.
*/
int ConverterRGB24::predict_size(int w, int h)
{
  return av_image_fill_arrays(framergb->data, framergb->linesize, nullptr, AV_PIX_FMT_BGR24, w, h, 1);
}



std::pair<int, int> width_height(const AVFrame& f)
{
  return std::make_pair(f.width, f.height);
}


int row_size(const AVFrame& f)
{
  return f.linesize[0];
}


void disable_logging()
{
  av_log_set_level(AV_LOG_QUIET);
}
