//
//  FSPlay.h
//  ffplayios
//
//  Created by andy on 2023/5/9.
//

#ifndef ffplay_h
#define ffplay_h

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavutil/bprint.h"
#include "libavutil/frame.h"
#include "libavformat/avformat.h"
//#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

#if CONFIG_AVFILTER
# include "libavfilter/avfilter.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_thread.h"

#include <assert.h>
#include "libavutil/common.h"
#include "render_frame.hpp"
#ifdef __cplusplus
}
#endif

#include "FSCmdUtils.hpp"

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define CURSOR_HIDE_DELAY 1000000

#define USE_ONEPASS_SUBTITLE_RENDER 1

extern void fs_log_set_callback(void (*callback)(void *, int, const char *, va_list));

typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList *next = NULL;
    int serial = 0;
} MyAVPacketList;

typedef struct PacketQueue {
    MyAVPacketList *first_pkt = NULL, *last_pkt = NULL;
    int nb_packets = 0;
    int size = 0;
    int64_t duration = 0;
    int abort_request = 0;
    int serial = 0;
    SDL_mutex *mutex = NULL;
    SDL_cond *cond = NULL;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

typedef struct AudioParams {
    int freq = 0;
    int channels = 0;
    int64_t channel_layout = 0;
    enum AVSampleFormat fmt = AV_SAMPLE_FMT_NONE;
    int frame_size = 0;
    int bytes_per_sec = 0;
} AudioParams;

typedef struct Clock {
    double pts = 0;           /* clock base */
    double pts_drift = 0;     /* clock base minus time at which we updated the clock */
    double last_updated = 0;
    double speed = 0;
    int serial = 0;           /* clock is based on a packet with this serial */
    int paused = 0;
    int *queue_serial = NULL;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
    AVFrame *frame = NULL;
    AVSubtitle sub;
    int serial = 0;
    double pts = 0;           /* presentation timestamp for the frame */
    double duration = 0;      /* estimated duration of the frame */
    int64_t pos = 0;          /* byte position of the frame in the input file */
    int width = 0;
    int height = 0;
    int format = 0;
    AVRational sar;
    int uploaded = 0;
    int flip_v = 0;
} Frame;

typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex = 0;
    int windex = 0;
    int size = 0;
    int max_size = 0;
    int keep_last = 0;
    int rindex_shown = 0;
    SDL_mutex *mutex = NULL;
    SDL_cond *cond = NULL;
    PacketQueue *pktq = NULL;
} FrameQueue;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct Decoder {
    AVPacket pkt;
    PacketQueue *queue = NULL;
    AVCodecContext *avctx = NULL;
    int pkt_serial = 0;
    int finished = 0;
    int packet_pending = 0;
    SDL_cond *empty_queue_cond = NULL;
    int64_t start_pts = 0;
    AVRational start_pts_tb;
    int64_t next_pts = 0;
    AVRational next_pts_tb;
    SDL_Thread *decoder_tid = NULL;
} Decoder;

enum ShowMode {
    SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
};

typedef struct VideoState {
    SDL_Thread *read_tid = NULL;
    SDL_Thread *video_refresh_tid = NULL;
    AVInputFormat *iformat = NULL;
    int abort_request = 0;
    int force_refresh = 0;
    int paused = 0;
    int last_paused = 0;
    int queue_attachments_req = 0;
    int seek_req = 0;
    int seek_flags = 0;
    int64_t seek_pos = 0;
    int64_t seek_rel = 0;
    int read_pause_return = 0;
    AVFormatContext *ic = NULL;
    int realtime = 0;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    FrameQueue subpq;
    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;
    Decoder subdec;

    int audio_stream = 0;

    int av_sync_type = 0;

    double audio_clock = 0;
    int audio_clock_serial = 0;
    double audio_diff_cum = 0; /* used for AV difference average computation */
    double audio_diff_avg_coef = 0;
    double audio_diff_threshold = 0;
    int audio_diff_avg_count = 0;
    AVStream *audio_st = NULL;
    PacketQueue audioq;
    int audio_hw_buf_size = 0;
    uint8_t *audio_buf = NULL;
    uint8_t *audio_buf1 = NULL;
    unsigned int audio_buf_size = 0; /* in bytes */
    unsigned int audio_buf1_size = 0;
    int audio_buf_index = 0; /* in bytes */
    int audio_write_buf_size = 0;
    int audio_volume = 0;
    int muted = 0;
    struct AudioParams audio_src;
#if CONFIG_AVFILTER
    struct AudioParams audio_filter_src;
#endif
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx = NULL;
    int frame_drops_early = 0;
    int frame_drops_late = 0;

    enum ShowMode show_mode = SHOW_MODE_NONE;
    
    int16_t sample_array[SAMPLE_ARRAY_SIZE] = {0};
    int sample_array_index = 0;
    int last_i_start = 0;
    RDFTContext *rdft = NULL;
    int rdft_bits = 0;
    FFTSample *rdft_data = NULL;
    int xpos = 0;
    double last_vis_time = 0;
    SDL_Texture *vis_texture = NULL;
    SDL_Texture *sub_texture = NULL;
    SDL_Texture *vid_texture = NULL;

    int subtitle_stream = 0;
    AVStream *subtitle_st = NULL;
    PacketQueue subtitleq;

    double frame_timer = 0;
    double frame_last_returned_time = 0;
    double frame_last_filter_delay = 0;
    int video_stream = 0;
    AVStream *video_st = NULL;
    PacketQueue videoq;
    double max_frame_duration = 0;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    struct SwsContext *img_convert_ctx = NULL;
    struct SwsContext *sub_convert_ctx = NULL;
    int eof = 0;

    char *filename = NULL;
    int width = 0, height = 0, xleft = 0, ytop = 0;
    int step = 0;

#if CONFIG_AVFILTER
    int vfilter_idx = 0;
    AVFilterContext *in_video_filter = NULL;   // the first filter in the video chain
    AVFilterContext *out_video_filter = NULL;  // the last filter in the video chain
    AVFilterContext *in_audio_filter = NULL;   // the first filter in the audio chain
    AVFilterContext *out_audio_filter = NULL;  // the last filter in the audio chain
    AVFilterGraph *agraph = NULL;              // audio filter graph
#endif

    int last_video_stream = 0, last_audio_stream = 0, last_subtitle_stream = 0;

    SDL_cond *continue_read_thread = NULL;
} VideoState;

