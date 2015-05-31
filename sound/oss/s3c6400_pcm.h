/*
 * Common audio handling for the SA11x0
 *
 * Copyright (c) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */

/*
 * Buffer Management
 */

typedef struct {
	int offset;		/* current offset */
	char *data;		/* points to actual buffer */
	dma_addr_t dma_addr;	/* physical buffer address */
	int dma_ref;		/* DMA refcount */
	int master;		/* owner for buffer allocation, contain size when true */
} audio_buf_t;

typedef struct {
	char *id;		/* identification string */
	struct device *dev;	/* device */
	audio_buf_t *buffers;	/* pointer to audio buffer structures */
	u_int dma;		/* user fragment index */
	u_int subchannel;		/* user fragment index */
	u_int usr_head;		/* user fragment index */
	u_int dma_head;		/* DMA fragment index to go */
	u_int dma_tail;		/* DMA fragment index to complete */
	u_int fragsize;		/* fragment i.e. buffer size */
	u_int nbfrags;		/* nbr of fragments i.e. buffers */
	u_int pending_frags;	/* Fragments sent to DMA */
	int bytecount;		/* nbr of processed bytes */
	int free_bufnum;		/* nbr of processed bytes */
	int fragcount;		/* nbr of fragment transitions */
	struct semaphore sem;	/* account for fragment usage */
	wait_queue_head_t wq;	/* for poll */
	int mapped:1;		/* mmap()'ed buffers */
	int active:1;		/* actually in progress */
	int stopped:1;		/* might be active but stopped */
} audio_stream_t;

/*
 * State structure for one instance
 */

typedef struct {
	audio_stream_t *output_stream;
	audio_stream_t *input_stream_line;
	audio_stream_t *input_stream_mic;
	audio_stream_t *input_stream;
	int rd_ref:1;		/* open reference for recording */
	int wr_ref:1;		/* open reference for playback */
	int need_tx_for_rx:1;	/* if data must be sent while receiving */
	void *data;
	void (*hw_init)(int *);
	void (*hw_shutdown)(void *);
	int (*client_ioctl)(struct inode *, struct file *, uint, ulong);
	struct semaphore sem;	/* to protect against races in attach() */
	int sound_mode;
} audio_state_t;

#define STEREO  2
#define MONO	1
#define CONFIG_MIC_PATH
/*
 * Functions exported by this module
 */
//extern int s3c64xx_audio_attach( struct inode *inode, struct file *file,
extern int s3c_audio_attach( struct inode *inode, struct file *file,
				audio_state_t *state);
int s3c64xx_audio_suspend(audio_state_t *s, u32 state, u32 level);
int s3c64xx_audio_resume(audio_state_t *s, u32 level);

/*
 * exported by this module
 */
enum {
        SUSPEND_NOTIFY,
        SUSPEND_SAVE_STATE,
        SUSPEND_DISABLE,
        SUSPEND_POWER_DOWN,
};

enum {
        RESUME_POWER_ON,
        RESUME_RESTORE_STATE,
        RESUME_ENABLE,
};

#ifdef CONFIG_SOUND_S3C6400_I2S
extern int wm8753_set_mic1_path(void);
extern int wm8753_set_linein_path(void);
extern int wm8753_set_hpout_path(void);
#endif