enum FSPlayStatus {
    FSPlayStatus_completed,
    FSPlayStatus_error,
};

typedef void(*SetParamsCallback)(void *);
typedef void(*RenderFrameCallback)(void *, VideoFrame *);
typedef void(*PlayStatusChangeCallback)(void *, enum FSPlayStatus);

class FSPlay {
public:
    FSPlay();
    
    VideoState *is = NULL;
    FSCmdUtils *cmdUtils = NULL;
    
    void sdl_audio_callback(void *opaque, uint8_t *stream, int len);
    int audio_thread(void *arg);
    int video_thread(void *arg);
    int subtitle_thread(void *arg);
    int video_refresh_thread(void *arg);
    int read_thread(void *arg);
    
    int64_t start_time = AV_NOPTS_VALUE;
    int audio_disable = 0;
    PlayStatusChangeCallback statusChangeCallback = NULL;
    
    int ffplay_main(void *openglesView, char *url, SetParamsCallback paramsCallback, RenderFrameCallback renderCallback);
    void ffplay_stream_close();
    void ffplay_pause();
    
    int opt_add_vfilter(void *optctx, const char *opt, const char *arg);
    void set_format_opt(const char *key, const char *value);
    int set_input_format(const char *short_name);
    
private:
    RenderFrameCallback renderCallback = NULL;
    int play_serial = 1;
    /* options specified by the user */
    AVInputFormat *file_iformat = NULL;
    char *input_filename = NULL;
    char *window_title = NULL;
    int default_width  = 640;
    int default_height = 480;
    int screen_width  = 0;
    int screen_height = 0;
    int screen_left = SDL_WINDOWPOS_CENTERED;
    int screen_top = SDL_WINDOWPOS_CENTERED;
    int video_disable = 0;
    int subtitle_disable = 0;
    char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
    int seek_by_bytes = -1;
    float seek_interval = 10;
    int display_disable = 0;
    int borderless = 0;
    int alwaysontop = 0;
    int startup_volume = 100;
    int show_status = -1;
    int av_sync_type = AV_SYNC_AUDIO_MASTER;
    int64_t duration = AV_NOPTS_VALUE;
    int fast = 0;
    int genpts = 0;
    int lowres = 0;
    int decoder_reorder_pts = -1;
    int autoexit = 0;
    int exit_on_keydown = 0;
    int exit_on_mousedown = 0;
    int loop = 1;
    int framedrop = -1;
    int infinite_buffer = -1;
    enum ShowMode show_mode = SHOW_MODE_NONE;
    char *audio_codec_name = NULL;
    char *subtitle_codec_name = NULL;
    char *video_codec_name = NULL;
    double rdftspeed = 0.02;
    int64_t cursor_last_shown = 0;
    int cursor_hidden = 0;
    #if CONFIG_AVFILTER
    char **vfilters_list = NULL;
    int nb_vfilters = 0;
    char *afilters = NULL;
    #endif
    int autorotate = 1;
    int find_stream_info = 1;
    int filter_nbthreads = 0;

    /* current context */
    int is_full_screen = 0;
    int64_t audio_callback_time = 0;

    AVPacket flush_pkt;

    #define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

    struct TextureFormatEntry {
        enum AVPixelFormat format;
        int texture_fmt;
    } sdl_texture_format_map[20] = {
        { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
        { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
        { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
        { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
        { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
        { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
        { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
        { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
        { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
        { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
        { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
        { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
        { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
        { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
        { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
        { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
        { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
        { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
        { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
        { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
    };

    void *openglesView = NULL;
    SwsContext *swsContext = NULL;
    AVFrame *rgbFrame = NULL;
    int packet_queue_put_private(PacketQueue *q, AVPacket *pkt);
    int packet_queue_put(PacketQueue *q, AVPacket *pkt);
    int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);
    int packet_queue_init(PacketQueue *q);
    void packet_queue_flush(PacketQueue *q);
    void packet_queue_destroy(PacketQueue *q);
    void packet_queue_abort(PacketQueue *q);
    void packet_queue_start(PacketQueue *q);
    int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);
    void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond);
    int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub);
    void decoder_destroy(Decoder *d);
    void frame_queue_unref_item(Frame *vp);
    int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);
    void frame_queue_destory(FrameQueue *f);
    void frame_queue_signal(FrameQueue *f);
    Frame *frame_queue_peek(FrameQueue *f);
    Frame *frame_queue_peek_next(FrameQueue *f);
    Frame *frame_queue_peek_last(FrameQueue *f);
    Frame *frame_queue_peek_writable(FrameQueue *f);
    Frame *frame_queue_peek_readable(FrameQueue *f);
    void frame_queue_push(FrameQueue *f);
    void frame_queue_next(FrameQueue *f);
    int frame_queue_nb_remaining(FrameQueue *f);
    int64_t frame_queue_last_pos(FrameQueue *f);
    void decoder_abort(Decoder *d, FrameQueue *fq);
    void copyFrameData(uint8_t *src, uint8_t *dst, int linesize, int width, int height);
    void video_image_display(VideoState *is);
    void stream_component_close(VideoState *is, int stream_index);
    void stream_close(VideoState *is);
    void video_display(VideoState *is);
    double get_clock(Clock *c);
    void set_clock_at(Clock *c, double pts, int serial, double time);
    void set_clock(Clock *c, double pts, int serial);
    void set_clock_speed(Clock *c, double speed);
    void init_clock(Clock *c, int *queue_serial);
    void sync_clock_to_slave(Clock *c, Clock *slave);
    int get_master_sync_type(VideoState *is);
    double get_master_clock(VideoState *is);
    void check_external_clock_speed(VideoState *is);
    void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes);
    void stream_toggle_pause(VideoState *is);
    void toggle_pause(VideoState *is);
    void toggle_mute(VideoState *is);
    void update_volume(VideoState *is, int sign, double step);
    void step_to_next_frame(VideoState *is);
    double compute_target_delay(double delay, VideoState *is);
    double vp_duration(VideoState *is, Frame *vp, Frame *nextvp);
    void update_video_pts(VideoState *is, double pts, int64_t pos, int serial);
    void video_refresh(void *opaque, double *remaining_time);
    int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);
    int get_video_frame(VideoState *is, AVFrame *frame);
    int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph,
                              AVFilterContext *source_ctx, AVFilterContext *sink_ctx);
    int configure_video_filters(AVFilterGraph *graph, VideoState *is, const char *vfilters, AVFrame *frame);
    int configure_audio_filters(VideoState *is, const char *afilters, int force_output_format);
    int decoder_start(Decoder *d, int (*fn)(void *), const char *thread_name, void* arg);
    void update_sample_display(VideoState *is, short *samples, int samples_size);
    int synchronize_audio(VideoState *is, int nb_samples);
    int audio_decode_frame(VideoState *is);
    int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params);
    int stream_component_open(VideoState *is, int stream_index);
    int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue);
    int is_realtime(AVFormatContext *s);
    VideoState *stream_open(VideoState *is, const char *filename, AVInputFormat *iformat);
    void stream_cycle_channel(VideoState *is, int codec_type);
    void seek_chapter(VideoState *is, int incr);
    int opt_frame_size(void *optctx, const char *opt, const char *arg);
    int opt_width(void *optctx, const char *opt, const char *arg);
    int opt_height(void *optctx, const char *opt, const char *arg);
    int opt_format(void *optctx, const char *opt, const char *arg);
    int opt_frame_pix_fmt(void *optctx, const char *opt, const char *arg);
    int opt_sync(void *optctx, const char *opt, const char *arg);
    int opt_seek(void *optctx, const char *opt, const char *arg);
    int opt_duration(void *optctx, const char *opt, const char *arg);
    int opt_show_mode(void *optctx, const char *opt, const char *arg);
    void opt_input_file(void *optctx, const char *filename);
    int opt_codec(void *optctx, const char *opt, const char *arg);
    void show_usage(void);
//    void show_help_default(const char *opt, const char *arg);
    
    inline
    int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1,
                       enum AVSampleFormat fmt2, int64_t channel_count2);
    inline
    int64_t get_valid_channel_layout(int64_t channel_layout, int channels);
    inline int compute_mod(int a, int b);
};



#endif /* ffplay_h */
